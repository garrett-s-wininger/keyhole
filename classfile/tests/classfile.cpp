#include "gtest/gtest.h"

#include "classfile.h"
#include "isa.h"
#include "tests/helpers.h"
#include "views.h"

using namespace std::literals;

namespace kh::jvm::classfile {

TEST(ClassFile, AddsMethodAppropriately) {
    auto klass = ClassFile{};
    constexpr auto name = "myMethod"sv;
    constexpr auto descriptor = "()V"sv;

    EXPECT_FALSE(klass.method(name, descriptor));

    constexpr auto code = kh::jvm::isa::InstructionSequence{};
    const auto result = klass.add_method(name, descriptor, code);
    EXPECT_TRUE(result.has_value());

    const auto method_opt = klass.method(name, descriptor);
    EXPECT_TRUE(method_opt.has_value());
}

TEST(ClassFile, AddMethodFailsIfAlreadyExists) {
    auto klass = ClassFile{};
    constexpr auto code = kh::jvm::isa::InstructionSequence{};
    constexpr auto name = "duplicate"sv;
    constexpr auto descriptor = "()V";

    const auto initial_insert = klass.add_method(name, descriptor, code);
    EXPECT_TRUE(initial_insert.has_value());

    const auto duplicate_insert = klass.add_method(name, descriptor, code);
    EXPECT_FALSE(duplicate_insert.has_value());
    ASSERT_TRUE(duplicate_insert.error() == Error::MethodExists);
}

TEST(ClassFile, ReplaceMethodReplacesExistingCode) {
    auto klass = ClassFile{};
    constexpr auto name = "replaceMe"sv;
    constexpr auto descriptor = "()V"sv;

    const auto original_code = kh::jvm::isa::InstructionSequence{
        kh::jvm::isa::Return
    };

    std::ignore = klass.add_method(name, descriptor, original_code);
    const auto new_code = kh::jvm::isa::InstructionSequence{
        kh::jvm::isa::Nop, kh::jvm::isa::Return
    };

    const auto replace_result = klass.replace_method(
        name, descriptor, new_code
    );

    ASSERT_TRUE(replace_result);

    const auto method_search_result = klass.method(name, descriptor);
    ASSERT_TRUE(method_search_result.has_value());

    auto& method = method_search_result.value().get();
    const auto code_search_result = kh::jvm::views::MethodView{
        klass.constant_pool, method
    }.attribute("Code");

    ASSERT_TRUE(code_search_result);
    const auto replaced_code_result = code_search_result.value().to_code();

    ASSERT_TRUE(replaced_code_result);

    constexpr auto expected_code = std::array<const std::byte, 2>{
        static_cast<std::byte>(kh::jvm::isa::Nop.opcode),
        static_cast<std::byte>(kh::jvm::isa::Return.opcode)
    };

    EXPECT_THAT(
        expected_code,
        EqualsBinary(replaced_code_result.value().bytecode)
    );
}

TEST(ClassFile, ReplaceMethodFailsIfNonExistent) {
    auto klass = ClassFile{};

    const auto new_code = kh::jvm::isa::InstructionSequence{
        kh::jvm::isa::Return
    };

    const auto result = klass.replace_method("noSuchMethod", "()V", new_code);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), Error::MethodDoesNotExist);
}

TEST(ClassFile, PrefixMethodPrependsInstructions) {
    auto klass = ClassFile{};
    constexpr auto name = "prefixed"sv;
    constexpr auto descriptor = "()V"sv;

    const auto code = kh::jvm::isa::InstructionSequence{
        kh::jvm::isa::Return
    };

    std::ignore = klass.add_method(name, descriptor, code);

    const auto prefix_code = kh::jvm::isa::InstructionSequence{
        kh::jvm::isa::Nop
    };

    const auto result = klass.prefix_method(name, descriptor, prefix_code);
    ASSERT_TRUE(result);

    const auto method_search_result = klass.method(name, descriptor);
    ASSERT_TRUE(method_search_result.has_value());

    auto& method = method_search_result.value().get();

    const auto code_search_result = kh::jvm::views::MethodView{
        klass.constant_pool, method
    }.attribute("Code");

    ASSERT_TRUE(code_search_result);
    const auto parse_result = code_search_result.value().to_code();
    ASSERT_TRUE(parse_result);

    constexpr auto expected_code = std::array<const std::byte, 2>{
        static_cast<std::byte>(kh::jvm::isa::Nop.opcode),
        static_cast<std::byte>(kh::jvm::isa::Return.opcode)
    };

    EXPECT_THAT(expected_code, EqualsBinary(parse_result.value().bytecode));
}

TEST(ClassFile, PrefixMethodFailsIfNonExistent) {
    auto klass = ClassFile{};

    const auto prefix_code = kh::jvm::isa::InstructionSequence{
        kh::jvm::isa::Nop
    };

    const auto result = klass.prefix_method("doesntExist", "()V", prefix_code);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), Error::MethodDoesNotExist);
}

} // kh::jvm::classfile
