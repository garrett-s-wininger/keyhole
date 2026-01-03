#include "classfile.h"

namespace kh::jvm::classfile {

ClassFile::ClassFile() noexcept
    : version{Version{55u, 0u}}
    , class_index{0u}
    , superclass_index{0u}
    , constant_pool(kh::jvm::constant_pool::ConstantPool{})
    , access_flags(0x0021)
    , methods(std::vector<kh::jvm::method::Method>{})
    , attributes(std::vector<kh::jvm::attribute::Attribute>{}) {}

} // namespace kh::jvm::classfile
