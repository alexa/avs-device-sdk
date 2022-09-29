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

#ifndef ACSDK_SAMPLE_ENDPOINT_ENDPOINTALEXASEEKCONTROLLERHANDLER_H_
#define ACSDK_SAMPLE_ENDPOINT_ENDPOINTALEXASEEKCONTROLLERHANDLER_H_

#include <chrono>
#include <memory>
#include <mutex>
#include <string>

#include <acsdkAlexaSeekControllerInterfaces/AlexaSeekControllerInterface.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace common {

/**
 * Sample implementation of an @c AlexaSeekControllerInterface.
 */

class EndpointAlexaSeekControllerHandler : public acsdkAlexaSeekControllerInterfaces::AlexaSeekControllerInterface {
public:
    /**
     * Create a EndpointAlexaSeekControllerHandler object.
     *
     * @param endpointName The name of the endpoint.
     * @return A pointer to a new EndpointAlexaSeekControllerHandler object if it succeeds; otherwise, @c nullptr.
     */
    static std::shared_ptr<EndpointAlexaSeekControllerHandler> create(const std::string& endpointName);

    /// @name AlexaSeekControllerInterface methods
    /// @{
    acsdkAlexaSeekControllerInterfaces::AlexaSeekControllerInterface::Response adjustSeekPosition(
        const std::chrono::milliseconds& deltaPosition) override;
    /// @}

private:
    /**
     * Constructor.
     * @param endpointName The name of the endpoint the capability is associated.
     */
    EndpointAlexaSeekControllerHandler(const std::string& endpointName);

    /// Mutex to serialize access to variables.
    std::mutex m_mutex;

    /// The name of the endpoint that this controller is associated.
    std::string m_endpointName;

    /// The current timestamp position of video or audio content on the device in milliseconds.
    std::chrono::milliseconds m_currentMediaPosition;
};

}  // namespace common
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ACSDK_SAMPLE_ENDPOINT_ENDPOINTALEXASEEKCONTROLLERHANDLER_H_
