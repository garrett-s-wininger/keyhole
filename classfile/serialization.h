#ifndef SERIALIZATION_H
#define SERIALIZATION_H

#include "attribute.h"
#include "classfile.h"
#include "constant_pool.h"
#include "method.h"
#include "sinks.h"

namespace kh::jvm::serialization {

auto serialize(
        kh::sinks::Sink auto& sink,
        const kh::jvm::attribute::Attribute& attribute) -> void {
    sink.write(attribute.name_index);
    sink.write(static_cast<std::uint32_t>(attribute.data.size()));
    sink.write_bytes(attribute.data);
}

auto serialize(
        kh::sinks::Sink auto& sink,
        const kh::jvm::method::Method& method) -> void {
    sink.write(method.access_flags);
    sink.write(method.name_index);
    sink.write(method.descriptor_index);
    sink.write(static_cast<std::uint16_t>(method.attributes.size()));

    for (const auto& attribute : method.attributes) {
        serialize(sink, attribute);
    }
}

auto serialize(
        kh::sinks::Sink auto& sink,
        const kh::jvm::constant_pool::ClassEntry entry) -> void {
    sink.write(static_cast<std::uint8_t>(constant_pool::tag(entry)));
    sink.write(entry.name_index);
}

auto serialize(
        kh::sinks::Sink auto& sink,
        const kh::jvm::constant_pool::MethodReferenceEntry entry) -> void {
    sink.write(static_cast<std::uint8_t>(constant_pool::tag(entry)));
    sink.write(entry.class_index);
    sink.write(entry.name_and_type_index);
}

auto serialize(
        kh::sinks::Sink auto& sink,
        kh::jvm::constant_pool::NameAndTypeEntry entry) -> void {
    sink.write(static_cast<uint8_t>(constant_pool::tag(entry)));
    sink.write(entry.name_index);
    sink.write(entry.descriptor_index);
}

auto serialize(
        kh::sinks::Sink auto& sink,
        kh::jvm::constant_pool::UTF8Entry entry) -> void {
    sink.write(static_cast<uint8_t>(constant_pool::tag(entry)));
    sink.write(static_cast<uint16_t>(entry.text.size()));

    for (const auto byte : entry.text) {
        sink.write(static_cast<uint8_t>(byte));
    }
}

auto serialize(
        kh::sinks::Sink auto& sink,
        const kh::jvm::constant_pool::ConstantPool& pool) -> void {
    for (const auto& entry : pool.entries()) {
        std::visit([&sink](const auto& e){
            serialize(sink, e);
        }, entry);
    }
}

auto serialize(
        kh::sinks::Sink auto& sink,
        const kh::jvm::classfile::ClassFile& klass) -> void {
    sink.write(static_cast<std::uint32_t>(0xCAFEBABE));
    sink.write(static_cast<std::uint16_t>(klass.version.minor));
    sink.write(static_cast<std::uint16_t>(klass.version.major));
    sink.write(static_cast<std::uint16_t>(klass.constant_pool.entries().size() + 1));

    serialize(sink, klass.constant_pool);

    sink.write(static_cast<std::uint16_t>(klass.access_flags));
    sink.write(static_cast<std::uint16_t>(klass.class_index));
    sink.write(static_cast<std::uint16_t>(klass.superclass_index));

    // TODO(garrett): Write interface entries
    sink.write(static_cast<std::uint16_t>(0x0000));

    // TODO(garrett): Write field entries
    sink.write(static_cast<std::uint16_t>(0x0000));

    sink.write(static_cast<std::uint16_t>(klass.methods.size()));

    for (const auto& method : klass.methods) {
        serialize(sink, method);
    }

    sink.write(static_cast<std::uint16_t>(klass.attributes.size()));

    for (const auto& attribute : klass.attributes) {
        serialize(sink, attribute);
    }
}

} // namespace kh::jvm::serialization

#endif // SERIALIZATION_H
