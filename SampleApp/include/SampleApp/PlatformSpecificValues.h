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

#ifndef ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_PLATFORMSPECIFICVALUES_H_
#define ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_PLATFORMSPECIFICVALUES_H_

#if defined(ANDROID_MEDIA_PLAYER) || defined(ANDROID_MICROPHONE)
#include <AndroidUtilities/AndroidSLESEngine.h>
#include <AndroidUtilities/PlatformSpecificValues.h>
#endif

namespace alexaClientSDK {
namespace sampleApp {

/*
 * This struct contains platform-specific values required by other components in the SampleApp. The struct is
 * passed to @c SampleApplicationComponent in order to instantiate components there.
 *
 * For example, when building for Android, media players and the microphone require the AndroidSLESEngine, so
 * we include the openSlEngine when the SDK is built for Android. Otherwise, the AndroidSLESEngine
 * and related Android utilities are not compiled with the SDK nor included in any of the components.
 *
 * Other platform-specific values can be added as required.
 */
#if defined(ANDROID_MEDIA_PLAYER) || defined(ANDROID_MICROPHONE)
using PlatformSpecificValues = applicationUtilities::androidUtilities::PlatformSpecificValues;
#else
struct PlatformSpecificValues {};
#endif

}  // namespace sampleApp
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_PLATFORMSPECIFICVALUES_H_
