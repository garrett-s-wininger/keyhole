#include "classfile.h"

namespace classfile {

ClassFile::ClassFile() noexcept
    : version{Version{55u, 0u}}
    , class_index{0u}
    , superclass_index{0u}
    , constant_pool(constant_pool::ConstantPool{})
    , access_flags(0x0021)
    , methods(std::vector<method::Method>{})
    , attributes(std::vector<attribute::Attribute>{}) {}

auto ClassFile::name() const -> std::string_view {
    const auto& class_entry = constant_pool.resolve<constant_pool::ClassEntry>(
        class_index
    );

    return constant_pool.resolve<constant_pool::UTF8Entry>(
        class_entry.name_index
    ).text;
}

auto ClassFile::superclass() const -> std::string_view {
    const auto& class_entry = constant_pool.resolve<constant_pool::ClassEntry>(
        superclass_index
    );

    return constant_pool.resolve<constant_pool::UTF8Entry>(
        class_entry.name_index
    ).text;
}

} // namespace classfile
