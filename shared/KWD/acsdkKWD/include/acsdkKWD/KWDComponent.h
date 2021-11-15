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

#ifndef ACSDKKWD_KWDCOMPONENT_H_
#define ACSDKKWD_KWDCOMPONENT_H_

#include <memory>

#include <acsdkKWDImplementations/AbstractKeywordDetector.h>
#include <acsdkKWDInterfaces/KeywordDetectorStateNotifierInterface.h>
#include <acsdkKWDInterfaces/KeywordNotifierInterface.h>
#include <acsdkManufactory/Annotated.h>
#include <acsdkManufactory/Component.h>
#include <acsdkManufactory/Import.h>
#include <acsdkManufactory/OptionalImport.h>
#include <AVSCommon/AVS/AudioInputStream.h>
#include <AVSCommon/Utils/AudioFormat.h>

namespace alexaClientSDK {
namespace acsdkKWD {

/**
 * Manufactory Component definition for the @c AbstractKeywordDetector.
 */
using KWDComponent = acsdkManufactory::Component<
    std::shared_ptr<acsdkKWDImplementations::AbstractKeywordDetector>,
    std::shared_ptr<acsdkKWDInterfaces::KeywordNotifierInterface>,
    std::shared_ptr<acsdkKWDInterfaces::KeywordDetectorStateNotifierInterface>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::avs::AudioInputStream>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::utils::AudioFormat>>>;

/**
 * Get the @c Manufactory component for creating an instance of @c AbstractKeywordDetector.
 *
 * @return The @c Manufactory component for creating an instance of @c AbstractKeywordDetector.
 */
KWDComponent getComponent();

}  // namespace acsdkKWD
}  // namespace alexaClientSDK

#endif  // ACSDKKWD_KWDCOMPONENT_H_
