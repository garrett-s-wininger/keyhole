#ifndef ARGPARSE_H
#define ARGPARSE_H

#include <expected>
#include <functional>
#include <print>
#include <span>

using namespace std::literals;

namespace kh::argparse {

struct Error {
    enum Type {
        CommandExecutionFailure,
        InvalidSubcommand,
        MalformedArgumentList
    };

    Type type;
    std::string message;
};

using CommandResult = std::expected<void, Error>;

auto fatal(std::string&& message) noexcept -> CommandResult {
    return std::unexpected(
        Error{
            Error::Type::CommandExecutionFailure,
            std::move(message)
        }
    );
}

template <typename Fn>
concept CommandImplementation = requires (Fn f) {
    requires (std::invocable<Fn>);
    { f() } -> std::same_as<CommandResult>;
} || requires (Fn f, std::string_view sv) {
    requires (std::invocable<Fn, std::string_view>);
    { f(sv) } -> std::same_as<CommandResult>;
};

template <std::size_t S>
struct NonTypeTemplateString {
    char buffer[S];

    constexpr NonTypeTemplateString(const char (&str)[S]) {
        std::copy_n(str, S, buffer);
    }

    constexpr operator std::string_view() const {
        return {buffer, S - 1};
    }
};

template <NonTypeTemplateString Name, CommandImplementation auto ExecuteFn>
struct Command {
    static constexpr std::string_view name = Name;

    static auto execute(std::string_view path, std::span<const char*> args)
            -> CommandResult {
        std::string_view error_message = ""sv;

        if (args.size() == 1) {
            if constexpr (std::invocable<decltype(ExecuteFn)>) {
               return std::invoke(ExecuteFn);
            } else {
                usage(path);
                error_message = "Requested command only takes a single argument"sv;
            }
        } else if (args.size() == 2) {
            if constexpr (std::invocable<decltype(ExecuteFn), std::string_view>) {
                return std::invoke(ExecuteFn, args[1]);
            } else {
                usage(path);
                error_message = "Requested command does not take any arguments"sv;
            }
        } else {
            usage(path);
            error_message = "Too many arguments provided to requested function"sv;
        }

        return std::unexpected(
            Error{
                Error::Type::MalformedArgumentList,
                std::string{error_message}
            }
        );
    }

    static auto usage(std::string_view path) -> void {
        std::println(stderr, "Usage:");

        if constexpr(std::invocable<decltype(ExecuteFn)>) {
            std::println(stderr, " {} {}", path, name);
        } else {
            std::println(stderr, " {} {} <ARGS>", path, name);
        }
    }
};

template <typename... Subcommands>
struct CLI {
    static_assert(
        sizeof...(Subcommands) > 0,
        "CLIs must have at least one subcommand"
    );

    std::string_view name;
    std::string_view version;
    std::string_view description;

    auto execute(const int argc, const char** argv)
            -> std::expected<void, Error> {
        const auto args = std::span<const char*>(
            argv,
            static_cast<std::size_t>(argc)
        );

        const auto& head = args[0];

        // TODO(garrett): Flag parsing/support

        if (args.size() < 2) {
            usage(head, stderr);

            return std::unexpected(
                Error{
                    Error::Type::MalformedArgumentList,
                    "A subcommand must be invoked"
                }
            );
        }

        const auto& next = args[1];

        if (next == "-h"sv || next == "--help"sv) {
            help(head);
            return {};
        }

        std::optional<CommandResult> dispatch_result = std::nullopt;

        ([args, &head, &next, &dispatch_result]{
            if (Subcommands::name == next) { 
                dispatch_result = Subcommands::execute(
                    head,
                    args.subspan(1)
                );
            } else {
                return;
            }
        }(), ...);

        if (!dispatch_result) {
            usage(head, stderr);
        }

        return dispatch_result.value_or(
            std::unexpected(
                Error{
                    Error::Type::InvalidSubcommand,
                    std::format(
                        "The requested subcommand ({}) is not registered",
                        next
                    )
                }
            )
        );
    }

    auto help(std::string_view program_name) -> void {
        std::println("{} v{}", name, version);
        std::println("  {}\n", description);
        usage(program_name, stdout);
    }

    auto usage(std::string_view program_name, std::FILE* destination) -> void {
        std::println(destination, "Usage:");
        std::println(destination, "  {} (-h|--help)", program_name);

        (
            (std::println(
                destination,
                "  {} {} [<ARGS>]",
                program_name,
                Subcommands::name)),
            ...
        );
    }
};

} // namespace kh::argparse

#endif // ARGPARSE_H
