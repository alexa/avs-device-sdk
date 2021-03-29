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

#ifndef ACSDKSYSTEMCLOCKMONITORINTERFACES_SYSTEMCLOCKMONITORINTERFACE_H_
#define ACSDKSYSTEMCLOCKMONITORINTERFACES_SYSTEMCLOCKMONITORINTERFACE_H_

namespace alexaClientSDK {
namespace acsdkSystemClockMonitorInterfaces {

/**
 * This interface allows the application to notify when the system clock is synchronized.
 */
class SystemClockMonitorInterface {
public:
    /**
     * Destructor.
     */
    virtual ~SystemClockMonitorInterface() = default;

    /**
     * Notify observers when the system clock is synchronized.
     */
    virtual void onSystemClockSynchronized() = 0;
};

}  // namespace acsdkSystemClockMonitorInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKSYSTEMCLOCKMONITORINTERFACES_SYSTEMCLOCKMONITORINTERFACE_H_
