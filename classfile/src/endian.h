#ifndef ENDIAN_H
#define ENDIAN_H

#include <bit>
#include <concepts>
#include <cstdint>

namespace kh::endian {

template <typename T>
concept MultiByteIntegral =
    std::same_as<T, std::uint8_t> || std::same_as<T, std::uint16_t> ||
    std::same_as<T, std::uint32_t>;

template <MultiByteIntegral V> auto big(V value) {
  if constexpr (std::same_as<V, std::uint8_t>) {
    return value;
  } else {
    V result = value;

    if constexpr (std::endian::native == std::endian::big) {
      return result;
    } else {
      return std::byteswap(result);
    }
  }
}

} // namespace kh::endian

#endif // ENDIAN_H
