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
#include "BlueZ/BlueZAVRCPController.h"

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

using namespace avsCommon::utils;
using namespace avsCommon::sdkInterfaces::bluetooth::services;

/// String to identify log entries originating from this file.
static const std::string TAG{"BlueZAVRCPController"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<BlueZAVRCPController> BlueZAVRCPController::create() {
    ACSDK_DEBUG5(LX(__func__));

    return std::shared_ptr<BlueZAVRCPController>(new BlueZAVRCPController());
}

BlueZAVRCPController::BlueZAVRCPController() : m_record{std::make_shared<bluetooth::AVRCPControllerRecord>("")} {
}

std::shared_ptr<SDPRecordInterface> BlueZAVRCPController::getRecord() {
    return m_record;
}

void BlueZAVRCPController::setup() {
}

void BlueZAVRCPController::cleanup() {
}

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK
