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
#ifndef ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_ANDROIDUTILITIES_TEST_ANDROIDUTILITIES_MOCKANDROIDSLESINTERFACE_H_
#define ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_ANDROIDUTILITIES_TEST_ANDROIDUTILITIES_MOCKANDROIDSLESINTERFACE_H_

namespace alexaClientSDK {
namespace applicationUtilities {
namespace androidUtilities {
namespace test {

/**
 * Interface for all mocks to OpenSL ES Interfaces which are C structs.
 */
class MockInterface {
public:
    /**
     * Set the given pointer to the underlying object.
     *
     * @param destination The object that will store the underlying object pointer.
     */
    virtual void set(void* destination) = 0;

    /**
     * Destructor.
     */
    virtual ~MockInterface() = default;
};

/**
 * Implement the @c MockInterface for any OpenSL ES Type.
 */
template <typename InterfaceT>
class MockInterfaceImpl : public MockInterface {
public:
    /// @name MockInterface methods.
    /// @{
    void set(void* destination);
    /// @}

    /// Constructor.
    MockInterfaceImpl();

    /// Destructor.
    ~MockInterfaceImpl();

    /**
     * Getter to the underlying mock object.
     * @return
     */
    InterfaceT& get();

private:
    /// We are actually mocking a double pointer to the actual struct.
    InterfaceT** m_mockAndroidSLESObject;
};

template <typename InterfaceT>
MockInterfaceImpl<InterfaceT>::MockInterfaceImpl() {
    auto object = new InterfaceT();
    auto objectPtr = new InterfaceT*();
    *objectPtr = object;
    m_mockAndroidSLESObject = objectPtr;
}

template <typename InterfaceT>
MockInterfaceImpl<InterfaceT>::~MockInterfaceImpl() {
    delete *m_mockAndroidSLESObject;
    delete m_mockAndroidSLESObject;
}

template <typename InterfaceT>
void MockInterfaceImpl<InterfaceT>::set(void* destination) {
    auto destinationPtr = static_cast<InterfaceT***>(destination);
    *destinationPtr = m_mockAndroidSLESObject;
}

template <typename InterfaceT>
InterfaceT& MockInterfaceImpl<InterfaceT>::get() {
    return **m_mockAndroidSLESObject;
}

}  // namespace test
}  // namespace androidUtilities
}  // namespace applicationUtilities
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_ANDROIDUTILITIES_TEST_ANDROIDUTILITIES_MOCKANDROIDSLESINTERFACE_H_
