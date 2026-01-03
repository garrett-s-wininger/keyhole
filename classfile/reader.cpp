#include "reader.h"

namespace kh::reader {

Reader::Reader(std::span<const std::byte> bytes) noexcept : remaining_(bytes) {}

auto Reader::read_bytes(std::uint32_t count)
        -> std::expected<std::span<const std::byte>, Error> {
    if (remaining_.size() < count) {
        return std::unexpected(Error::Truncated);
    }

    const auto result = remaining_.first(count);
    remaining_ = remaining_.subspan(count);

    return result;
}

} // namespace kh::reader
