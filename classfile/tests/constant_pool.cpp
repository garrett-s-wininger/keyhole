#include "gtest/gtest.h"

#include "constant_pool.h"
#include "tests/helpers.h"

using namespace std::literals;

namespace constant_pool {

TEST(ConstantPool, GeneratesCorrectTagValues) {
    constexpr ClassEntry klass{};
    constexpr MethodReferenceEntry method_ref{};
    constexpr NameAndTypeEntry name_and_type{};
    const constant_pool::UTF8Entry utf8{};

    EXPECT_EQ(7, static_cast<std::uint8_t>(tag(klass)));
    EXPECT_EQ(10, static_cast<std::uint8_t>(tag(method_ref)));
    EXPECT_EQ(12, static_cast<std::uint8_t>(tag(name_and_type)));
    EXPECT_EQ(1, static_cast<std::uint8_t>(tag(utf8)));
}

TEST(ConstantPool, ResolveFailsOnTypeMismatch) {
    const ConstantPool pool{ClassEntry{1u}};

    ASSERT_ANY_THROW(pool.resolve<UTF8Entry>(1u));
}

TEST(ConstantPool, ResolveFailsOnOutOfBoundsIndex) {
    const ConstantPool pool{};
    ASSERT_ANY_THROW(pool.resolve<UTF8Entry>(15u));
}

TEST(ConstantPool, ResolveFailesOnZeroIndex) {
    const auto entry_text = std::string{"Test"};
    const ConstantPool pool{UTF8Entry{entry_text}};

    ASSERT_ANY_THROW(pool.resolve<UTF8Entry>(0u));
}

TEST(ConstantPool, ResolveProperlyGrabsEntryReference) {
    const auto entry_text = std::string{"ExampleEntry"};
    const ConstantPool pool{UTF8Entry{entry_text}};

    const auto& entry = pool.resolve<UTF8Entry>(1u);
    const auto& entry2 = pool.resolve<UTF8Entry>(1u);

    ASSERT_EQ(std::addressof(entry), std::addressof(entry2));
}

TEST(ConstantPool, AddingExistingUTF8EntryIsNoop) {
    const auto entry_text = std::string{"MyExample"};
    auto pool = ConstantPool{UTF8Entry{entry_text}};

    const auto entry_idx = pool.try_add_utf8_entry(entry_text);
    ASSERT_EQ(1uz, pool.entries().size());
    ASSERT_EQ(1uz, entry_idx);
}

TEST(ConstantPool, AddingNewUTF8EntryCachesAppropriately) {
    const auto entry_text = std::string{"FirstExample"};
    auto pool = ConstantPool{UTF8Entry{entry_text}};

    const auto new_entry_text = std::string{"NewExample"};
    const auto entry_idx = pool.try_add_utf8_entry(new_entry_text);

    ASSERT_EQ(2uz, pool.entries().size());
    ASSERT_EQ(2uz, entry_idx);
}

} // namespace constant_pool
