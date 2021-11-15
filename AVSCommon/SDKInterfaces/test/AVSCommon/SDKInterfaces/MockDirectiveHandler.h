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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKDIRECTIVEHANDLER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKDIRECTIVEHANDLER_H_

#include <gmock/gmock.h>

#include <AVSCommon/SDKInterfaces/DirectiveHandlerInterface.h>
#include <AVSCommon/SDKInterfaces/DirectiveHandlerResultInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace test {

/// A mock adapter that allow us to mock the preHandleDirective since gmock does not work with unique_ptr.
class DirectiveHandlerMockAdapter : public avsCommon::sdkInterfaces::DirectiveHandlerInterface {
public:
    /// @name DirectiveHandlerInterface method.
    /// @{
    void preHandleDirective(
        std::shared_ptr<avsCommon::avs::AVSDirective> directive,
        std::unique_ptr<avsCommon::sdkInterfaces::DirectiveHandlerResultInterface> result) override;
    /// @}

    /**
     * Destructor.
     */
    virtual ~DirectiveHandlerMockAdapter() = default;

    /**
     * Variant of preHandleDirective taking a shared_ptr instead of a unique_ptr.
     *
     * @param directive The @c AVSDirective to be (pre) handled.
     * @param result The object to receive completion/failure notifications.
     */
    virtual void preHandleDirective(
        const std::shared_ptr<avsCommon::avs::AVSDirective>& directive,
        const std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerResultInterface>& result) = 0;
};

/// Mock directive handler.
class MockDirectiveHandler : public DirectiveHandlerMockAdapter {
public:
    MOCK_METHOD1(handleDirectiveImmediately, void(std::shared_ptr<avs::AVSDirective>));
    MOCK_METHOD1(handleDirective, bool(const std::string&));
    MOCK_METHOD1(cancelDirective, void(const std::string&));
    MOCK_METHOD0(onDeregistered, void());
    MOCK_CONST_METHOD0(getConfiguration, avs::DirectiveHandlerConfiguration());
    MOCK_METHOD2(
        preHandleDirective,
        void(
            const std::shared_ptr<avsCommon::avs::AVSDirective>&,
            const std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerResultInterface>&));
};

inline void DirectiveHandlerMockAdapter::preHandleDirective(
    std::shared_ptr<avs::AVSDirective> avsDirective,
    std::unique_ptr<sdkInterfaces::DirectiveHandlerResultInterface> handler) {
    if (handler) {
        preHandleDirective(avsDirective, std::move(handler));
    }
}

}  // namespace test
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKDIRECTIVEHANDLER_H_
