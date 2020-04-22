/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <string>

#include <gtest/gtest.h>

#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {

/**
 * Dummy structure used for our tests.
 */
struct Dummy {
    /// Dummy structure name used to validate the object content.
    std::string m_name;
};

/**
 * Test structure used to ensure that Optional can be used with types without default constructor.
 */
struct StructWithoutDefaultConstructor {
    /// Explicitly delete the default constructor.
    StructWithoutDefaultConstructor() = delete;

    /**
     * Create default constructor.
     */
    StructWithoutDefaultConstructor(const StructWithoutDefaultConstructor& other) = default;
    /**
     * Create object with the given id.
     *
     * @param id The id to assign to the new object.
     */
    StructWithoutDefaultConstructor(int id);

    /// This object id.
    int m_id;
};

/**
 * Class with static counters for the constructor and the destructor methods.
 */
struct ReferenceCounter {
    /// Count the amount of times the constructor has been called.
    static size_t s_built;

    /// Count the amount of times the destructor has been called.
    static size_t s_destroyed;

    /**
     * Empty constructor.
     */
    ReferenceCounter();

    /**
     * Copy constructor.
     */
    ReferenceCounter(const ReferenceCounter& rhs);

    /**
     * Destructor.
     */
    ~ReferenceCounter();
};

size_t ReferenceCounter::s_built = 0;
size_t ReferenceCounter::s_destroyed = 0;

ReferenceCounter::ReferenceCounter() {
    s_built++;
}

ReferenceCounter::ReferenceCounter(const ReferenceCounter& rhs) {
    s_built++;
}

ReferenceCounter::~ReferenceCounter() {
    s_destroyed++;
}

StructWithoutDefaultConstructor::StructWithoutDefaultConstructor(int id) : m_id{id} {
}

TEST(OptionalTest, test_createEmptyOptional) {
    Optional<Dummy> empty;
    EXPECT_FALSE(empty.hasValue());
}

TEST(OptionalTest, test_createOptionalWithValue) {
    Optional<Dummy> dummy{Dummy{}};
    EXPECT_TRUE(dummy.hasValue());
}

TEST(OptionalTest, test_getValueOfOptionalWithValue) {
    Optional<Dummy> dummy{Dummy{"EXPECTED_NAME"}};
    ASSERT_TRUE(dummy.hasValue());

    auto name = dummy.valueOr(Dummy{"OTHER_NAME"}).m_name;
    EXPECT_EQ(name, "EXPECTED_NAME");

    name = dummy.value().m_name;
    EXPECT_EQ(name, "EXPECTED_NAME");
}

TEST(OptionalTest, test_getValueOfEmptyOptional) {
    Optional<Dummy> dummy;
    ASSERT_FALSE(dummy.hasValue());

    auto name = dummy.valueOr(Dummy{"OTHER_NAME"}).m_name;
    EXPECT_EQ(name, "OTHER_NAME");

    name = dummy.value().m_name;
    EXPECT_EQ(name, std::string());
}

TEST(OptionalTest, test_functionWithEmptyOptionalReturn) {
    auto function = []() -> Optional<Dummy> { return Optional<Dummy>(); };
    auto empty = function();
    ASSERT_FALSE(empty.hasValue());
}

TEST(OptionalTest, test_functionWithNonEmptyOptionalReturn) {
    auto function = []() -> Optional<Dummy> { return Optional<Dummy>{Dummy{"EXPECTED_NAME"}}; };
    auto dummy = function();
    ASSERT_TRUE(dummy.hasValue());
    ASSERT_EQ(dummy.value().m_name, "EXPECTED_NAME");
}

TEST(OptionalTest, test_copyOptionalWithValue) {
    Optional<Dummy> dummy1{Dummy{"EXPECTED_NAME"}};
    ASSERT_TRUE(dummy1.hasValue());

    Optional<Dummy> dummy2{dummy1};
    EXPECT_TRUE(dummy2.hasValue());
    EXPECT_EQ(dummy1.value().m_name, dummy2.value().m_name);
}

TEST(OptionalTest, test_copyEmptyOptional) {
    Optional<Dummy> dummy1;
    ASSERT_FALSE(dummy1.hasValue());

    Optional<Dummy> dummy2{dummy1};
    EXPECT_FALSE(dummy2.hasValue());
}

TEST(OptionalTest, test_setNewValueForEmptyOptional) {
    Dummy dummy{"EXPECTED_NAME"};
    Optional<Dummy> optionalDummy;
    optionalDummy.set(dummy);

    EXPECT_TRUE(optionalDummy.hasValue());
    EXPECT_EQ(dummy.m_name, optionalDummy.value().m_name);
}

TEST(OptionalTest, test_setNewValueForNonEmptyOptional) {
    Optional<Dummy> optionalDummy{Dummy{"OLD_NAME"}};
    ASSERT_TRUE(optionalDummy.hasValue());

    optionalDummy.set(Dummy{"EXPECTED_NAME"});

    EXPECT_TRUE(optionalDummy.hasValue());
    EXPECT_EQ(optionalDummy.value().m_name, "EXPECTED_NAME");
}

TEST(OptionalTest, test_resetEmptyOptional) {
    Optional<Dummy> dummy;
    ASSERT_FALSE(dummy.hasValue());

    dummy.reset();
    EXPECT_FALSE(dummy.hasValue());
}

TEST(OptionalTest, test_resetNonEmptyOptional) {
    Optional<Dummy> optionalDummy{Dummy{"OLD_NAME"}};
    ASSERT_TRUE(optionalDummy.hasValue());

    optionalDummy.reset();
    EXPECT_FALSE(optionalDummy.hasValue());
}

TEST(OptionalTest, test_optionalObjectWithoutDefaultConstructor) {
    Optional<StructWithoutDefaultConstructor> emptyOptional;
    EXPECT_FALSE(emptyOptional.hasValue());

    const int id = 10;
    Optional<StructWithoutDefaultConstructor> validOptional{StructWithoutDefaultConstructor(id)};
    EXPECT_EQ(validOptional.valueOr(id + 1).m_id, id);
}

TEST(OptionalTest, test_constructorCallsMatchDestructorCalls) {
    {
        Optional<ReferenceCounter> optional{ReferenceCounter()};
        EXPECT_GT(ReferenceCounter::s_built, ReferenceCounter::s_destroyed);

        optional.set(ReferenceCounter());
        EXPECT_GT(ReferenceCounter::s_built, ReferenceCounter::s_destroyed);

        optional.reset();
        EXPECT_EQ(ReferenceCounter::s_built, ReferenceCounter::s_destroyed);

        optional.set(ReferenceCounter());
        EXPECT_GT(ReferenceCounter::s_built, ReferenceCounter::s_destroyed);

        Optional<ReferenceCounter> other{optional};
        EXPECT_GT(ReferenceCounter::s_built, ReferenceCounter::s_destroyed);
    }
    EXPECT_EQ(ReferenceCounter::s_built, ReferenceCounter::s_destroyed);
}

TEST(OptionalTest, test_equalityOperator) {
    Optional<std::string> empty;
    Optional<std::string> valid{"valid"};
    Optional<std::string> other{"other"};
    Optional<std::string> validCopy{valid};

    EXPECT_FALSE(empty == valid);
    EXPECT_FALSE(valid == other);
    EXPECT_EQ(valid, validCopy);
}

TEST(OptionalTest, test_inequalityOperator) {
    Optional<std::string> empty;
    Optional<std::string> valid{"valid"};
    Optional<std::string> other{"other"};
    Optional<std::string> validCopy{valid};

    EXPECT_NE(empty, valid);
    EXPECT_NE(valid, other);
    EXPECT_FALSE(valid != validCopy);
}

}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
