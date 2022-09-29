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

#ifndef SHARED_ACSDKSTARTUPMANAGERINTERFACES_TEST_ACSDKSTARTUPMANAGERINTERFACES_MOCKSTARTUPNOTIFIER_H_
#define SHARED_ACSDKSTARTUPMANAGERINTERFACES_TEST_ACSDKSTARTUPMANAGERINTERFACES_MOCKSTARTUPNOTIFIER_H_

#include <acsdkStartupManagerInterfaces/StartupNotifierInterface.h>

#include <gtest/gtest.h>

namespace alexaClientSDK {
namespace acsdkStartupManagerInterfaces {
namespace test {

/// Mock class for StartupNotifierInterface
class MockStartupNotifier : public StartupNotifierInterface {
public:
    MOCK_METHOD1(
        addObserver,
        void(const std::shared_ptr<acsdkStartupManagerInterfaces::RequiresStartupInterface>& observer));
    MOCK_METHOD1(
        removeObserver,
        void(const std::shared_ptr<acsdkStartupManagerInterfaces::RequiresStartupInterface>& observer));
    MOCK_METHOD1(
        addWeakPtrObserver,
        void(const std::weak_ptr<acsdkStartupManagerInterfaces::RequiresStartupInterface>& observer));
    MOCK_METHOD1(
        removeWeakPtrObserver,
        void(const std::weak_ptr<acsdkStartupManagerInterfaces::RequiresStartupInterface>& observer));
    MOCK_METHOD1(
        notifyObservers,
        void(std::function<void(const std::shared_ptr<acsdkStartupManagerInterfaces::RequiresStartupInterface>&)>));
    MOCK_METHOD1(
        notifyObserversInReverse,
        bool(std::function<void(const std::shared_ptr<acsdkStartupManagerInterfaces::RequiresStartupInterface>&)>));
    MOCK_METHOD1(
        setAddObserverFunction,
        void(std::function<void(const std::shared_ptr<acsdkStartupManagerInterfaces::RequiresStartupInterface>&)>));
};

}  // namespace test
}  // namespace acsdkStartupManagerInterfaces
}  // namespace alexaClientSDK

#endif  // SHARED_ACSDKSTARTUPMANAGERINTERFACES_TEST_ACSDKSTARTUPMANAGERINTERFACES_MOCKSTARTUPNOTIFIER_H_
