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
#ifndef SHARED_ACSDKSHUTDOWNMANAGERINTERFACES_TEST_ACSDKSHUTDOWNMANAGERINTERFACES_MOCKSHUTDOWNNOTIFIER_H_
#define SHARED_ACSDKSHUTDOWNMANAGERINTERFACES_TEST_ACSDKSHUTDOWNMANAGERINTERFACES_MOCKSHUTDOWNNOTIFIER_H_

#include <gmock/gmock.h>

#include <acsdkShutdownManagerInterfaces/ShutdownNotifierInterface.h>

namespace alexaClientSDK {
namespace acsdkShutdownManagerInterfaces {
namespace test {

/**
 * Mock class that implements ShutdownNotifierInterface.
 */
class MockShutdownNotifier : public acsdkShutdownManagerInterfaces::ShutdownNotifierInterface {
public:
    MOCK_METHOD1(addObserver, void(const std::shared_ptr<avsCommon::utils::RequiresShutdown>& observer));

    MOCK_METHOD1(removeObserver, void(const std::shared_ptr<avsCommon::utils::RequiresShutdown>& observer));

    MOCK_METHOD1(addWeakPtrObserver, void(const std::weak_ptr<avsCommon::utils::RequiresShutdown>& observer));

    MOCK_METHOD1(removeWeakPtrObserver, void(const std::weak_ptr<avsCommon::utils::RequiresShutdown>& observer));

    MOCK_METHOD1(
        notifyObservers,
        void(std::function<void(const std::shared_ptr<avsCommon::utils::RequiresShutdown>&)>));

    MOCK_METHOD1(
        notifyObserversInReverse,
        bool(std::function<void(const std::shared_ptr<avsCommon::utils::RequiresShutdown>&)>));

    MOCK_METHOD1(
        setAddObserverFunction,
        void(std::function<void(const std::shared_ptr<avsCommon::utils::RequiresShutdown>&)>));
};

}  // namespace test
}  // namespace acsdkShutdownManagerInterfaces
}  // namespace alexaClientSDK

#endif  // SHARED_ACSDKSHUTDOWNMANAGERINTERFACES_TEST_ACSDKSHUTDOWNMANAGERINTERFACES_MOCKSHUTDOWNNOTIFIER_H_