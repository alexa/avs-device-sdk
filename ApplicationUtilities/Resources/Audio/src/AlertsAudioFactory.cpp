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

#include "Audio/AlertsAudioFactory.h"

#include <AVSCommon/Utils/Stream/StreamFunctions.h>

#include "Audio/Data/med_system_alerts_melodic_01._TTH_.mp3.h"
#include "Audio/Data/med_system_alerts_melodic_01_short._TTH_.wav.h"
#include "Audio/Data/med_system_alerts_melodic_02._TTH_.mp3.h"
#include "Audio/Data/med_system_alerts_melodic_02_short._TTH_.wav.h"

namespace alexaClientSDK {
namespace applicationUtilities {
namespace resources {
namespace audio {

static std::unique_ptr<std::istream> alarmDefaultFactory() {
    return avsCommon::utils::stream::streamFromData(
        data::med_system_alerts_melodic_01__TTH__mp3, sizeof(data::med_system_alerts_melodic_01__TTH__mp3));
}
static std::unique_ptr<std::istream> alarmShortFactory() {
    return avsCommon::utils::stream::streamFromData(
        data::med_system_alerts_melodic_01_short__TTH__wav, sizeof(data::med_system_alerts_melodic_01_short__TTH__wav));
}

static std::unique_ptr<std::istream> timerDefaultFactory() {
    return avsCommon::utils::stream::streamFromData(
        data::med_system_alerts_melodic_02__TTH__mp3, sizeof(data::med_system_alerts_melodic_02__TTH__mp3));
}
static std::unique_ptr<std::istream> timerShortFactory() {
    return avsCommon::utils::stream::streamFromData(
        data::med_system_alerts_melodic_02_short__TTH__wav, sizeof(data::med_system_alerts_melodic_02_short__TTH__wav));
}

static std::unique_ptr<std::istream> reminderDefaultFactory() {
    return avsCommon::utils::stream::streamFromData(
        data::med_system_alerts_melodic_01__TTH__mp3, sizeof(data::med_system_alerts_melodic_01__TTH__mp3));
}
static std::unique_ptr<std::istream> reminderShortFactory() {
    return avsCommon::utils::stream::streamFromData(
        data::med_system_alerts_melodic_01_short__TTH__wav, sizeof(data::med_system_alerts_melodic_01_short__TTH__wav));
}

std::function<std::unique_ptr<std::istream>()> AlertsAudioFactory::alarmDefault() const {
    return alarmDefaultFactory;
}
std::function<std::unique_ptr<std::istream>()> AlertsAudioFactory::alarmShort() const {
    return alarmShortFactory;
}

std::function<std::unique_ptr<std::istream>()> AlertsAudioFactory::timerDefault() const {
    return timerDefaultFactory;
}
std::function<std::unique_ptr<std::istream>()> AlertsAudioFactory::timerShort() const {
    return timerShortFactory;
}

std::function<std::unique_ptr<std::istream>()> AlertsAudioFactory::reminderDefault() const {
    return reminderDefaultFactory;
}
std::function<std::unique_ptr<std::istream>()> AlertsAudioFactory::reminderShort() const {
    return reminderShortFactory;
}

}  // namespace audio
}  // namespace resources
}  // namespace applicationUtilities
}  // namespace alexaClientSDK
