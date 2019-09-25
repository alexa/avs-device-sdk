/*
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <AVSCommon/Utils/Stream/StreamFunctions.h>

#include "Audio/Data/med_ui_endpointing.wav.h"
#include "Audio/Data/med_ui_wakesound.wav.h"
#include "Audio/SystemSoundAudioFactory.h"

namespace alexaClientSDK {
namespace applicationUtilities {
namespace resources {
namespace audio {

static std::unique_ptr<std::istream> wakeWordNotificationToneFactory() {
    return avsCommon::utils::stream::streamFromData(data::med_ui_wakesound_wav, sizeof(data::med_ui_wakesound_wav));
}
static std::unique_ptr<std::istream> endSpeechToneFactory() {
    return avsCommon::utils::stream::streamFromData(data::med_ui_endpointing_wav, sizeof(data::med_ui_endpointing_wav));
}

std::function<std::unique_ptr<std::istream>()> SystemSoundAudioFactory::wakeWordNotificationTone() const {
    return wakeWordNotificationToneFactory;
}

std::function<std::unique_ptr<std::istream>()> SystemSoundAudioFactory::endSpeechTone() const {
    return endSpeechToneFactory;
}

}  // namespace audio
}  // namespace resources
}  // namespace applicationUtilities
}  // namespace alexaClientSDK
