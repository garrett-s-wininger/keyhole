#ifndef PARSING_H
#define PARSING_H

#include <expected>
#include <filesystem>

#include "attribute.h"
#include "classfile.h"
#include "constant_pool.h"
#include "method.h"
#include "reader.h"

namespace parsing {

enum Error {
    InvalidConstantPoolTag,
    InvalidMagic,
    NotImplemented,
    Truncated
};

struct LoadedClass {
    std::vector<std::byte> raw;
    classfile::ClassFile class_file;
};

auto load_class_from_file(const std::filesystem::path& path)
        -> std::expected<LoadedClass, Error>;

auto parse_attribute(reader::Reader&) noexcept
        -> std::expected<attribute::Attribute, Error>;

auto parse_class_file(reader::Reader&)
        -> std::expected<classfile::ClassFile, Error>;

auto parse_constant_pool(reader::Reader&, uint16_t count)
        -> std::expected<constant_pool::ConstantPool, Error>;

auto parse_constant_pool_entry(reader::Reader&) noexcept
        -> std::expected<constant_pool::Entry, Error>;

auto parse_method(reader::Reader&) -> std::expected<method::Method, Error>;

}

#endif // PARSING_H
