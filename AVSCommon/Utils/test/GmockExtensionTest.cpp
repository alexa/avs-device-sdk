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

#include <acsdk/Test/GmockExtensions.h>

namespace alexaClientSDK {
namespace test {

using namespace ::testing;

/**
 * @brief Interface with noexcept and const noexcept methods.
 */
class SomeInterface {
public:
    virtual ~SomeInterface() noexcept = default;
    virtual int noexceptMethod0() noexcept = 0;
    virtual int noexceptMethod1(int) noexcept = 0;
    virtual int noexceptMethod2(int, int) noexcept = 0;
    virtual int noexceptMethod3(int, int, int) noexcept = 0;
    virtual int noexceptMethod4(int, int, int, int) noexcept = 0;
    virtual int noexceptMethod5(int, int, int, int, int) noexcept = 0;
    virtual int noexceptMethod6(int, int, int, int, int, int) noexcept = 0;
    virtual int noexceptMethod7(int, int, int, int, int, int, int) noexcept = 0;
    virtual int noexceptMethod8(int, int, int, int, int, int, int, int) noexcept = 0;
    virtual int noexceptMethod9(int, int, int, int, int, int, int, int, int) noexcept = 0;
    virtual int noexceptMethod10(int, int, int, int, int, int, int, int, int, int) noexcept = 0;
    virtual int constNoexceptMethod0() const noexcept = 0;
    virtual int constNoexceptMethod1(int) const noexcept = 0;
    virtual int constNoexceptMethod2(int, int) const noexcept = 0;
    virtual int constNoexceptMethod3(int, int, int) const noexcept = 0;
    virtual int constNoexceptMethod4(int, int, int, int) const noexcept = 0;
    virtual int constNoexceptMethod5(int, int, int, int, int) const noexcept = 0;
    virtual int constNoexceptMethod6(int, int, int, int, int, int) const noexcept = 0;
    virtual int constNoexceptMethod7(int, int, int, int, int, int, int) const noexcept = 0;
    virtual int constNoexceptMethod8(int, int, int, int, int, int, int, int) const noexcept = 0;
    virtual int constNoexceptMethod9(int, int, int, int, int, int, int, int, int) const noexcept = 0;
    virtual int constNoexceptMethod10(int, int, int, int, int, int, int, int, int, int) const noexcept = 0;
};

/**
 * @brief Mock for @c SomeInterface.
 */
class SomeMock : public SomeInterface {
public:
    MOCK_NOEXCEPT_METHOD0(noexceptMethod0, int());
    MOCK_NOEXCEPT_METHOD1(noexceptMethod1, int(int));
    MOCK_NOEXCEPT_METHOD2(noexceptMethod2, int(int, int));
    MOCK_NOEXCEPT_METHOD3(noexceptMethod3, int(int, int, int));
    MOCK_NOEXCEPT_METHOD4(noexceptMethod4, int(int, int, int, int));
    MOCK_NOEXCEPT_METHOD5(noexceptMethod5, int(int, int, int, int, int));
    MOCK_NOEXCEPT_METHOD6(noexceptMethod6, int(int, int, int, int, int, int));
    MOCK_NOEXCEPT_METHOD7(noexceptMethod7, int(int, int, int, int, int, int, int));
    MOCK_NOEXCEPT_METHOD8(noexceptMethod8, int(int, int, int, int, int, int, int, int));
    MOCK_NOEXCEPT_METHOD9(noexceptMethod9, int(int, int, int, int, int, int, int, int, int));
    MOCK_NOEXCEPT_METHOD10(noexceptMethod10, int(int, int, int, int, int, int, int, int, int, int));

    MOCK_CONST_NOEXCEPT_METHOD0(constNoexceptMethod0, int());
    MOCK_CONST_NOEXCEPT_METHOD1(constNoexceptMethod1, int(int));
    MOCK_CONST_NOEXCEPT_METHOD2(constNoexceptMethod2, int(int, int));
    MOCK_CONST_NOEXCEPT_METHOD3(constNoexceptMethod3, int(int, int, int));
    MOCK_CONST_NOEXCEPT_METHOD4(constNoexceptMethod4, int(int, int, int, int));
    MOCK_CONST_NOEXCEPT_METHOD5(constNoexceptMethod5, int(int, int, int, int, int));
    MOCK_CONST_NOEXCEPT_METHOD6(constNoexceptMethod6, int(int, int, int, int, int, int));
    MOCK_CONST_NOEXCEPT_METHOD7(constNoexceptMethod7, int(int, int, int, int, int, int, int));
    MOCK_CONST_NOEXCEPT_METHOD8(constNoexceptMethod8, int(int, int, int, int, int, int, int, int));
    MOCK_CONST_NOEXCEPT_METHOD9(constNoexceptMethod9, int(int, int, int, int, int, int, int, int, int));
    MOCK_CONST_NOEXCEPT_METHOD10(constNoexceptMethod10, int(int, int, int, int, int, int, int, int, int, int));
};

/**
 * @brief Test fixture for testing GMock extensions.
 */
class GmockExtensionsTest : public ::testing::Test {};

TEST_F(GmockExtensionsTest, test_noexceptMethod0) {
    SomeMock mock;
    EXPECT_CALL(mock, noexceptMethod0()).Times(1).WillOnce(Return(-5));
    ASSERT_EQ(-5, mock.noexceptMethod0());
};

TEST_F(GmockExtensionsTest, test_noexceptMethod1) {
    SomeMock mock;
    EXPECT_CALL(mock, noexceptMethod1(1)).Times(1).WillOnce(Return(-5));
    ASSERT_EQ(-5, mock.noexceptMethod1(1));
};

TEST_F(GmockExtensionsTest, test_noexceptMethod2) {
    SomeMock mock;
    EXPECT_CALL(mock, noexceptMethod2(1, 2)).Times(1).WillOnce(Return(-5));
    ASSERT_EQ(-5, mock.noexceptMethod2(1, 2));
};

TEST_F(GmockExtensionsTest, test_noexceptMethod3) {
    SomeMock mock;
    EXPECT_CALL(mock, noexceptMethod3(1, 2, 3)).Times(1).WillOnce(Return(-5));
    ASSERT_EQ(-5, mock.noexceptMethod3(1, 2, 3));
};

TEST_F(GmockExtensionsTest, test_noexceptMethod4) {
    SomeMock mock;
    EXPECT_CALL(mock, noexceptMethod4(1, 2, 3, 4)).Times(1).WillOnce(Return(-5));
    ASSERT_EQ(-5, mock.noexceptMethod4(1, 2, 3, 4));
};

TEST_F(GmockExtensionsTest, test_noexceptMethod5) {
    SomeMock mock;
    EXPECT_CALL(mock, noexceptMethod5(1, 2, 3, 4, 5)).Times(1).WillOnce(Return(-5));
    ASSERT_EQ(-5, mock.noexceptMethod5(1, 2, 3, 4, 5));
};

TEST_F(GmockExtensionsTest, test_noexceptMethod6) {
    SomeMock mock;
    EXPECT_CALL(mock, noexceptMethod6(1, 2, 3, 4, 5, 6)).Times(1).WillOnce(Return(-5));
    ASSERT_EQ(-5, mock.noexceptMethod6(1, 2, 3, 4, 5, 6));
};

TEST_F(GmockExtensionsTest, test_noexceptMethod7) {
    SomeMock mock;
    EXPECT_CALL(mock, noexceptMethod7(1, 2, 3, 4, 5, 6, 7)).Times(1).WillOnce(Return(-5));
    ASSERT_EQ(-5, mock.noexceptMethod7(1, 2, 3, 4, 5, 6, 7));
};

TEST_F(GmockExtensionsTest, test_noexceptMethod8) {
    SomeMock mock;
    EXPECT_CALL(mock, noexceptMethod8(1, 2, 3, 4, 5, 6, 7, 8)).Times(1).WillOnce(Return(-5));
    ASSERT_EQ(-5, mock.noexceptMethod8(1, 2, 3, 4, 5, 6, 7, 8));
};

TEST_F(GmockExtensionsTest, test_noexceptMethod9) {
    SomeMock mock;
    EXPECT_CALL(mock, noexceptMethod9(1, 2, 3, 4, 5, 6, 7, 8, 9)).Times(1).WillOnce(Return(-5));
    ASSERT_EQ(-5, mock.noexceptMethod9(1, 2, 3, 4, 5, 6, 7, 8, 9));
};

TEST_F(GmockExtensionsTest, test_noexceptMethod10) {
    SomeMock mock;
    EXPECT_CALL(mock, noexceptMethod10(1, 2, 3, 4, 5, 6, 7, 8, 9, 10)).Times(1).WillOnce(Return(-5));
    ASSERT_EQ(-5, mock.noexceptMethod10(1, 2, 3, 4, 5, 6, 7, 8, 9, 10));
};

TEST_F(GmockExtensionsTest, test_constNoexceptMethod0) {
    SomeMock mock;
    EXPECT_CALL(mock, constNoexceptMethod0()).Times(1).WillOnce(Return(-5));
    ASSERT_EQ(-5, mock.constNoexceptMethod0());
};

TEST_F(GmockExtensionsTest, test_constNoexceptMethod1) {
    SomeMock mock;
    EXPECT_CALL(mock, constNoexceptMethod1(1)).Times(1).WillOnce(Return(-5));
    ASSERT_EQ(-5, mock.constNoexceptMethod1(1));
};

TEST_F(GmockExtensionsTest, test_constNoexceptMethod2) {
    SomeMock mock;
    EXPECT_CALL(mock, constNoexceptMethod2(1, 2)).Times(1).WillOnce(Return(-5));
    ASSERT_EQ(-5, mock.constNoexceptMethod2(1, 2));
};

TEST_F(GmockExtensionsTest, test_constNoexceptMethod3) {
    SomeMock mock;
    EXPECT_CALL(mock, noexceptMethod3(1, 2, 3)).Times(1).WillOnce(Return(-5));
    ASSERT_EQ(-5, mock.noexceptMethod3(1, 2, 3));
};

TEST_F(GmockExtensionsTest, test_constNoexceptMethod4) {
    SomeMock mock;
    EXPECT_CALL(mock, constNoexceptMethod4(1, 2, 3, 4)).Times(1).WillOnce(Return(-5));
    ASSERT_EQ(-5, mock.constNoexceptMethod4(1, 2, 3, 4));
};

TEST_F(GmockExtensionsTest, test_constNoexceptMethod5) {
    SomeMock mock;
    EXPECT_CALL(mock, constNoexceptMethod5(1, 2, 3, 4, 5)).Times(1).WillOnce(Return(-5));
    ASSERT_EQ(-5, mock.constNoexceptMethod5(1, 2, 3, 4, 5));
};

TEST_F(GmockExtensionsTest, test_constNoexceptMethod6) {
    SomeMock mock;
    EXPECT_CALL(mock, constNoexceptMethod6(1, 2, 3, 4, 5, 6)).Times(1).WillOnce(Return(-5));
    ASSERT_EQ(-5, mock.constNoexceptMethod6(1, 2, 3, 4, 5, 6));
};

TEST_F(GmockExtensionsTest, test_constNoexceptMethod7) {
    SomeMock mock;
    EXPECT_CALL(mock, constNoexceptMethod7(1, 2, 3, 4, 5, 6, 7)).Times(1).WillOnce(Return(-5));
    ASSERT_EQ(-5, mock.constNoexceptMethod7(1, 2, 3, 4, 5, 6, 7));
};

TEST_F(GmockExtensionsTest, test_constNoexceptMethod8) {
    SomeMock mock;
    EXPECT_CALL(mock, constNoexceptMethod8(1, 2, 3, 4, 5, 6, 7, 8)).Times(1).WillOnce(Return(-5));
    ASSERT_EQ(-5, mock.constNoexceptMethod8(1, 2, 3, 4, 5, 6, 7, 8));
};

TEST_F(GmockExtensionsTest, test_constNoexceptMethod9) {
    SomeMock mock;
    EXPECT_CALL(mock, constNoexceptMethod9(1, 2, 3, 4, 5, 6, 7, 8, 9)).Times(1).WillOnce(Return(-5));
    ASSERT_EQ(-5, mock.constNoexceptMethod9(1, 2, 3, 4, 5, 6, 7, 8, 9));
};

TEST_F(GmockExtensionsTest, test_constNoexceptMethod10) {
    SomeMock mock;
    EXPECT_CALL(mock, constNoexceptMethod10(1, 2, 3, 4, 5, 6, 7, 8, 9, 10)).Times(1).WillOnce(Return(-5));
    ASSERT_EQ(-5, mock.constNoexceptMethod10(1, 2, 3, 4, 5, 6, 7, 8, 9, 10));
};

}  // namespace test
}  // namespace alexaClientSDK
