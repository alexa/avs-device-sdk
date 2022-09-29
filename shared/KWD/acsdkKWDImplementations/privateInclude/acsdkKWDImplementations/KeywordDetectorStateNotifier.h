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

#ifndef ACSDKKWDIMPLEMENTATIONS_KEYWORDDETECTORSTATENOTIFIER_H_
#define ACSDKKWDIMPLEMENTATIONS_KEYWORDDETECTORSTATENOTIFIER_H_

#include <memory>

#include <acsdkKWDInterfaces/KeywordDetectorStateNotifierInterface.h>
#include <acsdk/Notifier/Notifier.h>
#include <AVSCommon/SDKInterfaces/KeyWordDetectorStateObserverInterface.h>

namespace alexaClientSDK {
namespace acsdkKWDImplementations {

/**
 * Relays notifications related to KeywordDetectorState.
 */
class KeywordDetectorStateNotifier
        : public notifier::Notifier<avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface> {
public:
    /**
     * Factory method.
     * @return A new instance of @c KeywordDetectorStateNotifierInterface.
     */
    static std::shared_ptr<acsdkKWDInterfaces::KeywordDetectorStateNotifierInterface>
    createKeywordDetectorStateNotifierInterface();
};

}  // namespace acsdkKWDImplementations
}  // namespace alexaClientSDK

#endif  // ACSDKKWDIMPLEMENTATIONS_KEYWORDDETECTORSTATENOTIFIER_H_
