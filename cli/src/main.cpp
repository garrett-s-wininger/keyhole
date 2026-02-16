#include <filesystem>

#include "argparse.h"
#include "isa.h"
#include "parsing.h"
#include "serialization.h"
#include "views.h"

constexpr auto jdk_version(
        const kh::jvm::classfile::Version version) noexcept -> uint8_t {
    if (version.major < 49) {
        // NOTE(garrett): Bundle 1.0-1.4 just because we're being lazy here.
        return 1;
    }

    // NOTE(garrett): Starting at JDK 5, there's a 44 number wide gap between
    // the major version and the named release number.
    return version.major - 44;
}

auto attachment_targets() -> kh::argparse::CommandResult {
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
                return kh::argparse::fatal(
                    std::format(
                        "User performance data location ({}) not a directory",
                        entry.path().string()
                    )
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
        return kh::argparse::fatal(
            "No processes found via user performance data fingerprinting"
        );
    }

    return {};
}

auto inspect_class_file(std::string_view target) -> kh::argparse::CommandResult {
    auto result = kh::jvm::parsing::load_class_from_file(target);

    if (!result) {
        return kh::argparse::fatal(
            std::format(
                "Failed to parse class from file ({})",
                target
            )
        );
    }

    auto& klass = result.value().class_file;

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
                kh::jvm::constant_pool::name(entries[i])
            );
        }
    }

    if (klass.methods.size() > 0) {
        std::println("Available Methods:");

        for (auto& method : klass.methods) {
            std::println(
                "  {}",
                kh::jvm::views::MethodView{klass.constant_pool, method}.name()
            );
        }
    }

    if (klass.attributes.size() > 0) {
        std::println("Assigned Attributes:");

        for (const auto& attribute : klass.attributes) {
            std::println(
                "  {}",
                klass.constant_pool.resolve<kh::jvm::constant_pool::UTF8Entry>(
                    attribute.name_index
                ).text
            );
        }
    }

    return {};
}

auto write_modified_class(std::string_view target) -> kh::argparse::CommandResult {
    auto parse_result = kh::jvm::parsing::load_class_from_file(target);

    if (!parse_result) {
        return kh::argparse::fatal(
            std::format(
                "Failed to parse class from file ({})",
                target
            )
        );
    }

    auto& klass = parse_result.value().class_file;

    const auto instructions = kh::jvm::isa::InstructionSequence{
        kh::jvm::isa::Return
    };

    const auto mutation_result = klass.add_method(
        "generatedAction"sv, "()V"sv, instructions
    );

    if (!mutation_result) {
        return kh::argparse::fatal("Attempted method addition failed");
    }

    const auto prefix_result = klass.prefix_method(
        "empty"sv,
        "()V"sv,
        {
            kh::jvm::isa::Nop,
            kh::jvm::isa::Nop,
            kh::jvm::isa::Nop
        }
    );

    if (!prefix_result) {
        return kh::argparse::fatal("Attempted method prefixing failed");
    }

    const auto replacement_result = klass.replace_method(
        "main"sv,
        "([Ljava/lang/String;)V"sv,
        instructions
    );

    if (!replacement_result) {
        return kh::argparse::fatal("Attempt to replace method contents failed");
    }

    const auto new_name = std::string{klass.name()} + "Modified";
    klass.rename(new_name);

    const auto source_path = std::filesystem::path{target};
    const auto destination_path = source_path.parent_path()
        / (source_path.stem().string() + "Modified.class");

    std::ofstream stream{destination_path};

    if (!stream) {
        return kh::argparse::fatal(
            std::format(
                "Failed to open requested file ({})", destination_path.string()
            )
        );
    }

    kh::sinks::FileSink sink{stream};
    kh::jvm::serialization::serialize(sink, klass);

    return {};
}

auto main(const int argc, const char** argv) -> int {
    using AttachmentTargetsCommand = kh::argparse::Command<
        "attachment-targets", ::attachment_targets
    >;

    using InspectCommand = kh::argparse::Command<"inspect", ::inspect_class_file>;
    using ModifyCommand = kh::argparse::Command<"modify-class", ::write_modified_class>;

    try {
        const auto result = kh::argparse::CLI<
            AttachmentTargetsCommand,
            InspectCommand,
            ModifyCommand
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
