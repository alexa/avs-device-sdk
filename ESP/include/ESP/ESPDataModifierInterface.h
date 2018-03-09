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
#ifndef ALEXA_CLIENT_SDK_ESP_INCLUDE_ESP_ESPDATAMODIFIERINTERFACE_H_
#define ALEXA_CLIENT_SDK_ESP_INCLUDE_ESP_ESPDATAMODIFIERINTERFACE_H_

#include <string>

#include <AIP/ESPData.h>

namespace alexaClientSDK {
namespace esp {

/**
 * The ESPDataModifierInterface is a debugging interface to allow modifying the ESPData in the DummyESPDataProvider.
 */
class ESPDataModifierInterface {
public:
    /**
     * Destructor
     */
    virtual ~ESPDataModifierInterface() = default;

    /**
     * Set new voice energy.
     *
     * @param voiceEnergy New voice energy value.
     */
    virtual void setVoiceEnergy(const std::string& voiceEnergy) = 0;

    /**
     * Set new ambient energy.
     *
     * @param ambientEnergy New ambient energy value.
     */
    virtual void setAmbientEnergy(const std::string& ambientEnergy) = 0;
};

}  // namespace esp
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ESP_INCLUDE_ESP_ESPDATAMODIFIERINTERFACE_H_
