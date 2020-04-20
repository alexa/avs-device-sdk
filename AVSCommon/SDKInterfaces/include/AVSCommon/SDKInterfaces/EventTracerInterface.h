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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_EVENTTRACERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_EVENTTRACERINTERFACE_H_

#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * This interface allows classes to observe messages sent to @c AVS via a callback.
 */
class EventTracerInterface {
public:
    /**
     * Destructor.
     */
    virtual ~EventTracerInterface() = default;

    /**
     * This method is called when an event is sent to @c AVS.
     *
     * @param messageContent The content of the message sent to @c AVS.
     */
    virtual void traceEvent(const std::string& messageContent) = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_EVENTTRACERINTERFACE_H_
