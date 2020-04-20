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

#include <AVSCommon/Utils/Logger/Logger.h>

#include "BlueZ/BlueZA2DPSource.h"
#include "BlueZ/BlueZDeviceManager.h"
#include "BlueZ/MediaEndpoint.h"

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

using namespace avsCommon::utils;
using namespace avsCommon::sdkInterfaces::bluetooth::services;

/// String to identify log entries originating from this file.
static const std::string TAG{"BlueZA2DPSource"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<BlueZA2DPSource> BlueZA2DPSource::create(std::shared_ptr<BlueZDeviceManager> deviceManager) {
    if (nullptr == deviceManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "deviceManager is null"));
        return nullptr;
    }
    return std::shared_ptr<BlueZA2DPSource>(new BlueZA2DPSource(deviceManager));
}

std::shared_ptr<avsCommon::utils::bluetooth::FormattedAudioStreamAdapter> BlueZA2DPSource::getSourceStream() {
    auto endpoint = m_deviceManager->getMediaEndpoint();
    if (!endpoint) {
        ACSDK_ERROR(LX("getSourceStreamFailed").d("reason", "Failed to get media endpoint"));
        return nullptr;
    }

    return endpoint->getAudioStream();
}

void BlueZA2DPSource::setup() {
}

void BlueZA2DPSource::cleanup() {
}

std::shared_ptr<SDPRecordInterface> BlueZA2DPSource::getRecord() {
    return m_record;
}

BlueZA2DPSource::BlueZA2DPSource(std::shared_ptr<BlueZDeviceManager> deviceManager) :
        m_record{std::make_shared<bluetooth::A2DPSourceRecord>("")},
        m_deviceManager{deviceManager} {
}

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK
