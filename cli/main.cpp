#include <filesystem>

#include "argparse.h"
#include "parsing.h"
#include "serialization.h"

constexpr auto jdk_version(const classfile::Version version) noexcept -> uint8_t {
    if (version.major < 49) {
        // NOTE(garrett): Bundle 1.0-1.4 just because we're being lazy here.
        return 1;
    }

    // NOTE(garrett): Starting at JDK 5, there's a 44 number wide gap between
    // the major version and the named release number.
    return version.major - 44;
}

auto attachment_targets() -> argparse::CommandResult {
    const auto tempdir = std::filesystem::temp_directory_path();
    const auto directory_iterator = std::filesystem::directory_iterator{
        tempdir,
        std::filesystem::directory_options::follow_directory_symlink
        | std::filesystem::directory_options::skip_permission_denied
    };

    // NOTE(garrett): Will need a different approach if we want to support
    // Windows, perhaps backed by some form of platform library to hide away
    // the differences.
    const auto username = std::getenv("USER");
    const auto jvm_perf_dir = std::filesystem::path{
        std::format("hsperfdata_{}", username)
    };

    auto found = false;

    for (const auto& entry : directory_iterator) {
        if (entry.path().filename() == jvm_perf_dir) {
            if (!entry.is_directory()) {
                return std::unexpected(
                    argparse::Error{
                        argparse::Error::Type::CommandExecutionFailure,
                        std::format(
                            "User performance data location ({}) not a directory",
                            entry.path().string()
                        )
                    }
                );
            }

            found = true;

            const auto subdir_iterator = std::filesystem::directory_iterator{
                entry.path()
            };

            for (const auto& sub_entry : subdir_iterator) {
                std::println("{}", sub_entry.path().filename().string());
            }
        }
    }

    if (!found) {
        return std::unexpected(
            argparse::Error{
                argparse::Error::Type::CommandExecutionFailure,
                "No processes found via user performance data fingerprinting"
            }
        );
    }

    return {};
}

auto inspect_class_file(std::string_view target) -> argparse::CommandResult {
    const auto result = parsing::load_class_from_file(target);

    if (!result) {
        return std::unexpected(
            argparse::Error{
                argparse::Error::Type::CommandExecutionFailure,
                std::format(
                    "Failed to parse class from file ({})",
                    target
                )
            }
        );
    }

    const auto& klass = result.value().class_file;

    std::println("Class File Overview:");

    std::println(
        "  Name         - {} ({})",
        klass.name(),
        klass.superclass()
    );

    std::println(
        "  Version      - {}.{} (Java {})",
        klass.version.major,
        klass.version.minor,
        jdk_version(klass.version)
    );

    std::println(
        "  Access Flags - 0x{:04X}",
        klass.access_flags
    );

    const auto entries = klass.constant_pool.entries();

    if (entries.size() > 0) {
        std::println("Constant Pool Entries:");

        for (auto i = 0uz; i < entries.size(); ++i) {
            std::println(
                "  {:>2}#: [{}]",
                i + 1,
                constant_pool::name(entries[i])
            );
        }
    }

    if (klass.methods.size() > 0) {
        std::println("Available Methods:");

        for (const auto& method : klass.methods) {
            std::println(
                "  {}",
                klass.constant_pool.resolve<constant_pool::UTF8Entry>(
                    method.name_index
                ).text
            );
        }
    }

    if (klass.attributes.size() > 0) {
        std::println("Assigned Attributes:");

        for (const auto& attribute : klass.attributes) {
            std::println(
                "  {}",
                klass.constant_pool.resolve<constant_pool::UTF8Entry>(
                    attribute.name_index
                ).text
            );
        }
    }

    return {};
}

auto write_test_class_file(std::string_view target) -> argparse::CommandResult {
    const std::string class_name{"MyClass"};
    const std::string superclass_name{"java/lang/Object"};
    classfile::ClassFile klass(class_name, superclass_name);

    std::ofstream stream{std::filesystem::path{target}};

    if (!stream) {
        return std::unexpected(
            argparse::Error{
                argparse::Error::Type::CommandExecutionFailure,
                std::format("Failed to open requested file ({})", target)
            }
        );
    }

    sinks::FileSink sink{stream};
    serialization::serialize(sink, klass);

    return {};
}

auto main(const int argc, const char** argv) -> int {
    using AttachmentTargetsCommand = argparse::Command<
        "attachment-targets", ::attachment_targets
    >;

    using InspectCommand = argparse::Command<"inspect", ::inspect_class_file>;
    using WriteCommand = argparse::Command<"write-class", ::write_test_class_file>;

    try {
        const auto result = argparse::CLI<
            AttachmentTargetsCommand,
            InspectCommand,
            WriteCommand
        >{
            .name = "KeyHole CLI",
            .version = "0.1.0",
            .description = "Provides instrumentation and introspection for JVM bytecode",
        }.execute(argc, argv);

        if (!result) {
            std::println(stderr, "[ERROR] {}", result.error().message);
            return EXIT_FAILURE;
        }
    } catch (const std::exception& e) {
        std::println(
            stderr,
            "[ERROR] An unhandled exception occurred:\n\n  {}",
            e.what()
        );

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
