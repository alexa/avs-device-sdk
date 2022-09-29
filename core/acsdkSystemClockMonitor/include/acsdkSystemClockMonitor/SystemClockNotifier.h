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

#ifndef ACSDKSYSTEMCLOCKMONITOR_SYSTEMCLOCKNOTIFIER_H_
#define ACSDKSYSTEMCLOCKMONITOR_SYSTEMCLOCKNOTIFIER_H_

#include <memory>

#include <acsdk/Notifier/Notifier.h>
#include <acsdkSystemClockMonitorInterfaces/SystemClockNotifierInterface.h>
#include <acsdkSystemClockMonitorInterfaces/SystemClockMonitorObserverInterface.h>

namespace alexaClientSDK {
namespace acsdkSystemClockMonitor {

/**
 * Implementation of StartupNotifierInterface.
 */
class SystemClockNotifier
        : public notifier::Notifier<acsdkSystemClockMonitorInterfaces::SystemClockMonitorObserverInterface> {
public:
    /**
     * Factory method.
     * @return A new instance of @c StartupNotifierInterface.
     */
    static std::shared_ptr<acsdkSystemClockMonitorInterfaces::SystemClockNotifierInterface>
    createSystemClockNotifierInterface();
};

}  // namespace acsdkSystemClockMonitor
}  // namespace alexaClientSDK

#endif  // ACSDKSYSTEMCLOCKMONITOR_SYSTEMCLOCKNOTIFIER_H_