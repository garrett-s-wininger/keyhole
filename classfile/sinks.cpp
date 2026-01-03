#include "sinks.h"

namespace kh::sinks {

FileSink::FileSink(std::ofstream& target) : target_(target) {}

VectorSink::VectorSink() : buffer_(std::vector<std::byte>()) {}

VectorSink::VectorSink(std::vector<std::byte>& buffer) noexcept : buffer_(buffer) {}

auto VectorSink::view() const noexcept -> std::span<const std::byte> {
    return buffer_;
}

} // namespace kh::sinks
