#ifndef VIEWS_H
#define VIEWS_H

#include "attribute.h"
#include "constant_pool.h"
#include "method.h"
#include "parsing.h"

namespace kh::jvm::views {

struct AttributeView {
  kh::jvm::constant_pool::ConstantPool &pool;
  kh::jvm::attribute::Attribute &attribute;

  AttributeView(kh::jvm::constant_pool::ConstantPool &pool,
                kh::jvm::attribute::Attribute &attribute);

  auto name() -> std::string_view;
  auto to_code() const -> std::expected<kh::jvm::attribute::CodeAttribute,
                                        kh::jvm::parsing::Error>;
};

struct MethodView {
  kh::jvm::constant_pool::ConstantPool &pool;
  kh::jvm::method::Method &method;

  MethodView(kh::jvm::constant_pool::ConstantPool &, kh::jvm::method::Method &);

  auto attribute(std::string_view) -> std::optional<AttributeView>;
  auto descriptor() -> std::string_view;
  auto name() -> std::string_view;
};

} // namespace kh::jvm::views

#endif // VIEWS_H
