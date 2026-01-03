#ifndef PARSING_H
#define PARSING_H

#include <expected>
#include <filesystem>

#include "attribute.h"
#include "classfile.h"
#include "constant_pool.h"
#include "method.h"
#include "reader.h"

namespace kh::jvm::parsing {

enum Error {
    InvalidConstantPoolTag,
    InvalidMagic,
    NotImplemented,
    Truncated
};

struct LoadedClass {
    std::vector<std::byte> raw;
    kh::jvm::classfile::ClassFile class_file;
};

auto load_class_from_file(const std::filesystem::path& path)
        -> std::expected<LoadedClass, Error>;

auto parse_attribute(kh::reader::Reader&) noexcept
        -> std::expected<attribute::Attribute, Error>;

auto parse_class_file(kh::reader::Reader&)
        -> std::expected<classfile::ClassFile, Error>;

auto parse_constant_pool(kh::reader::Reader&, uint16_t count)
        -> std::expected<constant_pool::ConstantPool, Error>;

auto parse_constant_pool_entry(kh::reader::Reader&) noexcept
        -> std::expected<constant_pool::Entry, Error>;

auto parse_method(kh::reader::Reader&)
        -> std::expected<kh::jvm::method::Method, Error>;

} // namespace kh::jvm::parsing

#endif // PARSING_H
