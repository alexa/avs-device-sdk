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

#include "AVSCommon/Utils/Bluetooth/SDPRecords.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace bluetooth {

using namespace avsCommon::sdkInterfaces::bluetooth::services;

SDPRecord::SDPRecord(const std::string& name, const std::string& uuid, const std::string& version) :
        m_name{name},
        m_uuid{uuid},
        m_version{version} {
}

std::string SDPRecord::getName() const {
    return m_name;
}

std::string SDPRecord::getUuid() const {
    return m_uuid;
}

std::string SDPRecord::getVersion() const {
    return m_version;
}

// Constants are defined as part of the Bluetooth specification.

// A2DPSource.
const std::string A2DPSourceRecord::UUID = "0000110a-0000-1000-8000-00805f9b34fb";
const std::string A2DPSourceRecord::NAME = "AudioSource";

A2DPSourceRecord::A2DPSourceRecord(const std::string& version) :
        SDPRecord{A2DPSourceRecord::NAME, A2DPSourceRecord::UUID, version} {
}

// A2DPSink.
const std::string A2DPSinkRecord::UUID = "0000110b-0000-1000-8000-00805f9b34fb";
const std::string A2DPSinkRecord::NAME = "AudioSink";

A2DPSinkRecord::A2DPSinkRecord(const std::string& version) :
        SDPRecord{A2DPSinkRecord::NAME, A2DPSinkRecord::UUID, version} {
}

// AVRCPTargetRecord.
const std::string AVRCPTargetRecord::UUID = "0000110c-0000-1000-8000-00805f9b34fb";
const std::string AVRCPTargetRecord::NAME = "A/V_RemoteControlTarget";

AVRCPTargetRecord::AVRCPTargetRecord(const std::string& version) :
        SDPRecord{AVRCPTargetRecord::NAME, AVRCPTargetRecord::UUID, version} {
}

}  // namespace bluetooth
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
