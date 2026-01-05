#ifndef VIEWS_H
#define VIEWS_H

#include "classfile.h"

namespace kh::jvm::views {
    struct AttributeView {
        const kh::jvm::constant_pool::ConstantPool& pool;
        const kh::jvm::attribute::Attribute& attribute;

        AttributeView(
            const kh::jvm::constant_pool::ConstantPool& pool,
            const kh::jvm::attribute::Attribute& attribute
        );

        auto name() const -> std::string_view;
    };

    struct MethodView {
        const kh::jvm::constant_pool::ConstantPool& pool;
        const kh::jvm::method::Method& method;

        MethodView(
            const kh::jvm::constant_pool::ConstantPool&,
            const kh::jvm::method::Method&
        );

        auto name() const -> std::string_view;
        auto attribute(std::string_view) const -> std::optional<AttributeView>;
    };

    struct ClassView {
        const kh::jvm::classfile::ClassFile& klass;

        explicit ClassView(const kh::jvm::classfile::ClassFile&);

        // TODO(garrett): Also include descriptor to handle overloads
        auto method(std::string_view) const -> std::optional<MethodView>;
        auto name() const -> std::string_view;
        auto superclass() const -> std::string_view;
    };
} // namespace kh::jvm::views

#endif // VIEWS_H
