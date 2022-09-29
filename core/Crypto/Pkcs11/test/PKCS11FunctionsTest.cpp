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

#include <acsdk/Pkcs11/private/PKCS11Functions.h>
#include <acsdk/Pkcs11/private/PKCS11Slot.h>
#include <acsdk/Pkcs11/private/PKCS11Config.h>

#include <gtest/gtest.h>

namespace alexaClientSDK {
namespace pkcs11 {
namespace test {

using namespace ::testing;

/// Test that the constructor with a nullptr doesn't segfault.
TEST(PKCS11FunctionsTest, test_badFunction) {
    auto functions = PKCS11Functions::create("/lib_doesnt_exist.so");
    ASSERT_EQ(nullptr, functions);
}

TEST(PKCS11FunctionsTest, test_initHsm) {
    auto functions = PKCS11Functions::create(PKCS11_LIBRARY);
    ASSERT_NE(nullptr, functions);
}

TEST(PKCS11FunctionsTest, test_listSlotsNoTokens) {
    auto functions = PKCS11Functions::create(PKCS11_LIBRARY);
    ASSERT_NE(nullptr, functions);
    std::vector<std::shared_ptr<PKCS11Slot> > slots;
    ASSERT_TRUE(functions->listSlots(false, slots));
}

TEST(PKCS11FunctionsTest, test_listSlotsWithTokens) {
    auto functions = PKCS11Functions::create(PKCS11_LIBRARY);
    ASSERT_NE(nullptr, functions);
    std::vector<std::shared_ptr<PKCS11Slot> > slots;
    ASSERT_TRUE(functions->listSlots(true, slots));
    ASSERT_FALSE(slots.empty());
}

TEST(PKCS11FunctionsTest, test_findTestSlot) {
    auto functions = PKCS11Functions::create(PKCS11_LIBRARY);
    ASSERT_NE(nullptr, functions);
    std::shared_ptr<PKCS11Slot> slot;
    ASSERT_TRUE(functions->findSlotByTokenName(PKCS11_TOKEN_NAME, slot));
    ASSERT_NE(nullptr, slot);
}

TEST(PKCS11FunctionsTest, test_findOtherSlot) {
    auto functions = PKCS11Functions::create(PKCS11_LIBRARY);
    ASSERT_NE(nullptr, functions);
    std::shared_ptr<PKCS11Slot> slot;
    ASSERT_TRUE(functions->findSlotByTokenName("ACSDK-ERR", slot));
    ASSERT_EQ(nullptr, slot);
}

}  // namespace test
}  // namespace pkcs11
}  // namespace alexaClientSDK
