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

#ifndef ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_DEFAULTENDPOINT_DEFAULTENDPOINTRANGECONTROLLERHANDLER_H_
#define ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_DEFAULTENDPOINT_DEFAULTENDPOINTRANGECONTROLLERHANDLER_H_

#include <list>

#include <AVSCommon/SDKInterfaces/RangeController/RangeControllerInterface.h>
#include <AVSCommon/Utils/Threading/Executor.h>

namespace alexaClientSDK {
namespace sampleApp {

/**
 * An implementation of an @c RangeControllerInterface.
 */
class DefaultEndpointRangeControllerHandler
        : public avsCommon::sdkInterfaces::rangeController::RangeControllerInterface {
public:
    /**
     * Create a DefaultEndpointRangeControllerHandler object.
     *
     * @param instance The instance name of the capability.
     * @return A pointer to a new DefaultEndpointRangeControllerHandler object if it succeeds; otherwise, @c nullptr.
     */
    static std::shared_ptr<DefaultEndpointRangeControllerHandler> create(const std::string& instance);

    /// @name RangeControllerInterface methods
    /// @{
    avsCommon::sdkInterfaces::rangeController::RangeControllerInterface::RangeControllerConfiguration getConfiguration()
        override;
    std::pair<avsCommon::avs::AlexaResponseType, std::string> setRangeValue(
        double value,
        avsCommon::sdkInterfaces::AlexaStateChangeCauseType cause) override;
    std::pair<avsCommon::avs::AlexaResponseType, std::string> adjustRangeValue(
        double value,
        avsCommon::sdkInterfaces::AlexaStateChangeCauseType cause) override;
    std::pair<
        avsCommon::avs::AlexaResponseType,
        avsCommon::utils::Optional<avsCommon::sdkInterfaces::rangeController::RangeControllerInterface::RangeState>>
    getRangeState() override;
    bool addObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::rangeController::RangeControllerObserverInterface> observer) override;
    void removeObserver(
        const std::shared_ptr<avsCommon::sdkInterfaces::rangeController::RangeControllerObserverInterface>& observer)
        override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param instance The instance name of the capability.
     */
    DefaultEndpointRangeControllerHandler(const std::string& instance);

    /// The instance name of the capability.
    std::string m_instance;

    /// Current range value of the capability.
    double m_currentRangeValue;

    /// Represents the maximum range supported.
    double m_maximumRangeValue;

    /// Represents the minimum range supported.
    double m_minmumRangeValue;

    /// Represents the value to change when moving through the range.
    double m_precision;

    /// Mutex to serialize access to variables.
    std::mutex m_mutex;

    /// The list of @c RangeControllerObserverInterface observers that will get notified.
    std::list<std::shared_ptr<avsCommon::sdkInterfaces::rangeController::RangeControllerObserverInterface>> m_observers;
};

}  // namespace sampleApp
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_DEFAULTENDPOINT_DEFAULTENDPOINTRANGECONTROLLERHANDLER_H_
