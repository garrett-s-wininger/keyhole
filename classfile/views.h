#ifndef VIEWS_H
#define VIEWS_H

#include "classfile.h"

namespace kh::jvm::views {
    struct ClassView {
        const kh::jvm::classfile::ClassFile& klass;

        explicit ClassView(const kh::jvm::classfile::ClassFile&);

        auto name() const -> std::string_view;
        auto superclass() const -> std::string_view;
    };

} // namespace kh::jvm::views

#endif // VIEWS_H
