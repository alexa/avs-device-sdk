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

#include <ESP/DummyESPDataProvider.h>

namespace alexaClientSDK {
namespace esp {

DummyESPDataProvider::DummyESPDataProvider() : m_enabled{false} {
}

capabilityAgents::aip::ESPData DummyESPDataProvider::getESPData() {
    return capabilityAgents::aip::ESPData(m_voiceEnergy, m_ambientEnergy);
}

bool DummyESPDataProvider::isEnabled() const {
    return m_enabled;
}

void DummyESPDataProvider::disable() {
    m_enabled = false;
    m_ambientEnergy.clear();
    m_voiceEnergy.clear();
}

void DummyESPDataProvider::enable() {
    m_enabled = true;
}

void DummyESPDataProvider::setVoiceEnergy(const std::string& voiceEnergy) {
    m_voiceEnergy = voiceEnergy;
}

void DummyESPDataProvider::setAmbientEnergy(const std::string& ambientEnergy) {
    m_ambientEnergy = ambientEnergy;
}

}  // namespace esp
}  // namespace alexaClientSDK
