/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_MESSAGEOBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_MESSAGEOBSERVERINTERFACE_H_

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

#include <string>

/**
 * This class allows a client to receive messages from AVS.
 */
class MessageObserverInterface {
public:
    /**
     * Destructor.
     */
    virtual ~MessageObserverInterface() = default;

    /**
     * A function that a client must implement to receive Messages from AVS.
     *
     * @param contextId The context for the message, which in this case reflects the logical HTTP/2 stream the
     * message arrived on.
     * @param message The AVS message that has been received.
     */
    virtual void receive(const std::string& contextId, const std::string& message) = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_MESSAGEOBSERVERINTERFACE_H_
