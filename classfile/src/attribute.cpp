#include "attribute.h"

namespace kh::jvm::attribute {

Attribute::Attribute(std::uint16_t name_index, std::span<const std::byte> data)
        : owned_data_({}), name_index(name_index), data(data) {}

Attribute::Attribute(std::uint16_t name_index, std::vector<std::byte>&& data)
        : owned_data_(std::move(data)), name_index(name_index),
        data(owned_data_) {}

Attribute::Attribute(const Attribute& other)
        : name_index(other.name_index) {
    if (!other.owned_data_.empty()) {
        owned_data_ = other.owned_data_;
    } else if (!other.data.empty()) {
        owned_data_.assign(other.data.begin(), other.data.end());
    }

    data = owned_data_;
}

Attribute::Attribute(Attribute&& other) noexcept
        : name_index(other.name_index) {
    if (!other.owned_data_.empty()) {
        owned_data_ = std::move(other.owned_data_);
    } else if (!other.data.empty()) {
        owned_data_.assign(other.data.begin(), other.data.end());
    }

    data = owned_data_;
}

auto Attribute::operator=(const Attribute& other) -> Attribute& {
    if (this != &other) {
        name_index = other.name_index;

        if (!other.owned_data_.empty()) {
            owned_data_ = other.owned_data_;
        } else if (!other.data.empty()) {
            owned_data_.assign(other.data.begin(), other.data.end());
        } else {
            owned_data_.clear();
        }

        data = owned_data_;
    }

    return *this;
}

auto Attribute::operator=(Attribute&& other) noexcept -> Attribute& {
    if (this != &other) {
        name_index = other.name_index;

        if (!other.owned_data_.empty()) {
            owned_data_ = std::move(other.owned_data_);
        } else if (!other.data.empty()) {
            owned_data_.assign(other.data.begin(), other.data.end());
        } else {
            owned_data_.clear();
        }

        data = owned_data_;
    }

    return *this;
}

} // kh::jvm::attribute
