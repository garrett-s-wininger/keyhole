#include "gtest/gtest.h"

#include "serialization.h"
#include "tests/helpers.h"

namespace kh::jvm::serialization {

TEST(Serialization, SerializesAttribute) {
    constexpr std::array<const std::byte, 3> data{
        std::byte{'A'}, std::byte{'B'}, std::byte{'C'}
    };

    const attribute::Attribute attribute{
        12u,
        data
    };

    kh::sinks::VectorSink sink{};
    serialize(sink, attribute);

    constexpr std::array<std::byte, 9> expected{
        // Name index
        std::byte{0x00}, std::byte{0x0C},
        // Length
        std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x03},
        // Data
        std::byte{'A'}, std::byte{'B'}, std::byte{'C'}
    };

    const auto actual = sink.view();
    EXPECT_THAT(expected, EqualsBinary(actual));
}

TEST(Serialization, SerializesMethod) {
    const method::Method method{
        static_cast<uint16_t>(method::AccessFlags::ACC_PUBLIC),
        3u,
        4u,
        std::vector<attribute::Attribute>{
            attribute::Attribute{
                5u,
                std::span<const std::byte>{}
            }
        }
    };

    kh::sinks::VectorSink sink{};
    serialize(sink, method);

    constexpr auto expected = std::to_array({
        // Method access
        std::byte{0x00}, std::byte{0x01},
        // Name index
        std::byte{0x00}, std::byte{0x03},
        // Descriptor index
        std::byte{0x00}, std::byte{0x04},
        // Attribute count
        std::byte{0x00}, std::byte{0x01},
        // Attributes, 1 (empty)
        std::byte{0x00}, std::byte{0x05},
        std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}
    });

    const auto actual = sink.view();
    EXPECT_THAT(expected, EqualsBinary(actual));
}

TEST(Serialization, SerializesClassEntries) {
    constexpr constant_pool::ClassEntry entry{16};
    kh::sinks::VectorSink sink{};

    serialize(sink, entry);

    constexpr auto expected = std::array<const std::byte, 3>{
        std::byte{0x07}, std::byte{0x00}, std::byte{0x10}
    };

    const auto actual = sink.view();
    EXPECT_THAT(expected, EqualsBinary(actual));
}

TEST(Serialization, SerializesConstantPool) {
    const std::string text = "A";

    const constant_pool::ConstantPool pool{
        constant_pool::ClassEntry{2u},
        constant_pool::UTF8Entry{text}
    };

    kh::sinks::VectorSink sink{};
    serialize(sink, pool);

    constexpr auto expected = std::array<const std::byte, 7>{
        // Class entry
        std::byte{0x07},
        std::byte{0x00}, std::byte{0x02},
        // UTF8 entry
        std::byte{0x01},
        std::byte{0x00}, std::byte{0x01},
        std::byte{'A'}
    };

    const auto actual = sink.view();
    EXPECT_THAT(expected, EqualsBinary(actual));
}

TEST(Serialization, SerializesMethodReferenceEntries) {
    constexpr constant_pool::MethodReferenceEntry entry{1, 2};
    kh::sinks::VectorSink sink{};

    serialize(sink, entry);

    constexpr auto expected = std::array<const std::byte, 5>{
        // Tag
        std::byte{0x0A},
        // Class index
        std::byte{0x00}, std::byte{0x01},
        // Descriptor index
        std::byte{0x00}, std::byte{0x02}
    };

    const auto actual = sink.view();
    EXPECT_THAT(expected, EqualsBinary(actual));
}

TEST(Serialization, SerializesNameAndTypeEntries) {
    constexpr constant_pool::NameAndTypeEntry entry{2, 4};
    kh::sinks::VectorSink sink{};

    serialize(sink, entry);

    constexpr auto expected = std::array<const std::byte, 5>{
        // Tag
        std::byte{0x0C},
        // Name index
        std::byte{0x00}, std::byte{0x02},
        // Descriptor index
        std::byte{0x00}, std::byte{0x04}
    };

    const auto actual = sink.view();
    EXPECT_THAT(expected, EqualsBinary(actual));
}

TEST(Serialization, SerializesUTF8Entries) {
    const constant_pool::UTF8Entry entry{"MyClass"};
    kh::sinks::VectorSink sink{};

    serialize(sink, entry);

    constexpr auto expected = std::array<const std::byte, 10>{
        std::byte{0x01},
        std::byte{0x00}, std::byte{0x07},
        std::byte{'M'}, std::byte{'y'}, std::byte{'C'}, std::byte{'l'},
        std::byte{'a'}, std::byte{'s'}, std::byte{'s'}
    };

    const auto actual = sink.view();
    EXPECT_THAT(expected, EqualsBinary(actual));
}

TEST(Serialization, SerializesClassFile) {
    const auto class_name = std::string{"MyClass"};
    const auto superclass_name = std::string{"java/lang/Object"};
    auto klass = classfile::ClassFile(class_name, superclass_name);

    const auto method_name = std::string{"method"};
    const auto descriptor = "()V";

    klass.constant_pool.add(
        kh::jvm::constant_pool::UTF8Entry{method_name}
    );

    klass.constant_pool.add(
        kh::jvm::constant_pool::UTF8Entry{descriptor}
    );

    const auto attribute_name = std::string{"Deprecated"};

    klass.constant_pool.add(
        kh::jvm::constant_pool::UTF8Entry{attribute_name}
    );

    const auto deprecated_attribute = kh::jvm::attribute::Attribute{
        .name_index = 7,
        .data = std::span<const std::byte>{}
    };

    klass.methods.push_back(
        kh::jvm::method::Method{
            .access_flags =
                static_cast<std::uint16_t>(kh::jvm::method::AccessFlags::ACC_PUBLIC)
                | static_cast<std::uint16_t>(kh::jvm::method::AccessFlags::ACC_FINAL),
            .name_index = 5,
            .descriptor_index = 6,
            .attributes = std::vector<kh::jvm::attribute::Attribute>{
                deprecated_attribute
            }
        }
    );

    klass.attributes.push_back(deprecated_attribute);

    kh::sinks::VectorSink sink{};
    serialize(sink, klass);

    constexpr auto expected = std::to_array<const std::byte>({
        // NOTE(garrett): All multi-byte values in Big-Endian representation
        // Magic - u32
        std::byte{0xCA}, std::byte{0xFE}, std::byte{0xBA}, std::byte{0xBE},
        // Minor - u16
        std::byte{0x00}, std::byte{0x00},
        // Major - u16
        std::byte{0x00}, std::byte{0x37},
        // Constant Pool count + 1
        std::byte{0x00}, std::byte{0x08},
        // Name UTF8 Entry
        std::byte{0x01},
        std::byte{0x00}, std::byte{0x07},
        std::byte{'M'}, std::byte{'y'}, std::byte{'C'}, std::byte{'l'},
        std::byte{'a'}, std::byte{'s'}, std::byte{'s'},
        // Class info entry
        std::byte{0x07}, std::byte{0x00}, std::byte{0x01},
        // Name UTF8 Entry
        std::byte{0x01},
        std::byte{0x00}, std::byte{0x10},
        std::byte{'j'}, std::byte{'a'}, std::byte{'v'}, std::byte{'a'},
        std::byte{'/'}, std::byte{'l'}, std::byte{'a'}, std::byte{'n'},
        std::byte{'g'}, std::byte{'/'}, std::byte{'O'}, std::byte{'b'},
        std::byte{'j'}, std::byte{'e'}, std::byte{'c'}, std::byte{'t'},
        // Class info entry
        std::byte{0x07}, std::byte{0x00}, std::byte{0x03},
        // Name UTF8 Entry
        std::byte{0x01},
        std::byte{0x00}, std::byte{0x06},
        std::byte{'m'}, std::byte{'e'}, std::byte{'t'}, std::byte{'h'},
        std::byte{'o'}, std::byte{'d'},
        // Descriptor UTF8 Entry
        std::byte{0x01},
        std::byte{0x00}, std::byte{0x03},
        std::byte{'('}, std::byte{')'}, std::byte{'V'},
        // Attribute UTF8 Entry
        std::byte{0x01},
        std::byte{0x00}, std::byte{0x0A},
        std::byte{'D'}, std::byte{'e'}, std::byte{'p'}, std::byte{'r'},
        std::byte{'e'}, std::byte{'c'}, std::byte{'a'}, std::byte{'t'},
        std::byte{'e'}, std::byte{'d'},
        // Access flags
        std::byte{0x00}, std::byte{0x21},
        // Class name index
        std::byte{0x00}, std::byte{0x02},
        // Super class name index
        std::byte{0x00}, std::byte{0x04},
        // Interface count
        std::byte{0x00}, std::byte{0x00},
        // TODO(garrett): Include interfaces
        // Field count
        std::byte{0x00}, std::byte{0x00},
        // TODO(garrett): Include fields
        // Method count
        std::byte{0x00}, std::byte{0x01},
        // Method
        std::byte{0x00}, std::byte{0x11},
        std::byte{0x00}, std::byte{0x05},
        std::byte{0x00}, std::byte{0x06},
        std::byte{0x00}, std::byte{0x01},
        // Method - Deprecated attribute
        std::byte{0x00}, std::byte{0x07},
        std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
        // Attribute count
        std::byte{0x00}, std::byte{0x01},
        // Deprecated attribute
        std::byte{0x00}, std::byte{0x07},
        std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
    });

    const auto actual = sink.view();
    EXPECT_THAT(expected, EqualsBinary(actual));
}

} // namespace kh::jvm::serialization
