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
#ifndef ACSDKNOTIFIERINTERFACES_INTERNAL_MOCKNOTIFIER_H_
#define ACSDKNOTIFIERINTERFACES_INTERNAL_MOCKNOTIFIER_H_

#include <gmock/gmock.h>

#include <acsdkNotifierInterfaces/internal/NotifierInterface.h>

namespace alexaClientSDK {
namespace acsdkNotifierInterfaces {
namespace test {

/**
 * Mock class that implements NotifierInterface.
 *
 * To make Expect calls against the observer you will need to set a ON_CALL WillByDefault assertion that will pass the
 * args to the observer.
 *
 * ON_CALL(MockNotifier, notifyObservers(_))
            .WillByDefault(Invoke(
                [this](std::function<void(
                           const std::shared_ptr<ObserverType>&)>
                           notifyFn) { notifyFn(observer); }));
 */
template <typename ObserverType>
class MockNotifier : public acsdkNotifierInterfaces::NotifierInterface<ObserverType> {
public:
    MOCK_METHOD1_T(addObserver, void(const std::shared_ptr<ObserverType>& observer));

    MOCK_METHOD1_T(removeObserver, void(const std::shared_ptr<ObserverType>& observer));

    MOCK_METHOD1_T(addWeakPtrObserver, void(const std::weak_ptr<ObserverType>& observer));

    MOCK_METHOD1_T(removeWeakPtrObserver, void(const std::weak_ptr<ObserverType>& observer));

    MOCK_METHOD1_T(notifyObservers, void(std::function<void(const std::shared_ptr<ObserverType>&)>));

    MOCK_METHOD1_T(notifyObserversInReverse, bool(std::function<void(const std::shared_ptr<ObserverType>&)>));

    MOCK_METHOD1_T(setAddObserverFunction, void(std::function<void(const std::shared_ptr<ObserverType>&)>));
};

}  // namespace test
}  // namespace acsdkNotifierInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKNOTIFIERINTERFACES_INTERNAL_MOCKNOTIFIER_H_
