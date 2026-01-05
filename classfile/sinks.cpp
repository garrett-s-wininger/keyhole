#include "sinks.h"

namespace kh::sinks {

FileSink::FileSink(std::ofstream& target) : target_(target) {}

auto FileSink::write_bytes(const std::span<const std::byte> bytes) -> void {
    target_.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

VectorSink::VectorSink() : buffer_(std::vector<std::byte>()) {}

VectorSink::VectorSink(std::vector<std::byte>& buffer) noexcept : buffer_(buffer) {}

auto VectorSink::view() const noexcept -> std::span<const std::byte> {
    return buffer_;
}

auto VectorSink::write_bytes(const std::span<const std::byte> bytes) -> void {
    buffer_.append_range(bytes);
}

} // namespace kh::sinks
