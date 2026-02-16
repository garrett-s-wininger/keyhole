#ifndef CLASSFILE_H
#define CLASSFILE_H

#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <string_view>
#include <vector>

#include "attribute.h"
#include "constant_pool.h"
#include "isa.h"
#include "method.h"

namespace kh::jvm::classfile {

struct Version {
    std::uint16_t major;
    std::uint16_t minor;
};

enum class AccessFlags : uint16_t {
    ACC_PUBLIC = 0x0001,
    ACC_FINAL = 0x0010,
    ACC_SUPER = 0x0020,
    ACC_INTERFACE = 0x0200,
    ACC_ABSTRACT = 0x0400,
    ACC_SYNTHETIC = 0x1000,
    ACC_ANNOTATION = 0x2000,
    ACC_ENUM = 0x4000,
    ACC_MODULE = 0x8000
};

template <typename T>
concept PersistentString =
    std::same_as<std::remove_reference_t<T>, const std::string> &&
    std::is_lvalue_reference_v<T&&>;

enum Error {
    MethodBodyNotParsable,
    MethodBodyNotModifyable,
    MethodDoesNotExist,
    MethodExists,
};

struct ClassFile {
    Version version;
    std::uint16_t class_index;
    std::uint16_t superclass_index;
    kh::jvm::constant_pool::ConstantPool constant_pool;
    std::uint16_t access_flags;
    // TODO(garrett): Interface, field storage
    std::vector<kh::jvm::method::Method> methods;
    std::vector<kh::jvm::attribute::Attribute> attributes;

    ClassFile() noexcept;

    template <PersistentString S1, PersistentString S2>
    ClassFile(S1&& class_name, S2&& superclass_name)
            : ClassFile() {
        constant_pool.add(
            kh::jvm::constant_pool::UTF8Entry{class_name}
        );

        constant_pool.add(kh::jvm::constant_pool::ClassEntry{1});
        class_index = 2;

        constant_pool.add(
            kh::jvm::constant_pool::UTF8Entry{superclass_name}
        );

        constant_pool.add(kh::jvm::constant_pool::ClassEntry{3});
        superclass_index = 4;
    }

    auto add_method(
            std::string_view,
            std::string_view,
            const kh::jvm::isa::InstructionSequence&)
            -> std::expected<void, Error>;

    auto method(std::string_view)
        -> std::optional<std::reference_wrapper<kh::jvm::method::Method>>;

    auto method(std::string_view, std::string_view)
        -> std::optional<std::reference_wrapper<kh::jvm::method::Method>>;

    auto name() -> std::string_view;

    auto prefix_method(
        std::string_view,
        std::string_view,
        const kh::jvm::isa::InstructionSequence&) -> std::expected<void, Error>;

    auto rename(std::string_view) -> void;

    auto replace_method(
            std::string_view,
            std::string_view,
            const kh::jvm::isa::InstructionSequence&)
            -> std::expected<void, Error>;

    auto superclass() -> std::string_view;
};

} // namespace kh::jvm::classfile

#endif // CLASSFILE_H
