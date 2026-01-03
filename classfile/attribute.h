#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H

#include <cstdint>
#include <span>

namespace kh::jvm::attribute {

struct Attribute {
    uint16_t name_index;
    std::span<const std::byte> data;
};

} // namespace kh::jvm::attribute

#endif // ATTRIBUTE_H
