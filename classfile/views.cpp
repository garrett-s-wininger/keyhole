#include <ranges>

#include "views.h"

namespace kh::jvm::views {

AttributeView::AttributeView(
        const kh::jvm::constant_pool::ConstantPool& pool,
        const kh::jvm::attribute::Attribute& attribute)
        : pool(pool), attribute(attribute) {}

auto AttributeView::name() const -> std::string_view {
    return pool.resolve<kh::jvm::constant_pool::UTF8Entry>(
        attribute.name_index
    ).text;
}

MethodView::MethodView(
        const kh::jvm::constant_pool::ConstantPool& pool,
        const kh::jvm::method::Method& method) : pool(pool), method(method) {}

auto MethodView::attribute(std::string_view name) const -> std::optional<AttributeView> {
    auto candidates = method.attributes | std::views::filter(
        [this, name](const auto& attribute){
            return AttributeView{pool, attribute}.name() == name;
        }
    );

    if (candidates.begin() == candidates.end()) {
        return std::nullopt;
    }

    return AttributeView{pool, *(candidates.begin())};
}

auto MethodView::descriptor() const -> std::string_view {
    return pool.resolve<kh::jvm::constant_pool::UTF8Entry>(
        method.descriptor_index
    ).text;
}

auto MethodView::name() const -> std::string_view {
    return pool.resolve<kh::jvm::constant_pool::UTF8Entry>(
        method.name_index
    ).text;
}

ClassView::ClassView(const kh::jvm::classfile::ClassFile& klass) : klass(klass) {};

// NOTE(garrett): It may be preferable to return the method, regardless of
// descriptor, if there are no overloads. Should we find multiple, we can then
// return the one with the no-arg void return like we do now, and finally return
// an optional if we observe ambiguity or the method not being defined at all.
auto ClassView::method(std::string_view name) const -> std::optional<MethodView> {
    return method(name, "()V");
}

auto ClassView::method(std::string_view name, std::string_view descriptor) const
        -> std::optional<MethodView> {
    auto candidates = klass.methods | std::views::filter(
        [this, name, descriptor](const auto& method){
            const auto view = MethodView{klass.constant_pool, method};
            return view.name() == name && view.descriptor() == descriptor;
        }
    );

    if (candidates.begin() == candidates.end()) {
        return std::nullopt;
    }

    return MethodView{klass.constant_pool, *(candidates.begin())};
}

auto ClassView::name() const -> std::string_view {
    const auto& class_entry = klass.constant_pool.resolve<constant_pool::ClassEntry>(
        klass.class_index
    );

    return klass.constant_pool.resolve<kh::jvm::constant_pool::UTF8Entry>(
        class_entry.name_index
    ).text;
}

auto ClassView::superclass() const -> std::string_view {
    const auto& class_entry = klass.constant_pool.resolve<
            kh::jvm::constant_pool::ClassEntry>(
        klass.superclass_index
    );

    return klass.constant_pool.resolve<kh::jvm::constant_pool::UTF8Entry>(
        class_entry.name_index
    ).text;
}

} // namespace kh::jvm::views
