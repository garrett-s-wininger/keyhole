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

auto inspect_class_file(std::string_view target)
        -> std::expected<void, argparse::Error> {
    if (!std::filesystem::exists(target)) {
        return std::unexpected(
            argparse::Error{
                argparse::Error::Type::CommandExecutionFailure,
                std::format("Requested file ({}) does not exist", target)
            }
        );
    }

    std::ifstream file_reader{
        std::filesystem::path{target},
        std::ios::binary | std::ios::ate
    };

    if (!file_reader.is_open()) {
        return std::unexpected(
            argparse::Error{
                argparse::Error::Type::CommandExecutionFailure,
                std::format("Failed to open file ({})", target)
            }
        );
    }

    const auto size = file_reader.tellg();
    file_reader.seekg(0, std::ios::beg);

    std::vector<std::byte> contents{};
    contents.resize(size);

    if (!file_reader.read(reinterpret_cast<char*>(contents.data()), size)) {
        return std::unexpected(
            argparse::Error{
                argparse::Error::Type::CommandExecutionFailure,
                std::format("Failed to read file ({}) contents", target)
            }
        );
    }

    reader::Reader reader{contents};
    const auto class_file = parsing::parse_class_file(reader);

    if (!class_file) {
        return std::unexpected(
            argparse::Error{
                argparse::Error::Type::CommandExecutionFailure,
                std::format("Failed to parse file ({}) contents", target)
            }
        );
    }

    const auto klass = class_file.value();

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

auto write_test_class_file(std::string_view target)
        -> std::expected<void, argparse::Error> {
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
    using InspectCommand = argparse::Command<"inspect", ::inspect_class_file>;
    using WriteCommand = argparse::Command<"test-class", ::write_test_class_file>;

    const auto result = argparse::CLI<InspectCommand, WriteCommand>{
        .name = "KeyHole CLI",
        .version = "0.1.0",
        .description = "Provides instrumentation and introspection for JVM bytecode",
    }.execute(argc, argv);

    if (!result) {
        std::println(stderr, "[ERROR] {}", result.error().message);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
