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

#include "Bluetooth/BluetoothAVRCPTransformer.h"

#include <AVSCommon/SDKInterfaces/Bluetooth/Services/AVRCPTargetInterface.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace bluetooth {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::bluetooth;
using namespace avsCommon::sdkInterfaces::bluetooth::services;
using namespace avsCommon::utils::bluetooth;

/// String to identify log entries originating from this file.
static const std::string TAG{"BluetoothAVRCPTransformer"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<BluetoothAVRCPTransformer> BluetoothAVRCPTransformer::create(
    std::shared_ptr<BluetoothEventBus> eventBus,
    std::shared_ptr<PlaybackRouterInterface> playbackRouter) {
    ACSDK_DEBUG5(LX(__func__));

    if (!eventBus) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullEventBus"));
    } else if (!playbackRouter) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullPlaybackRouter"));
    } else {
        auto avrcpTransformer =
            std::shared_ptr<BluetoothAVRCPTransformer>(new BluetoothAVRCPTransformer(eventBus, playbackRouter));

        if (avrcpTransformer->init()) {
            return avrcpTransformer;
        } else {
            ACSDK_ERROR(LX(__func__).d("reason", "initFailed"));
        }
    }

    return nullptr;
}

BluetoothAVRCPTransformer::BluetoothAVRCPTransformer(
    std::shared_ptr<BluetoothEventBus> eventBus,
    std::shared_ptr<PlaybackRouterInterface> playbackRouter) :
        m_eventBus{eventBus},
        m_playbackRouter{playbackRouter} {
}

bool BluetoothAVRCPTransformer::init() {
    ACSDK_DEBUG5(LX(__func__));
    m_eventBus->addListener(
        {avsCommon::utils::bluetooth::BluetoothEventType::AVRCP_COMMAND_RECEIVED}, shared_from_this());

    return true;
}

void BluetoothAVRCPTransformer::onEventFired(const avsCommon::utils::bluetooth::BluetoothEvent& event) {
    ACSDK_DEBUG5(LX(__func__));
    if (BluetoothEventType::AVRCP_COMMAND_RECEIVED != event.getType()) {
        ACSDK_ERROR(LX(__func__).d("reason", "unexpectedEventReceived"));
        return;
    }

    std::shared_ptr<AVRCPCommand> avrcp = event.getAVRCPCommand();
    if (!avrcp) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullAVRCPCommand"));
        return;
    }

    switch (*avrcp) {
        case AVRCPCommand::PLAY:
            m_playbackRouter->buttonPressed(PlaybackButton::PLAY);
            break;
        case AVRCPCommand::PAUSE:
            m_playbackRouter->buttonPressed(PlaybackButton::PAUSE);
            break;
        case AVRCPCommand::NEXT:
            m_playbackRouter->buttonPressed(PlaybackButton::NEXT);
            break;
        case AVRCPCommand::PREVIOUS:
            m_playbackRouter->buttonPressed(PlaybackButton::PREVIOUS);
            break;
        default:
            ACSDK_ERROR(LX(__func__).d("reason", "commandNotSupported").d("command", *avrcp));
            return;
    }
}

}  // namespace bluetooth
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
