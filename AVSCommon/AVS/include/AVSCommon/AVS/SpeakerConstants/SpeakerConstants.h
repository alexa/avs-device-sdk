/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

/**
 * @file
 */
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_SPEAKERCONSTANTS_SPEAKERCONSTANTS_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_SPEAKERCONSTANTS_SPEAKERCONSTANTS_H_

#include <cstdint>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace speakerConstants {

/// AVS setVolume Minimum.
const int8_t AVS_SET_VOLUME_MIN = 0;

/// AVS setVolume Maximum.
const int8_t AVS_SET_VOLUME_MAX = 100;

/// AVS adjustVolume Minimum.
const int8_t AVS_ADJUST_VOLUME_MIN = -100;

/// AVS adjustVolume Maximum.
const int8_t AVS_ADJUST_VOLUME_MAX = 100;

}  // namespace speakerConstants
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_SPEAKERCONSTANTS_SPEAKERCONSTANTS_H_
