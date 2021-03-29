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

#ifndef ACSDKSYSTEMCLOCKMONITOR_SYSTEMCLOCKMONITOR_H_
#define ACSDKSYSTEMCLOCKMONITOR_SYSTEMCLOCKMONITOR_H_

#include <acsdkSystemClockMonitorInterfaces/SystemClockMonitorInterface.h>
#include <acsdkSystemClockMonitorInterfaces/SystemClockMonitorObserverInterface.h>
#include <acsdkSystemClockMonitorInterfaces/SystemClockNotifierInterface.h>

namespace alexaClientSDK {
namespace acsdkSystemClockMonitor {

/**
 * Implementation of SystemClockMonitorInterface.
 *
 * When @c onSystemClockSynchronized() is called, observers that have added themselves via SystemClockNotifierInterface
 * will have their @c onSystemClockSynchronized() method called.
 */
class SystemClockMonitor : public acsdkSystemClockMonitorInterfaces::SystemClockMonitorInterface {
public:
    /**
     * Create a new instance of SystemClockMonitorInterface.
     *
     * @param notifier The notifier to use to invoke SystemClockMonitorObserverInterface::onSystemClockSynchronized().
     */
    static std::shared_ptr<acsdkSystemClockMonitorInterfaces::SystemClockMonitorInterface>
    createSystemClockMonitorInterface(
        const std::shared_ptr<acsdkSystemClockMonitorInterfaces::SystemClockNotifierInterface>& notifier);

    /// @name SystemClockMonitorInterface methods.
    /// @{
    void onSystemClockSynchronized() override;
    /// @}
private:
    /**
     * Constructor.
     *
     * @param notifier The notifier to use to invoke SystemClockMonitorObserverInterface::onSystemClockSynchronized().
     */
    SystemClockMonitor(
        const std::shared_ptr<acsdkSystemClockMonitorInterfaces::SystemClockNotifierInterface>& notifier);

    /// The notifier to use to invoke SystemClockMonitorObserverInterface::onSystemClockSynchronized().
    std::shared_ptr<acsdkSystemClockMonitorInterfaces::SystemClockNotifierInterface> m_notifier;
};

}  // namespace acsdkSystemClockMonitor
}  // namespace alexaClientSDK

#endif  // ACSDKSYSTEMCLOCKMONITOR_SYSTEMCLOCKMONITOR_H_