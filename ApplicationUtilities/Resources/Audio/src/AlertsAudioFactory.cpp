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

#include "Audio/AlertsAudioFactory.h"

#include <AVSCommon/Utils/Stream/StreamFunctions.h>

#include "Audio/Data/med_alerts_notification_03.mp3.h"
#include "Audio/Data/med_system_alerts_melodic_01.mp3.h"
#include "Audio/Data/med_system_alerts_melodic_01_short.wav.h"
#include "Audio/Data/med_system_alerts_melodic_02.mp3.h"
#include "Audio/Data/med_system_alerts_melodic_02_short.wav.h"

namespace alexaClientSDK {
namespace applicationUtilities {
namespace resources {
namespace audio {

static std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType> alarmDefaultFactory() {
    return std::make_pair(
        avsCommon::utils::stream::streamFromData(
            data::med_system_alerts_melodic_01_mp3, sizeof(data::med_system_alerts_melodic_01_mp3)),
        avsCommon::utils::MimeTypeToMediaType(data::med_system_alerts_melodic_01_mp3_mimetype));
}
static std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType> alarmShortFactory() {
    return std::make_pair(
        avsCommon::utils::stream::streamFromData(
            data::med_system_alerts_melodic_01_short_wav, sizeof(data::med_system_alerts_melodic_01_short_wav)),
        avsCommon::utils::MimeTypeToMediaType(data::med_system_alerts_melodic_01_short_wav_mimetype));
}

static std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType> timerDefaultFactory() {
    return std::make_pair(
        avsCommon::utils::stream::streamFromData(
            data::med_system_alerts_melodic_02_mp3, sizeof(data::med_system_alerts_melodic_02_mp3)),
        avsCommon::utils::MimeTypeToMediaType(data::med_system_alerts_melodic_02_mp3_mimetype));
}
static std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType> timerShortFactory() {
    return std::make_pair(
        avsCommon::utils::stream::streamFromData(
            data::med_system_alerts_melodic_02_short_wav, sizeof(data::med_system_alerts_melodic_02_short_wav)),
        avsCommon::utils::MimeTypeToMediaType(data::med_system_alerts_melodic_02_short_wav_mimetype));
}

static std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType> reminderDefaultFactory() {
    return std::make_pair(
        avsCommon::utils::stream::streamFromData(
            data::med_alerts_notification_03_mp3, sizeof(data::med_alerts_notification_03_mp3)),
        avsCommon::utils::MimeTypeToMediaType(data::med_alerts_notification_03_mp3_mimetype));
}
static std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType> reminderShortFactory() {
    return std::make_pair(
        avsCommon::utils::stream::streamFromData(
            data::med_alerts_notification_03_mp3, sizeof(data::med_alerts_notification_03_mp3)),
        avsCommon::utils::MimeTypeToMediaType(data::med_alerts_notification_03_mp3_mimetype));
}

std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()> AlertsAudioFactory::
    alarmDefault() const {
    return alarmDefaultFactory;
}
std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()> AlertsAudioFactory::
    alarmShort() const {
    return alarmShortFactory;
}

std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()> AlertsAudioFactory::
    timerDefault() const {
    return timerDefaultFactory;
}
std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()> AlertsAudioFactory::
    timerShort() const {
    return timerShortFactory;
}

std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()> AlertsAudioFactory::
    reminderDefault() const {
    return reminderDefaultFactory;
}
std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()> AlertsAudioFactory::
    reminderShort() const {
    return reminderShortFactory;
}

}  // namespace audio
}  // namespace resources
}  // namespace applicationUtilities
}  // namespace alexaClientSDK
