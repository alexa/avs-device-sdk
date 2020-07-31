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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_TOGGLECONTROLLER_TOGGLECONTROLLERATTRIBUTES_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_TOGGLECONTROLLER_TOGGLECONTROLLERATTRIBUTES_H_

#include <AVSCommon/AVS/CapabilityResources.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace toggleController {

/**
 * The toggle controller attribute containing the capability resources required for
 * Capability Agent discovery.
 *
 * @see https://developer.amazon.com/docs/alexa/alexa-voice-service/alexa-togglecontroller.html#capability-assertion
 */
using ToggleControllerAttributes = avsCommon::avs::CapabilityResources;

}  // namespace toggleController
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_TOGGLECONTROLLER_TOGGLECONTROLLERATTRIBUTES_H_
