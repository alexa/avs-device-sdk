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

#include "acsdkKWDImplementations/KeywordDetectorStateNotifier.h"
#include "acsdkKWDImplementations/KeywordNotifier.h"
#include "acsdkKWDImplementations/KWDNotifierFactories.h"

namespace alexaClientSDK {
namespace acsdkKWDImplementations {

std::shared_ptr<acsdkKWDInterfaces::KeywordDetectorStateNotifierInterface> KWDNotifierFactories::
    createKeywordDetectorStateNotifier() {
    return KeywordDetectorStateNotifier::createKeywordDetectorStateNotifierInterface();
}

std::shared_ptr<acsdkKWDInterfaces::KeywordNotifierInterface> KWDNotifierFactories::createKeywordNotifier() {
    return KeywordNotifier::createKeywordNotifierInterface();
}

}  // namespace acsdkKWDImplementations
}  // namespace alexaClientSDK
