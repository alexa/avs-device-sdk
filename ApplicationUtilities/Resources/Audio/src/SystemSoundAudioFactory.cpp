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

#include <AVSCommon/Utils/Stream/StreamFunctions.h>

#include "Audio/Data/med_ui_endpointing.wav.h"
#include "Audio/Data/med_ui_wakesound.wav.h"
#include "Audio/SystemSoundAudioFactory.h"

namespace alexaClientSDK {
namespace applicationUtilities {
namespace resources {
namespace audio {

static std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType> wakeWordNotificationToneFactory() {
    return std::make_pair(
        avsCommon::utils::stream::streamFromData(data::med_ui_wakesound_wav, sizeof(data::med_ui_wakesound_wav)),
        avsCommon::utils::MimeTypeToMediaType(data::med_ui_wakesound_wav_mimetype));
}
static std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType> endSpeechToneFactory() {
    return std::make_pair(
        avsCommon::utils::stream::streamFromData(data::med_ui_endpointing_wav, sizeof(data::med_ui_endpointing_wav)),
        avsCommon::utils::MimeTypeToMediaType(data::med_ui_endpointing_wav_mimetype));
}

std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()> SystemSoundAudioFactory::
    wakeWordNotificationTone() const {
    return wakeWordNotificationToneFactory;
}

std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()> SystemSoundAudioFactory::
    endSpeechTone() const {
    return endSpeechToneFactory;
}

}  // namespace audio
}  // namespace resources
}  // namespace applicationUtilities
}  // namespace alexaClientSDK
