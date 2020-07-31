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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_DIAGNOSTICS_PROTOCOLTRACERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_DIAGNOSTICS_PROTOCOLTRACERINTERFACE_H_

#include <AVSCommon/SDKInterfaces/EventTracerInterface.h>
#include <AVSCommon/SDKInterfaces/MessageObserverInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace diagnostics {

class ProtocolTracerInterface
        : public avsCommon::sdkInterfaces::MessageObserverInterface
        , public avsCommon::sdkInterfaces::EventTracerInterface {
public:
    /// Destructor.
    virtual ~ProtocolTracerInterface() = default;

    /*
     * Return the max number of messages.
     *
     * @return The max number of messages.
     */
    virtual unsigned int getMaxMessages() = 0;

    /**
     * Change the max messages that can be stored. If @c limit is lower than the number of
     * currently stored messages, then this function should fail and leave the limit unchanged.
     *
     * @param limit The maximum number of messages that will be stored.
     * @return Whether this function succeeded in changing the limit.
     */
    virtual bool setMaxMessages(unsigned int limit) = 0;

    /**
     * Set the protocol trace flag.
     * @param enabled indicating if the flag should be true/false.
     */
    virtual void setProtocolTraceFlag(bool enabled) = 0;

    /**
     * Returns the recorded protocol trace string.
     * Sample output:
     * {
     *  "trace" : [
     *      <Directive 1>,
     *      <Directive 2>,
     *      <Event 1>,
     *      <Directive 3>
     *    ]
     * }
     *
     * @return the protocol trace as a string.
     */
    virtual std::string getProtocolTrace() = 0;

    /**
     * This method acquires a mutex and clears the trace messages that are recorded.
     */
    virtual void clearTracedMessages() = 0;
};

}  // namespace diagnostics
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_DIAGNOSTICS_PROTOCOLTRACERINTERFACE_H_
