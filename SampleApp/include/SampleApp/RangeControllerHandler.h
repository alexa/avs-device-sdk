/*
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_RANGECONTROLLERHANDLER_H_
#define ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_RANGECONTROLLERHANDLER_H_

#include <list>

#include <AVSCommon/SDKInterfaces/RangeController/RangeControllerInterface.h>
#include <AVSCommon/Utils/Threading/Executor.h>

namespace alexaClientSDK {
namespace sampleApp {

/**
 * An implementation of an @c RangeControllerInterface.
 */
class RangeControllerHandler : public avsCommon::sdkInterfaces::rangeController::RangeControllerInterface {
public:
    /**
     * Create a RangeControllerHandler object.
     *
     * @return A pointer to a new RangeControllerHandler object if it succeeds; otherwise, @c nullptr.
     */
    static std::shared_ptr<RangeControllerHandler> create();

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

    /**
     * Notify range value change to the observers of @c RangeControllerObserverInterface.
     *
     * @param rangeValue The changed range value to be notified to the observer.
     * @param cause The change cause represented using @c AlexaStateChangeCauseType.
     * @param observers The list of observer to be notified.
     */
    void notifyObservers(
        const avsCommon::sdkInterfaces::rangeController::RangeControllerInterface::RangeState& rangeState,
        avsCommon::sdkInterfaces::AlexaStateChangeCauseType cause,
        std::list<std::shared_ptr<avsCommon::sdkInterfaces::rangeController::RangeControllerObserverInterface>>
            observers);
    /**
     * Set the range of the controller.
     *
     * @param value Range value to be set.
     */
    void setRangeValue(double value);

private:
    /**
     * Constructor.
     */
    RangeControllerHandler();

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

#endif  // ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_RANGECONTROLLERHANDLER_H_
