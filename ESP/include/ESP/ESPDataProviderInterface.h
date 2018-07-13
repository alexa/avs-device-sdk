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
#ifndef ALEXA_CLIENT_SDK_ESP_INCLUDE_ESP_ESPDATAPROVIDERINTERFACE_H_
#define ALEXA_CLIENT_SDK_ESP_INCLUDE_ESP_ESPDATAPROVIDERINTERFACE_H_

#include <AIP/ESPData.h>

namespace alexaClientSDK {
namespace esp {

/**
 * The ESPDataProviderInterface should be used to provide ESPData.
 */
class ESPDataProviderInterface {
public:
    /**
     * Destructor
     */
    virtual ~ESPDataProviderInterface() = default;

    /**
     * Retrieve the ESPData from the ESP Library.
     *
     * @return Collected ESPData if ESP is enabled, otherwise it returns ESPData::EMPTY_ESP_DATA.
     */
    virtual capabilityAgents::aip::ESPData getESPData() = 0;

    /**
     * Return whether the ESP is enabled or not.
     *
     * @return @c true if ESP is enabled, else @c false.
     */
    virtual bool isEnabled() const = 0;

    /**
     * Disable ESP and stop the processing thread.
     */
    virtual void disable() = 0;

    /**
     * Enable ESP and starts the processing thread if it wasn't running yet.
     */
    virtual void enable() = 0;
};

}  // namespace esp
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ESP_INCLUDE_ESP_ESPDATAPROVIDERINTERFACE_H_
