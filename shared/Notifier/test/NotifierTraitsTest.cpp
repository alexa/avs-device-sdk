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

#include <memory>

#include <gtest/gtest.h>

#include <acsdk/Notifier/internal/NotifierTraits.h>

namespace alexaClientSDK {
namespace notifier {
namespace test {

using namespace ::testing;

/**
 * @brief Test fixture for @c NotifierTraits.
 * @ingroup Lib_acsdkNotifierTests
 */
class NotifierTraitsTest : public ::testing::Test {};

/**
 * Verify conversion to void works.
 */
TEST_F(NotifierTraitsTest, test_toVoidConversion) {
    auto reference = std::make_shared<std::string>("test");
    auto voidReference = NotifierTraits<std::string>::toVoidPtr(reference);
    ASSERT_EQ(reference.get(), voidReference.get());
}

/**
 * Verify conversion from void works.
 */
TEST_F(NotifierTraitsTest, test_fromVoidConversion) {
    auto reference = std::make_shared<std::string>("test");
    auto voidReference = NotifierTraits<std::string>::toVoidPtr(reference);
    auto reference2 = NotifierTraits<std::string>::fromVoidPtr(voidReference);
    ASSERT_EQ(reference.get(), reference2.get());
}

/**
 * Verify function adapter works.
 */
TEST_F(NotifierTraitsTest, test_adaptFunction) {
    std::shared_ptr<std::string> calledValue;
    std::function<void(const std::shared_ptr<std::string>& value)> fn =
        [&calledValue](const std::shared_ptr<std::string>& value) { calledValue = value; };
    auto adaptedFn = NotifierTraits<std::string>::adaptFunction(std::move(fn));
    auto reference = std::make_shared<std::string>("test");
    adaptedFn(NotifierTraits<std::string>::toVoidPtr(reference));
    ASSERT_TRUE(calledValue);
    ASSERT_EQ(calledValue, reference);
}

}  // namespace test
}  // namespace notifier
}  // namespace alexaClientSDK
