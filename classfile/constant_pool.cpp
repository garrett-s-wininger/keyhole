#include "constant_pool.h"

namespace kh::jvm::constant_pool {

ConstantPool::ConstantPool()
        : entries_(std::deque<Entry>{})
        , resolution_table_(std::vector<std::optional<std::size_t>>{})
        , text_entries_(std::unordered_map<std::string_view, std::size_t>{}) {
    // NOTE(garrett): Index zero is reserved, access should be 1-indexed so we
    // can grab data directly from other classfile references
    resolution_table_.push_back(std::nullopt);
}

ConstantPool::ConstantPool(std::initializer_list<Entry> entries)
        : ConstantPool::ConstantPool() {
    for (const auto& entry : entries) {
        add(entry);
    }
}

auto ConstantPool::add(const Entry entry) -> std::size_t {
    resolution_table_.push_back(entries_.size());
    entries_.push_back(entry);

    const auto resolution_index = resolution_table_.size() - 1;

    if (std::holds_alternative<UTF8Entry>(entry)) {
        const auto text_entry = std::get<UTF8Entry>(entry);
        text_entries_.try_emplace(text_entry.text, resolution_index);
    }

    return resolution_index;
}

auto ConstantPool::entries() const -> const std::deque<Entry>& {
    return entries_;
}

auto ConstantPool::try_add_utf8_entry(std::string_view text) -> std::size_t {
    const auto search_result = text_entries_.find(text);

    if (search_result != text_entries_.end()) {
        return search_result->second;
    }

    return add(UTF8Entry{text});
}

} // namespace kh::jvm::constant_pool
