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
#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_INCLUDE_SPEAKERMANAGER_SPEAKERMANAGERCONSTANTS_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_INCLUDE_SPEAKERMANAGER_SPEAKERMANAGERCONSTANTS_H_

#include "AVSCommon/AVS/NamespaceAndName.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace speakerManager {

/// The Speaker interface namespace.
const std::string NAMESPACE = "Speaker";

/// The @c SetVolume directive identifier.
const avsCommon::avs::NamespaceAndName SET_VOLUME{NAMESPACE, "SetVolume"};

/// The @c AdjustVolume directive identifier.
const avsCommon::avs::NamespaceAndName ADJUST_VOLUME{NAMESPACE, "AdjustVolume"};

/// The @c SetMute directive directive identifier.
const avsCommon::avs::NamespaceAndName SET_MUTE{NAMESPACE, "SetMute"};

/// The @c Volume State for use with the context.
const avsCommon::avs::NamespaceAndName VOLUME_STATE{NAMESPACE, "VolumeState"};

/// The @c Volume Key in AVS Directives and Events.
const char VOLUME_KEY[] = "volume";

/// The @c Mute Key. For directives.
const char MUTE_KEY[] = "mute";

/// The @c Muted Key. For events
const char MUTED_KEY[] = "muted";

/// The @c VolumeChanged event key.
const std::string VOLUME_CHANGED = "VolumeChanged";

/// The @c VolumeMute event.
const std::string MUTE_CHANGED = "MuteChanged";

}  // namespace speakerManager
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_INCLUDE_SPEAKERMANAGER_SPEAKERMANAGERCONSTANTS_H_
