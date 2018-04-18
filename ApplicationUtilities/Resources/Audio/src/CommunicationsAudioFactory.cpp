/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "Audio/CommunicationsAudioFactory.h"

#include <AVSCommon/Utils/Stream/StreamFunctions.h>

#include <Audio/Data/system_comm_call_connected_.mp3.h>
#include <Audio/Data/system_comm_call_disconnected_.mp3.h>
#include <Audio/Data/system_comm_drop_in_connected_.mp3.h>
#include <Audio/Data/system_comm_outbound_ringtone_.mp3.h>
#include <Audio/Data/system_comm_call_incoming_ringtone_intro_.mp3.h>

namespace alexaClientSDK {
namespace applicationUtilities {
namespace resources {
namespace audio {

static std::unique_ptr<std::istream> callConnectedRingtoneFactory() {
    return avsCommon::utils::stream::streamFromData(
        data::system_comm_call_connected_mp3, sizeof(data::system_comm_call_connected_mp3));
}

static std::unique_ptr<std::istream> callDisconnectedRingtoneFactory() {
    return avsCommon::utils::stream::streamFromData(
        data::system_comm_call_disconnected_mp3, sizeof(data::system_comm_call_disconnected_mp3));
}

static std::unique_ptr<std::istream> outboundRingtoneFactory() {
    return avsCommon::utils::stream::streamFromData(
        data::system_comm_outbound_ringtone_mp3, sizeof(data::system_comm_outbound_ringtone_mp3));
}

static std::unique_ptr<std::istream> dropInConnectedRingtoneFactory() {
    return avsCommon::utils::stream::streamFromData(
        data::system_comm_drop_in_connected_mp3, sizeof(data::system_comm_drop_in_connected_mp3));
}

static std::unique_ptr<std::istream> callIncomingRingtoneFactory() {
    return avsCommon::utils::stream::streamFromData(
        data::system_comm_call_incoming_ringtone_intro_mp3, sizeof(data::system_comm_call_incoming_ringtone_intro_mp3));
}

std::function<std::unique_ptr<std::istream>()> CommunicationsAudioFactory::callConnectedRingtone() const {
    return callConnectedRingtoneFactory;
}

std::function<std::unique_ptr<std::istream>()> CommunicationsAudioFactory::callDisconnectedRingtone() const {
    return callDisconnectedRingtoneFactory;
}

std::function<std::unique_ptr<std::istream>()> CommunicationsAudioFactory::outboundRingtone() const {
    return outboundRingtoneFactory;
}

std::function<std::unique_ptr<std::istream>()> CommunicationsAudioFactory::dropInConnectedRingtone() const {
    return dropInConnectedRingtoneFactory;
}

std::function<std::unique_ptr<std::istream>()> CommunicationsAudioFactory::callIncomingRingtone() const {
    return callIncomingRingtoneFactory;
}

}  // namespace audio
}  // namespace resources
}  // namespace applicationUtilities
}  // namespace alexaClientSDK
