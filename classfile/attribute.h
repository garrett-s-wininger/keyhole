#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H

#include <cstdint>
#include <span>
#include <vector>

namespace kh::jvm::attribute {

class Attribute {
private:
    std::vector<std::byte> owned_data_;
public:
    std::uint16_t name_index;
    std::span<const std::byte> data;

    Attribute(std::uint16_t, std::span<const std::byte>);
    Attribute(std::uint16_t, std::vector<std::byte>&&);

    Attribute(const Attribute&);
    Attribute(Attribute&&) noexcept;
    auto operator=(const Attribute&) -> Attribute&;
    auto operator=(Attribute&&) noexcept -> Attribute&;
};

struct CodeAttribute {
    std::uint16_t max_operand_stack_size;
    std::uint16_t max_local_variables;
    std::span<const std::byte> bytecode;
    std::span<const std::byte> exception_table;
    std::vector<Attribute> attributes;
};

} // namespace kh::jvm::attribute

#endif // ATTRIBUTE_H
