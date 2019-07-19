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

#include "AIP/ESPData.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace aip {

ESPData::ESPData(const std::string& voiceEnergy, const std::string& ambientEnergy) :
        m_voiceEnergy{voiceEnergy},
        m_ambientEnergy{ambientEnergy} {
}

const ESPData ESPData::getEmptyESPData() {
    static const ESPData emptyESPData{};
    return emptyESPData;
}

std::string ESPData::getVoiceEnergy() const {
    return m_voiceEnergy;
}

std::string ESPData::getAmbientEnergy() const {
    return m_ambientEnergy;
}

bool ESPData::verify() const {
    return verifyHelper(m_voiceEnergy) && verifyHelper(m_ambientEnergy);
}

bool ESPData::isEmpty() const {
    return m_voiceEnergy.empty() && m_ambientEnergy.empty();
}

bool ESPData::verifyHelper(const std::string& valueToVerify) {
    bool isValid = false;
    if (!valueToVerify.empty()) {
        isValid = true;
        for (auto c : valueToVerify) {
            if (!std::isalnum(c) && c != '.' && c != '-' && c != '+') {
                isValid = false;
                break;
            }
        }
    }
    return isValid;
}

}  // namespace aip
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
