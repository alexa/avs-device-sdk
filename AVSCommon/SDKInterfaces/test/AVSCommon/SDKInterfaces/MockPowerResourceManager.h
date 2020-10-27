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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKPOWERRESOURCEMANAGER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKPOWERRESOURCEMANAGER_H_

#include <memory>

#include <gmock/gmock.h>

#include "AVSCommon/SDKInterfaces/PowerResourceManagerInterface.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace test {

/**
 * Mock class implementing @c PowerResourceManagerInterface
 */
class MockPowerResourceManager : public PowerResourceManagerInterface {
public:
    MOCK_METHOD2(acquirePowerResource, void(const std::string& component, const PowerResourceLevel level));
    MOCK_METHOD1(releasePowerResource, void(const std::string& component));
    MOCK_METHOD1(isPowerResourceAcquired, bool(const std::string& component));
    MOCK_METHOD3(
        create,
        std::shared_ptr<PowerResourceId>(
            const std::string& resourceId,
            bool isRefCounted,
            const PowerResourceLevel level));
    MOCK_METHOD2(
        acquire,
        bool(const std::shared_ptr<PowerResourceId>& id, const std::chrono::milliseconds autoReleaseTimeout));
    MOCK_METHOD1(release, bool(const std::shared_ptr<PowerResourceId>& id));
    MOCK_METHOD1(close, bool(const std::shared_ptr<PowerResourceId>& id));

    /**
     * Some methods require default behavior to be useful, even in a mock. This function sets those default behaviors.
     */
    void setDefaultBehavior();
};

inline void MockPowerResourceManager::setDefaultBehavior() {
    ON_CALL(*this, create(testing::_, testing::_, testing::_))
        .WillByDefault(
            testing::Invoke([](const std::string& resourceId,
                               bool isRefCounted,
                               const sdkInterfaces::PowerResourceManagerInterface::PowerResourceLevel level) {
                return std::make_shared<PowerResourceManagerInterface::PowerResourceId>(resourceId);
            }));
}

}  // namespace test
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKPOWERRESOURCEMANAGER_H_
