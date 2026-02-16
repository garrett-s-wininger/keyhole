#include <ranges>

#include "views.h"

namespace kh::jvm::views {

AttributeView::AttributeView(
        kh::jvm::constant_pool::ConstantPool& pool,
        kh::jvm::attribute::Attribute& attribute)
        : pool(pool), attribute(attribute) {}

auto AttributeView::name() -> std::string_view {
    return pool.resolve<kh::jvm::constant_pool::UTF8Entry>(
        attribute.name_index
    ).text;
}

auto AttributeView::to_code() const
        -> std::expected<kh::jvm::attribute::CodeAttribute, kh::jvm::parsing::Error> {
    auto reader = kh::reader::Reader{attribute.data};
    return parsing::parse_code_attribute(reader);
}

MethodView::MethodView(
        kh::jvm::constant_pool::ConstantPool& pool,
        kh::jvm::method::Method& method) : pool(pool), method(method) {}

auto MethodView::attribute(std::string_view name) -> std::optional<AttributeView> {
    auto candidates = method.attributes | std::views::filter(
        [this, name](auto& attribute){
            return AttributeView{pool, attribute}.name() == name;
        }
    );

    if (candidates.begin() == candidates.end()) {
        return std::nullopt;
    }

    return AttributeView{pool, *(candidates.begin())};
}

auto MethodView::descriptor() -> std::string_view {
    return pool.resolve<kh::jvm::constant_pool::UTF8Entry>(
        method.descriptor_index
    ).text;
}

auto MethodView::name() -> std::string_view {
    return pool.resolve<kh::jvm::constant_pool::UTF8Entry>(
        method.name_index
    ).text;
}

} // namespace kh::jvm::views
