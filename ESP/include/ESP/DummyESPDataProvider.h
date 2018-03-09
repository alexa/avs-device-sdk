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
#ifndef ALEXA_CLIENT_SDK_ESP_INCLUDE_ESP_DUMMYESPDATAPROVIDER_H_
#define ALEXA_CLIENT_SDK_ESP_INCLUDE_ESP_DUMMYESPDATAPROVIDER_H_

#include <string>

#include <AIP/ESPData.h>
#include <ESP/ESPDataModifierInterface.h>
#include <ESP/ESPDataProviderInterface.h>

namespace alexaClientSDK {
namespace esp {

/**
 * This is a dummy provider that allows customer to manually test ESP or just to keep it disabled.
 */
class DummyESPDataProvider
        : public ESPDataProviderInterface
        , public ESPDataModifierInterface {
public:
    /**
     * DummyESPDataProvider Constructor.
     */
    DummyESPDataProvider();

    /// @name Overridden ESPDataProviderInterface methods.
    /// @{
    capabilityAgents::aip::ESPData getESPData() override;
    bool isEnabled() const override;
    void disable() override;
    void enable() override;
    /// @}

    /// @name Overridden ESPDataModifierInterface methods.
    /// @{
    void setVoiceEnergy(const std::string& voiceEnergy) override;
    void setAmbientEnergy(const std::string& ambientEnergy) override;
    /// @}

private:
    std::string m_voiceEnergy;
    std::string m_ambientEnergy;
    bool m_enabled;
};

}  // namespace esp
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ESP_INCLUDE_ESP_DUMMYESPDATAPROVIDER_H_
