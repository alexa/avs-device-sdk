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
#ifndef ACSDK_SAMPLE_ENDPOINT_ENDPOINTINPUTCONTROLLERHANDLER_H_
#define ACSDK_SAMPLE_ENDPOINT_ENDPOINTINPUTCONTROLLERHANDLER_H_

#include <memory>
#include <string>
#include <vector>

#include <acsdk/AlexaInputControllerInterfaces/InputControllerInterface.h>

#include <acsdk/Notifier/Notifier.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace common {

/**
 * Sample implementation of an @c InputControllerInterface.
 */
class EndpointInputControllerHandler : public alexaInputControllerInterfaces::InputControllerInterface {
public:
    /**
     * Create a InputControllerInterface object.
     *
     * @param endpointName The name of the endpoint
     *
     * @return A pointer to a new InputControllerHandler object if it succeeds; otherwise, @c nullptr.
     */
    static std::shared_ptr<EndpointInputControllerHandler> create(const std::string& endpointName);

    /// @name InputControllerInterface methods
    /// @{
    SupportedInputs getSupportedInputs() override;
    InputControllerInterface::Response setInput(alexaInputControllerInterfaces::Input input) override;
    alexaInputControllerInterfaces::Input getInput() override;
    bool addObserver(
        const std::weak_ptr<alexaInputControllerInterfaces::InputControllerObserverInterface> observer) override;
    void removeObserver(
        const std::weak_ptr<alexaInputControllerInterfaces::InputControllerObserverInterface> observer) override;
    /// @}

private:
    /**
     * Constructor.
     * @param endpointName The name of the endpoint the capability is associated.
     */
    EndpointInputControllerHandler(const std::string& endpointName);

    /// Mutex to serialize access to variables.
    std::mutex m_mutex;

    /// The name of the endpoint that this controller is associated.
    std::string m_endpointName;

    /// Current input of the endpoint.
    alexaInputControllerInterfaces::Input m_currentInput;

    /// Inputs supported by the endpoint.
    alexaInputControllerInterfaces::InputControllerInterface::SupportedInputs m_supportedInputs;

    /// The notifier of @c InputControllerObserverInterface observers.
    notifier::Notifier<alexaInputControllerInterfaces::InputControllerObserverInterface> m_notifier;
};

}  // namespace common
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ACSDK_SAMPLE_ENDPOINT_ENDPOINTINPUTCONTROLLERHANDLER_H_
