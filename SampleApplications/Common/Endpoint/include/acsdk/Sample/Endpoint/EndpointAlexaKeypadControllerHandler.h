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

#ifndef ACSDK_SAMPLE_ENDPOINT_ENDPOINTALEXAKEYPADCONTROLLERHANDLER_H_
#define ACSDK_SAMPLE_ENDPOINT_ENDPOINTALEXAKEYPADCONTROLLERHANDLER_H_

#include <memory>
#include <mutex>
#include <string>

#include <acsdkAlexaKeypadControllerInterfaces/AlexaKeypadControllerInterface.h>
#include <acsdkAlexaKeypadControllerInterfaces/Keystroke.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace common {

/**
 * Sample implementation of an @c AlexaKeypadControllerInterface.
 */

class EndpointAlexaKeypadControllerHandler
        : public acsdkAlexaKeypadControllerInterfaces::AlexaKeypadControllerInterface {
public:
    /**
     * Create a AlexaKeypadControllerHandler object.
     *
     * @param endpointName The name of the endpoint.
     * @return A pointer to a new @c AlexaKeypadControllerHandler object if it succeeds; otherwise, @c nullptr.
     */
    static std::shared_ptr<EndpointAlexaKeypadControllerHandler> create(const std::string& endpointName);

    /// @name AlexaKeypadControllerInterface methods
    /// @{
    acsdkAlexaKeypadControllerInterfaces::AlexaKeypadControllerInterface::Response handleKeystroke(
        acsdkAlexaKeypadControllerInterfaces::Keystroke keystroke) override;

    std::set<acsdkAlexaKeypadControllerInterfaces::Keystroke> getSupportedKeys() override;
    /// @}

private:
    /**
     * Constructor.
     * @param endpointName The name of the endpoint the capability is associated.
     */
    EndpointAlexaKeypadControllerHandler(const std::string& endpointName);

    /// Mutex to serialize access to variables.
    std::mutex m_mutex;

    /// The name of the endpoint that this controller is associated.
    std::string m_endpointName;
};

}  // namespace common
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ACSDK_SAMPLE_ENDPOINT_ENDPOINTALEXAKEYPADCONTROLLERHANDLER_H_
