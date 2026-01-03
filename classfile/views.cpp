#include "views.h"

namespace kh::jvm::views {

ClassView::ClassView(const kh::jvm::classfile::ClassFile& klass) : klass(klass) {};

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

MethodView::MethodView(
        const kh::jvm::constant_pool::ConstantPool& pool,
        const kh::jvm::method::Method& method) : pool(pool), method(method) {}

auto MethodView::name() const -> std::string_view {
    return pool.resolve<kh::jvm::constant_pool::UTF8Entry>(
        method.name_index
    ).text;
}

} // namespace kh::jvm::views
