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
#include "AVSCommon/SDKInterfaces/Bluetooth/Services/A2DPSinkInterface.h"
#include "AVSCommon/SDKInterfaces/Bluetooth/Services/A2DPSourceInterface.h"
#include "AVSCommon/SDKInterfaces/Bluetooth/Services/AVRCPControllerInterface.h"
#include "AVSCommon/SDKInterfaces/Bluetooth/Services/AVRCPTargetInterface.h"

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

A2DPSourceRecord::A2DPSourceRecord(const std::string& version) :
        SDPRecord{A2DPSourceInterface::NAME, A2DPSourceInterface::UUID, version} {
}

A2DPSinkRecord::A2DPSinkRecord(const std::string& version) :
        SDPRecord{A2DPSinkInterface::NAME, A2DPSinkInterface::UUID, version} {
}

AVRCPTargetRecord::AVRCPTargetRecord(const std::string& version) :
        SDPRecord{AVRCPTargetInterface::NAME, AVRCPTargetInterface::UUID, version} {
}

AVRCPControllerRecord::AVRCPControllerRecord(const std::string& version) :
        SDPRecord{AVRCPControllerInterface::NAME, AVRCPControllerInterface::UUID, version} {
}

}  // namespace bluetooth
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
