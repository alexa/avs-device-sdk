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

#ifndef ACSDKSYSTEMCLOCKMONITORINTERFACES_SYSTEMCLOCKNOTIFIERINTERFACE_H_
#define ACSDKSYSTEMCLOCKMONITORINTERFACES_SYSTEMCLOCKNOTIFIERINTERFACE_H_

#include <acsdk/NotifierInterfaces/NotifierInterface.h>

#include "acsdkSystemClockMonitorInterfaces/SystemClockMonitorObserverInterface.h"

namespace alexaClientSDK {
namespace acsdkSystemClockMonitorInterfaces {

/**
 * Interface for registering to observe when the system clock is synchronized.
 */
using SystemClockNotifierInterface = notifierInterfaces::NotifierInterface<SystemClockMonitorObserverInterface>;

}  // namespace acsdkSystemClockMonitorInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKSYSTEMCLOCKMONITORINTERFACES_SYSTEMCLOCKNOTIFIERINTERFACE_H_
