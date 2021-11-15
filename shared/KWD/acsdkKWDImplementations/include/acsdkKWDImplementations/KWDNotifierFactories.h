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

#ifndef ACSDKKWDIMPLEMENTATIONS_KWDNOTIFIERFACTORIES_H_
#define ACSDKKWDIMPLEMENTATIONS_KWDNOTIFIERFACTORIES_H_

#include <memory>

#include <acsdkKWDInterfaces/KeywordDetectorStateNotifierInterface.h>
#include <acsdkKWDInterfaces/KeywordNotifierInterface.h>

namespace alexaClientSDK {
namespace acsdkKWDImplementations {

/**
 * This class produces a @c KeywordNotifierInterface and @c KeywordDetectorStateNotifierInterface.
 */
class KWDNotifierFactories {
public:
    static std::shared_ptr<acsdkKWDInterfaces::KeywordDetectorStateNotifierInterface>
    createKeywordDetectorStateNotifier();

    static std::shared_ptr<acsdkKWDInterfaces::KeywordNotifierInterface> createKeywordNotifier();
};

}  // namespace acsdkKWDImplementations
}  // namespace alexaClientSDK

#endif  // ACSDKKWDIMPLEMENTATIONS_KWDNOTIFIERFACTORIES_H_
