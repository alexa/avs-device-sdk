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

#include <AVSCommon/Utils/Logger/Logger.h>
#include "BlueZ/BlueZAVRCPTarget.h"

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

using namespace avsCommon::utils;
using namespace avsCommon::sdkInterfaces::bluetooth::services;

/// String to identify log entries originating from this file.
static const std::string TAG{"BlueZAVRCPTarget"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The AVRCP Play command.
static const std::string PLAY_CMD = "Play";

/// The AVRCP Play command.
static const std::string PAUSE_CMD = "Pause";

/// The AVRCP Play command.
static const std::string NEXT_CMD = "Next";

/// The AVRCP Play command.
static const std::string PREVIOUS_CMD = "Previous";

std::shared_ptr<BlueZAVRCPTarget> BlueZAVRCPTarget::create(std::shared_ptr<DBusProxy> mediaControlProxy) {
    ACSDK_DEBUG5(LX(__func__));

    if (!mediaControlProxy) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullMediaControlProxy"));
        return nullptr;
    }

    return std::shared_ptr<BlueZAVRCPTarget>(new BlueZAVRCPTarget(mediaControlProxy));
}

BlueZAVRCPTarget::BlueZAVRCPTarget(std::shared_ptr<DBusProxy> mediaControlProxy) :
        m_record{std::make_shared<bluetooth::AVRCPTargetRecord>("")},
        m_mediaControlProxy{mediaControlProxy} {
}

std::shared_ptr<SDPRecordInterface> BlueZAVRCPTarget::getRecord() {
    return m_record;
}

void BlueZAVRCPTarget::setup() {
}

void BlueZAVRCPTarget::cleanup() {
}

bool BlueZAVRCPTarget::play() {
    ACSDK_DEBUG5(LX(__func__));
    std::lock_guard<std::mutex> lock(m_cmdMutex);

    ManagedGError error;
    m_mediaControlProxy->callMethod(PLAY_CMD, nullptr, error.toOutputParameter());

    if (error.hasError()) {
        ACSDK_ERROR(LX(__func__).d("error", error.getMessage()));
        return false;
    }

    return true;
}

bool BlueZAVRCPTarget::pause() {
    ACSDK_DEBUG5(LX(__func__));
    std::lock_guard<std::mutex> lock(m_cmdMutex);

    ManagedGError error;
    m_mediaControlProxy->callMethod(PAUSE_CMD, nullptr, error.toOutputParameter());

    if (error.hasError()) {
        ACSDK_ERROR(LX(__func__).d("error", error.getMessage()));
        return false;
    }

    return true;
}

bool BlueZAVRCPTarget::next() {
    ACSDK_DEBUG5(LX(__func__));
    std::lock_guard<std::mutex> lock(m_cmdMutex);

    ManagedGError error;
    m_mediaControlProxy->callMethod(NEXT_CMD, nullptr, error.toOutputParameter());

    if (error.hasError()) {
        ACSDK_ERROR(LX(__func__).d("error", error.getMessage()));
        return false;
    }

    return true;
}

bool BlueZAVRCPTarget::previous() {
    ACSDK_DEBUG5(LX(__func__));
    std::lock_guard<std::mutex> lock(m_cmdMutex);

    ManagedGError error;
    m_mediaControlProxy->callMethod(PREVIOUS_CMD, nullptr, error.toOutputParameter());

    if (error.hasError()) {
        ACSDK_ERROR(LX(__func__).d("error", error.getMessage()));
        return false;
    }

    return true;
}

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK
