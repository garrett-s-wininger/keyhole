#include <stdexcept>
#include <fstream>
#include <vector>

#include "parsing.h"
#include "isa.h"

namespace kh::jvm::parsing {

auto
load_class_from_file(const std::filesystem::path& path)
        -> std::expected<LoadedClass, Error> {
    auto file_reader = std::ifstream{path, std::ios::binary | std::ios::ate};

    if (!file_reader) {
        throw std::runtime_error(
            std::format("Failed to access file ({})", path.string())
        );
    }

    const auto size = file_reader.tellg();
    file_reader.seekg(0, std::ios::beg);

    auto contents = std::vector<std::byte>{};
    contents.resize(size);

    if (!file_reader.read(reinterpret_cast<char*>(contents.data()), size)) {
        throw std::runtime_error(
            std::format("Failed to read file ({}) contents", path.string())
        );
    }

    auto reader = kh::reader::Reader{contents};
    const auto class_file = parse_class_file(reader);

    if (!class_file) {
        return std::unexpected(class_file.error());
    }

    return LoadedClass{std::move(contents), class_file.value()};
}

auto
parse_attribute(reader::Reader& reader) noexcept
        -> std::expected<attribute::Attribute, Error> {
    const auto header = reader.read_bytes(sizeof(std::uint32_t) + sizeof(std::uint16_t));

    if (!header) {
        return std::unexpected(Error::Truncated);
    }

    reader::Reader header_reader{header.value()};

    const auto name_index = header_reader.read_unchecked<std::uint16_t>();
    const auto body_size = header_reader.read_unchecked<std::uint32_t>();

    const auto body = reader.read_bytes(body_size);

    if (!body) {
        return std::unexpected(Error::Truncated);
    }

    return kh::jvm::attribute::Attribute{
        name_index,
        body.value()
    };
}

auto parse_bytecode(kh::reader::Reader& reader)
        -> std::expected<isa::InstructionSequence, Error> {
    auto result = isa::InstructionSequence{};

    // NOTE(garrett): For single-byte reads, we only support a single return
    // value so don't need to worry about the error type specifically.
    while (const auto byte = reader.read_byte()) {
        // TODO(garrett): Support other instruction forms
        result.push_back(
            isa::NoOperandInstruction{static_cast<isa::Opcode>(byte.value())}
        );
    }

    return result;
}

auto parse_method(kh::reader::Reader& reader)
        -> std::expected<method::Method, Error> {
    const auto method_header = reader.read_bytes(sizeof(std::uint64_t));

    if (!method_header) {
        return std::unexpected(Error::Truncated);
    }

    reader::Reader method_reader{method_header.value()};

    const auto access_flags = method_reader.read_unchecked<std::uint16_t>();
    const auto name_index = method_reader.read_unchecked<std::uint16_t>();
    const auto descriptor_index = method_reader.read_unchecked<std::uint16_t>();
    const auto attribute_count = method_reader.read_unchecked<std::uint16_t>();

    std::vector<kh::jvm::attribute::Attribute> attributes{};
    attributes.reserve(attribute_count);

    for (auto i = 0uz; i < attribute_count; ++i) {
        const auto result = parse_attribute(reader);

        if (!result) {
            return std::unexpected(Error::Truncated);
        }

        attributes.push_back(std::move(result.value()));
    }

    return kh::jvm::method::Method{
        access_flags,
        name_index,
        descriptor_index,
        std::move(attributes)
    };
}

auto parse_class_info_entry(kh::reader::Reader& reader) noexcept
        -> std::expected<kh::jvm::constant_pool::ClassEntry, Error> {
    const auto index = reader.read<std::uint16_t>();

    if (!index) {
        return std::unexpected(Error::Truncated);
    }

    return kh::jvm::constant_pool::ClassEntry{index.value()};
}

auto parse_code_attribute(kh::reader::Reader& reader) noexcept
        -> std::expected<kh::jvm::attribute::CodeAttribute, Error> {
    auto metadata_result = reader.read_bytes(sizeof(std::uint64_t));

    if (!metadata_result) {
        return std::unexpected{Error::Truncated};
    }

    auto metadata_reader = kh::reader::Reader{metadata_result.value()};
    const auto max_stack = metadata_reader.read_unchecked<std::uint16_t>();
    const auto max_locals = metadata_reader.read_unchecked<std::uint16_t>();
    const auto bytecode_size = metadata_reader.read_unchecked<std::uint32_t>();

    const auto bytecode_result = reader.read_bytes(bytecode_size);

    if (!bytecode_result) {
        return std::unexpected{Error::Truncated};
    }

    const auto exception_table_size_result = reader.read<std::uint16_t>();

    if (!exception_table_size_result) {
        return std::unexpected{Error::Truncated};
    }

    const auto exception_table_result = reader.read_bytes(
        exception_table_size_result.value()
    );

    if (!exception_table_result) {
        return std::unexpected{Error::Truncated};
    }

    const auto attribute_count_result = reader.read<std::uint16_t>();

    if (!attribute_count_result) {
        return std::unexpected{Error::Truncated};
    }

    const auto attribute_count = attribute_count_result.value();
    auto attributes = std::vector<kh::jvm::attribute::Attribute>{};
    attributes.reserve(attribute_count);

    for (auto i = 0uz; i < attribute_count; ++i) {
        const auto attribute = parse_attribute(reader);

        if (!attribute) {
            return std::unexpected{Error::Truncated};
        }

        attributes.push_back(std::move(attribute.value()));
    }

    return kh::jvm::attribute::CodeAttribute{
        .max_operand_stack_size = max_stack,
        .max_local_variables = max_locals,
        .bytecode = bytecode_result.value(),
        .exception_table = exception_table_result.value(),
        .attributes = std::move(attributes)
    };
}

// TODO(garrett): Perhaps collapse these (any maybe others) to a generic
// x-bytes parse
auto parse_method_reference_entry(kh::reader::Reader& reader) noexcept
        -> std::expected<kh::jvm::constant_pool::MethodReferenceEntry, Error> {
    const auto entry_contents = reader.read_bytes(sizeof(std::uint32_t));

    if (!entry_contents) {
        return std::unexpected(Error::Truncated);
    }

    kh::reader::Reader entry_reader{entry_contents.value()};

    return kh::jvm::constant_pool::MethodReferenceEntry{
        entry_reader.read_unchecked<std::uint16_t>(),
        entry_reader.read_unchecked<std::uint16_t>()
    };
}

auto parse_name_and_type_entry(kh::reader::Reader& reader) noexcept
        -> std::expected<kh::jvm::constant_pool::NameAndTypeEntry, Error> {
    const auto entry_contents = reader.read_bytes(sizeof(std::uint32_t));

    if (!entry_contents) {
        return std::unexpected(Error::Truncated);
    }

    kh::reader::Reader entry_reader{entry_contents.value()};

    return kh::jvm::constant_pool::NameAndTypeEntry{
        entry_reader.read_unchecked<std::uint16_t>(),
        entry_reader.read_unchecked<std::uint16_t>()
    };
}

auto parse_utf8_entry(kh::reader::Reader& reader) noexcept
        -> std::expected<kh::jvm::constant_pool::UTF8Entry, Error> {
    auto entry = kh::jvm::constant_pool::UTF8Entry{};
    const auto size = reader.read<std::uint16_t>();

    if (!size) {
        return std::unexpected(Error::Truncated);
    }

    const auto text_content = reader.read_bytes(size.value());

    if (!text_content) {
        return std::unexpected(Error::Truncated);
    }

    entry.text = std::string_view{
        reinterpret_cast<const char*>(text_content.value().data()),
        size.value()
    };

    return entry;
}

auto parse_constant_pool_entry(kh::reader::Reader& reader) noexcept
        -> std::expected<kh::jvm::constant_pool::Entry, Error> {
    const auto tag = reader.read<std::uint8_t>();

    if (!tag) {
        return std::unexpected(Error::Truncated);
    }

    switch (static_cast<kh::jvm::constant_pool::Tag>(tag.value())) {
        case kh::jvm::constant_pool::Tag::Class: {
            return parse_class_info_entry(reader);
        }
        case kh::jvm::constant_pool::Tag::MethodReference: {
            return parse_method_reference_entry(reader);
        }
        case kh::jvm::constant_pool::Tag::NameAndType: {
            return parse_name_and_type_entry(reader);
        }
        case kh::jvm::constant_pool::Tag::UTF8: {
            return parse_utf8_entry(reader);
        }
        default:
            return std::unexpected(Error::InvalidConstantPoolTag);
    }
}

auto parse_constant_pool(kh::reader::Reader& reader, std::uint16_t count)
        -> std::expected<kh::jvm::constant_pool::ConstantPool, Error> {
    kh::jvm::constant_pool::ConstantPool pool{};

    for (auto i = 0uz; i < count; ++i) {
        const auto entry = parse_constant_pool_entry(reader);

        if (!entry) {
            return std::unexpected(Error::Truncated);
        }

        pool.add(entry.value());
    }

    return pool;
}

auto parse_class_file(kh::reader::Reader& reader)
        -> std::expected<kh::jvm::classfile::ClassFile, Error> {
    const auto header = reader.read_bytes(sizeof(std::uint64_t) + sizeof(std::uint16_t));

    if (!header) {
        return std::unexpected(Truncated);
    }

    auto header_reader = kh::reader::Reader{header.value()};

    if (header_reader.read_unchecked<std::uint32_t>() != 0xCAFEBABE) {
        return std::unexpected(Error::InvalidMagic);
    }

    const auto minor = header_reader.read_unchecked<std::uint16_t>();
    const auto major = header_reader.read_unchecked<std::uint16_t>();

    auto result = kh::jvm::classfile::ClassFile{};
    result.version = kh::jvm::classfile::Version{major, minor};

    const auto pool = parse_constant_pool(
        reader,
        // NOTE(garrett): Classfile contains the actual count, plus one
        header_reader.read_unchecked<std::uint16_t>() - 1
    );

    if (!pool) {
        return std::unexpected(Error::Truncated);
    }

    result.constant_pool = pool.value();

    const auto access_and_class_metadata = reader.read_bytes(sizeof(std::uint64_t));

    if (!access_and_class_metadata) {
        return std::unexpected(Error::Truncated);
    }

    auto metadata_reader = kh::reader::Reader{access_and_class_metadata.value()};
    result.access_flags = metadata_reader.read_unchecked<std::uint16_t>();
    result.class_index = metadata_reader.read_unchecked<std::uint16_t>();
    result.superclass_index = metadata_reader.read_unchecked<std::uint16_t>();

    const auto interface_count = metadata_reader.read_unchecked<std::uint16_t>();

    // TODO(garrett): Interface parsing
    if (interface_count > 0) {
        return std::unexpected(Error::NotImplemented);
    }

    const auto fields_count = reader.read<std::uint16_t>();

    // TODO(garrett): Field parsing
    if (!fields_count) {
        return std::unexpected(Error::Truncated);
    } else if (fields_count.value() > 0) {
        return std::unexpected(Error::NotImplemented);
    }

    const auto methods_count = reader.read<std::uint16_t>();

    if (!methods_count) {
        return std::unexpected(Error::Truncated);
    }

    result.methods = std::vector<method::Method>{};
    result.methods.reserve(methods_count.value());

    for (auto i = 0u; i < methods_count.value(); ++i) {
        const auto method = parse_method(reader);

        if (!method) {
            return std::unexpected(Error::Truncated);
        }

        result.methods.push_back(method.value());
    }

    const auto attributes_count = reader.read<std::uint16_t>();

    if (!attributes_count) {
        return std::unexpected(Error::Truncated);
    }

    result.attributes = std::vector<kh::jvm::attribute::Attribute>{};

    for (auto i = 0uz; i < attributes_count.value(); ++i) {
        const auto attribute = parse_attribute(reader);

        if (!attribute) {
            return std::unexpected(Error::Truncated);
        }

        result.attributes.push_back(std::move(attribute.value()));
    }

    return result;
}

} // namespace kh::jvm::parsing
