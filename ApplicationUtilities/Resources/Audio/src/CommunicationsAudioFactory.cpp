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

#include "Audio/CommunicationsAudioFactory.h"

#include <AVSCommon/Utils/Stream/StreamFunctions.h>

#include <Audio/Data/med_comms_call_connected.mp3.h>
#include <Audio/Data/med_comms_call_disconnected.mp3.h>
#include <Audio/Data/med_comms_call_incoming_ringtone.mp3.h>
#include <Audio/Data/med_comms_drop_in_incoming.mp3.h>
#include <Audio/Data/med_comms_outbound_ringtone.mp3.h>

namespace alexaClientSDK {
namespace applicationUtilities {
namespace resources {
namespace audio {

using namespace avsCommon::utils;

static std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType> callConnectedRingtoneFactory() {
    return std::make_pair(
        avsCommon::utils::stream::streamFromData(
            data::med_comms_call_connected_mp3, sizeof(data::med_comms_call_connected_mp3)),
        avsCommon::utils::MimeTypeToMediaType(data::med_comms_call_connected_mp3_mimetype));
}

static std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType> callDisconnectedRingtoneFactory() {
    return std::make_pair(
        avsCommon::utils::stream::streamFromData(
            data::med_comms_call_disconnected_mp3, sizeof(data::med_comms_call_disconnected_mp3)),
        avsCommon::utils::MimeTypeToMediaType(data::med_comms_call_disconnected_mp3_mimetype));
}

static std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType> outboundRingtoneFactory() {
    return std::make_pair(
        avsCommon::utils::stream::streamFromData(
            data::med_comms_outbound_ringtone_mp3, sizeof(data::med_comms_outbound_ringtone_mp3)),
        avsCommon::utils::MimeTypeToMediaType(data::med_comms_outbound_ringtone_mp3_mimetype));
}

static std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType> dropInIncomingFactory() {
    return std::make_pair(
        avsCommon::utils::stream::streamFromData(
            data::med_comms_drop_in_incoming_mp3, sizeof(data::med_comms_drop_in_incoming_mp3)),
        avsCommon::utils::MimeTypeToMediaType(data::med_comms_drop_in_incoming_mp3_mimetype));
}

static std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType> callIncomingRingtoneFactory() {
    return std::make_pair(
        avsCommon::utils::stream::streamFromData(
            data::med_comms_call_incoming_ringtone_mp3, sizeof(data::med_comms_call_incoming_ringtone_mp3)),
        avsCommon::utils::MimeTypeToMediaType(data::med_comms_call_incoming_ringtone_mp3_mimetype));
}

std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()>
CommunicationsAudioFactory::callConnectedRingtone() const {
    return callConnectedRingtoneFactory;
}

std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()>
CommunicationsAudioFactory::callDisconnectedRingtone() const {
    return callDisconnectedRingtoneFactory;
}

std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()>
CommunicationsAudioFactory::outboundRingtone() const {
    return outboundRingtoneFactory;
}

std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()>
CommunicationsAudioFactory::dropInIncoming() const {
    return dropInIncomingFactory;
}

std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()>
CommunicationsAudioFactory::callIncomingRingtone() const {
    return callIncomingRingtoneFactory;
}

}  // namespace audio
}  // namespace resources
}  // namespace applicationUtilities
}  // namespace alexaClientSDK
