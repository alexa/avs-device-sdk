/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <array>
#include <gtest/gtest.h>

#include <AndroidUtilities/AndroidSLESObject.h>

using namespace ::testing;

namespace alexaClientSDK {
namespace applicationUtilities {
namespace androidUtilities {
namespace test {

class AndroidSLESObjectTest : public Test {
public:
    /// Used to check if destroyed has been called.
    static bool destroyed;

    /// This will be used to mock the getInterface result.
    static struct MockInterface { SLInterfaceID id; } mockInterface;

protected:
    /**
     * Setup default value for static flags.
     */
    virtual void SetUp();
};

bool AndroidSLESObjectTest::destroyed = false;
AndroidSLESObjectTest::MockInterface AndroidSLESObjectTest::mockInterface;

void AndroidSLESObjectTest::SetUp() {
    destroyed = false;
}

/// Mock realize that succeeds.
SLresult mockRealizeSucceed(SLObjectItf self, SLboolean async) {
    return SL_RESULT_SUCCESS;
}

/// Mock realize that fails.
SLresult mockRealizeFail(SLObjectItf self, SLboolean async) {
    return SL_RESULT_PERMISSION_DENIED;
}

/// Mock destroy and keep track that the method was called once.
void mockDestroy(SLObjectItf self) {
    AndroidSLESObjectTest::destroyed = true;
}

/// Mock get interface.
SLresult mockGetInterface(SLObjectItf self, const SLInterfaceID iid, void* pInterface) {
    if (iid == AndroidSLESObjectTest::mockInterface.id) {
        auto tmpPtr = static_cast<AndroidSLESObjectTest::MockInterface*>(pInterface);
        *tmpPtr = AndroidSLESObjectTest::mockInterface;
        return SL_RESULT_SUCCESS;
    }
    return SL_RESULT_CONTENT_UNSUPPORTED;
}

/**
 * Test create method when the provided object can be realized.
 */
TEST_F(AndroidSLESObjectTest, testCreateDestroySucceed) {
    {
        SLObjectItf_ mockObj;
        SLObjectItf_* mockSinglePtr = &mockObj;
        SLObjectItf mockDoublePtr = &mockSinglePtr;
        mockObj.Realize = mockRealizeSucceed;
        mockObj.Destroy = mockDestroy;

        auto androidObject = AndroidSLESObject::create(mockDoublePtr);
        EXPECT_NE(androidObject, nullptr);
        EXPECT_FALSE(destroyed);
    }
    // Check that @c SLObjectItf_.Destroy is called when androidObject is destroyed.
    EXPECT_TRUE(destroyed);
}

/**
 * Test create method when the provided object cannot be realized.
 */
TEST_F(AndroidSLESObjectTest, testCreateFailed) {
    SLObjectItf_ mockObj;
    SLObjectItf_* mockSinglePtr = &mockObj;
    SLObjectItf mockDoublePtr = &mockSinglePtr;
    mockObj.Realize = mockRealizeFail;
    mockObj.Destroy = mockDestroy;

    auto androidObject = AndroidSLESObject::create(mockDoublePtr);
    EXPECT_EQ(androidObject, nullptr);
    EXPECT_TRUE(destroyed);  // object should be destroyed after unsuccessful realization.
}

/**
 * Test get interface when method succeeds.
 */
TEST_F(AndroidSLESObjectTest, testGetInterface) {
    SLObjectItf_ mockObj;
    SLObjectItf_* mockSinglePtr = &mockObj;
    SLObjectItf mockDoublePtr = &mockSinglePtr;
    mockObj.Realize = mockRealizeSucceed;
    mockObj.Destroy = mockDestroy;
    mockObj.GetInterface = mockGetInterface;
    mockInterface.id = SL_IID_ENGINE;

    auto androidObject = AndroidSLESObject::create(mockDoublePtr);
    EXPECT_NE(androidObject, nullptr);

    MockInterface copy;
    copy.id = SL_IID_PLAY;
    EXPECT_TRUE(androidObject->getInterface(SL_IID_ENGINE, &copy));
    EXPECT_EQ(copy.id, mockInterface.id);
}

/**
 * Test get interface when method fails.
 */
TEST_F(AndroidSLESObjectTest, testGetInterfaceFailed) {
    SLObjectItf_ mockObj;
    SLObjectItf_* mockSinglePtr = &mockObj;
    SLObjectItf mockDoublePtr = &mockSinglePtr;
    mockObj.Realize = mockRealizeSucceed;
    mockObj.Destroy = mockDestroy;
    mockObj.GetInterface = mockGetInterface;
    mockInterface.id = SL_IID_ENGINE;

    auto androidObject = AndroidSLESObject::create(mockDoublePtr);
    EXPECT_NE(androidObject, nullptr);

    MockInterface copy;
    copy.id = SL_IID_PLAY;
    EXPECT_FALSE(androidObject->getInterface(SL_IID_PLAY, &copy));
    EXPECT_EQ(copy.id, SL_IID_PLAY);
}

}  // namespace test
}  // namespace androidUtilities
}  // namespace applicationUtilities
}  // namespace alexaClientSDK
