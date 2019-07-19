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
#ifndef ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_ANDROIDUTILITIES_TEST_ANDROIDUTILITIES_MOCKANDROIDSLESOBJECT_H_
#define ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_ANDROIDUTILITIES_TEST_ANDROIDUTILITIES_MOCKANDROIDSLESOBJECT_H_

#include <unordered_map>
#include <SLES/OpenSLES.h>

#include <AndroidUtilities/MockAndroidSLESInterface.h>

namespace alexaClientSDK {
namespace applicationUtilities {
namespace androidUtilities {
namespace test {

/**
 * This class will be used to mock @c SLObjectItf.
 *
 * Note that for this we create a real structure and set their function pointers to point to mock methods.
 */
class MockAndroidSLESObject {
public:
    /**
     * Mock the realize method to either succeed or fail. By default, this mock object realize call will succeed.
     *
     * @param succeed @c true if the mocked method should succeed, @c false otherwise.
     */
    void mockRealize(bool succeed = true);

    /**
     * Mock get interface for the given ID.
     *
     * @param id The interface id.
     * @param object The mock interface that will be returned.
     */
    void mockGetInterface(SLInterfaceID id, std::shared_ptr<MockInterface> object);

    /**
     * Gets the underlying object.
     *
     * @return The underlying open sl object.
     */
    SLObjectItf getObject();

    /**
     * Constructor.
     */
    MockAndroidSLESObject();

    /**
     * Destructor.
     */
    ~MockAndroidSLESObject();

private:
    /// The underlying object. We don't use the @c SLObjectItf because it points to a constant struct, which wouldn't
    /// allow us to set their function pointers.
    SLObjectItf_** m_slObject;
};

// Static interface map that will be used to mock getInterface.
static std::unordered_map<SLInterfaceID, std::shared_ptr<MockInterface>> g_interfaces;

SLresult realizeSucceed(SLObjectItf self, SLboolean async) {
    return SL_RESULT_SUCCESS;
}

SLresult realizeFailed(SLObjectItf self, SLboolean async) {
    return SL_RESULT_INTERNAL_ERROR;
}

SLresult getMockInterface(SLObjectItf self, SLInterfaceID id, void* interface) {
    auto it = g_interfaces.find(id);
    if (it != g_interfaces.end() && it->second != nullptr) {
        it->second->set(interface);
        return SL_RESULT_SUCCESS;
    }
    return SL_RESULT_CONTENT_NOT_FOUND;
}

void MockAndroidSLESObject::mockGetInterface(SLInterfaceID id, std::shared_ptr<MockInterface> object) {
    g_interfaces[id] = object;
}

void MockAndroidSLESObject::mockRealize(bool succeed) {
    (*m_slObject)->Realize = succeed ? realizeSucceed : realizeFailed;
}

SLObjectItf MockAndroidSLESObject::getObject() {
    return m_slObject;
}

MockAndroidSLESObject::MockAndroidSLESObject() {
    auto object = new SLObjectItf_();
    auto objectPtr = new SLObjectItf_*();
    *objectPtr = object;
    m_slObject = objectPtr;
    (*m_slObject)->Realize = realizeSucceed;
    (*m_slObject)->GetInterface = getMockInterface;
    (*m_slObject)->Destroy = [](SLObjectItf obj) {};
};

MockAndroidSLESObject::~MockAndroidSLESObject() {
    delete *m_slObject;
    delete m_slObject;
    g_interfaces.clear();
}

}  // namespace test
}  // namespace androidUtilities
}  // namespace applicationUtilities
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_ANDROIDUTILITIES_TEST_ANDROIDUTILITIES_MOCKANDROIDSLESOBJECT_H_
