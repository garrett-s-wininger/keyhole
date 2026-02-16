#include <ranges>
#include <string_view>

#include "classfile.h"
#include "isa.h"
#include "parsing.h"
#include "views.h"

namespace kh::jvm::classfile {

ClassFile::ClassFile() noexcept
    : version{Version{55u, 0u}}
    , class_index{0u}
    , superclass_index{0u}
    , constant_pool(kh::jvm::constant_pool::ConstantPool{})
    , access_flags(0x0021)
    , methods(std::vector<kh::jvm::method::Method>{})
    , attributes(std::vector<kh::jvm::attribute::Attribute>{}) {}

auto
ClassFile::add_method(
        std::string_view name,
        std::string_view descriptor,
        const kh::jvm::isa::InstructionSequence& code)
        -> std::expected<void, Error> {
    using kh::jvm::method::AccessFlags;

    const auto method_search_result = method(name, descriptor);

    if (method_search_result) {
        return std::unexpected{Error::MethodExists};
    }

    auto code_attribute = kh::jvm::attribute::Attribute(
        constant_pool.try_add_utf8_entry("Code"),
        kh::jvm::isa::generate_code(code)
    );

    methods.push_back(
        kh::jvm::method::Method{
            .access_flags = static_cast<std::uint16_t>(
                AccessFlags::ACC_PUBLIC
                | AccessFlags::ACC_STATIC
                | AccessFlags::ACC_SYNTHETIC
            ),
            .name_index = static_cast<std::uint16_t>(
                constant_pool.try_add_utf8_entry(name)
            ),
            .descriptor_index = static_cast<std::uint16_t>(
                constant_pool.try_add_utf8_entry(descriptor)
            ),
            .attributes = std::vector<kh::jvm::attribute::Attribute>{
                std::move(code_attribute)
            }
        }
    );

    return {};
}

// NOTE(garrett): It may be preferable to return the method, regardless of
// descriptor, if there are no overloads. Should we find multiple, we can then
// return the one with the no-arg void return like we do now, and finally return
// an optional if we observe ambiguity or the method not being defined at all.
auto
ClassFile::method(std::string_view name)
        -> std::optional<std::reference_wrapper<kh::jvm::method::Method>> {
    return method(name, "()V");
}

auto
ClassFile::method(std::string_view name, std::string_view descriptor)
        -> std::optional<std::reference_wrapper<kh::jvm::method::Method>> {
    auto candidates = methods | std::views::filter(
        [this, name, descriptor](auto& method){
            auto view = kh::jvm::views::MethodView{constant_pool, method};
            return view.name() == name && view.descriptor() == descriptor;
        }
    );

    if (candidates.begin() == candidates.end()) {
        return std::nullopt;
    }

    return std::ref(*(candidates.begin()));
}

auto
ClassFile::name() -> std::string_view {
    const auto& class_entry = constant_pool.resolve<constant_pool::ClassEntry>(
        class_index
    );

    return constant_pool.resolve<kh::jvm::constant_pool::UTF8Entry>(
        class_entry.name_index
    ).text;
}

auto
ClassFile::prefix_method(
        std::string_view name,
        std::string_view descriptor,
        const kh::jvm::isa::InstructionSequence& instructions)
        -> std::expected<void, Error> {
    auto method_search_result = method(name, descriptor);

    if (!method_search_result) {
        return std::unexpected{Error::MethodDoesNotExist};
    }

    auto& m = method_search_result.value().get();

    auto attribute_search_result = kh::jvm::views::MethodView{
        constant_pool, m
    }.attribute("Code");

    if (!attribute_search_result) {
        return std::unexpected{Error::MethodBodyNotModifyable};
    }

    auto& attribute = attribute_search_result.value().attribute;
    auto code_view = kh::jvm::views::AttributeView{constant_pool, attribute};
    auto attribute_parse_result = code_view.to_code();

    if (!attribute_parse_result) {
        return std::unexpected{Error::MethodBodyNotParsable};
    }

    auto existing_code = attribute_parse_result.value();
    kh::jvm::isa::InstructionSequence merged_instructions;

    // NOTE(garrett): We'll need a more sophisticated method when instructions
    // have operands
    merged_instructions.reserve(
        instructions.size() + existing_code.bytecode.size()
    );

    merged_instructions.insert(
        merged_instructions.end(), instructions.begin(), instructions.end()
    );

    auto reader = kh::reader::Reader{existing_code.bytecode};

    if (auto parse_result = parsing::parse_bytecode(reader); parse_result) {
        merged_instructions.insert(
            merged_instructions.end(),
            parse_result.value().begin(),
            parse_result.value().end()
        );
    } else {
        return std::unexpected{Error::MethodBodyNotParsable};
    }

    return replace_method(name, descriptor, merged_instructions);
}

auto
ClassFile::rename(std::string_view new_name) -> void {
    const auto name_index = constant_pool.try_add_utf8_entry(new_name);
    auto& class_constant = constant_pool.resolve<
        kh::jvm::constant_pool::ClassEntry
    >(class_index);


    class_constant.name_index = name_index;
}

// NOTE(garrett): Perhaps it may be better in this implementation to create the
// function if it doesn't exist (or we could do a separate function).
auto
ClassFile::replace_method(
        std::string_view name,
        std::string_view descriptor,
        const kh::jvm::isa::InstructionSequence& new_code)
        -> std::expected<void, Error> {
    const auto method_search_result = method(name, descriptor);

    if (!method_search_result) {
        return std::unexpected{Error::MethodDoesNotExist};
    }

    auto& method = method_search_result.value().get();

    const auto attribute_search_result = kh::jvm::views::MethodView{
        constant_pool, method
    }.attribute("Code");

    if (!attribute_search_result) {
        return std::unexpected{Error::MethodBodyNotModifyable};
    }

    auto& attribute = attribute_search_result.value().attribute;
    const auto view = kh::jvm::views::AttributeView{constant_pool, attribute};
    const auto existing_code_result = view.to_code();

    if (!existing_code_result) {
        return std::unexpected{Error::MethodBodyNotParsable};
    }

    const auto existing_code = existing_code_result.value();

    auto new_code_attribute = kh::jvm::attribute::Attribute(
        constant_pool.try_add_utf8_entry("Code"),
        kh::jvm::isa::generate_code(
            existing_code.max_operand_stack_size,
            existing_code.max_local_variables,
            new_code
        )
    );

    auto it = std::find_if(
        method.attributes.begin(),
        method.attributes.end(),
        [&attribute](const auto& attrib) {
            return std::addressof(attrib) == std::addressof(attribute);
        }
    );

    if (it != method.attributes.end()) {
        *it = std::move(new_code_attribute);
    }

    return {};
}

auto
ClassFile::superclass() -> std::string_view {
    const auto& class_entry = constant_pool.resolve<
            kh::jvm::constant_pool::ClassEntry>(
        superclass_index
    );

    return constant_pool.resolve<kh::jvm::constant_pool::UTF8Entry>(
        class_entry.name_index
    ).text;
}

} // namespace kh::jvm::classfile
