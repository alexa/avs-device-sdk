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

#ifndef ALEXA_CLIENT_SDK_DIAGNOSTICS_INCLUDE_DIAGNOSTICS_DEVICEPROTOCOLTRACER_H_
#define ALEXA_CLIENT_SDK_DIAGNOSTICS_INCLUDE_DIAGNOSTICS_DEVICEPROTOCOLTRACER_H_

#include <memory>
#include <mutex>
#include <vector>

#include <AVSCommon/SDKInterfaces/Diagnostics/ProtocolTracerInterface.h>

namespace alexaClientSDK {
namespace diagnostics {

/**
 * Utility class to record directives and events processed by the SDK.
 */
class DeviceProtocolTracer : public avsCommon::sdkInterfaces::diagnostics::ProtocolTracerInterface {
public:
    /**
     * Creates a new instance of @c DeviceProtocolTracer.
     *
     * @return a new @c DeviceProtocolTracer.
     */
    static std::shared_ptr<DeviceProtocolTracer> create();

    /// @name ProtocolTracerInterface Functions
    /// @{
    unsigned int getMaxMessages() override;
    bool setMaxMessages(unsigned int limit) override;
    void setProtocolTraceFlag(bool enabled) override;
    std::string getProtocolTrace() override;
    void clearTracedMessages() override;
    /// @}

    /// @name EventTracerInterface Functions
    /// @{
    void traceEvent(const std::string& messageContent) override;
    /// @}

    /// @name MessageObserverInterface Functions
    /// @{
    void receive(const std::string& contextId, const std::string& message) override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param maxMessages Maximum number of messages.
     */
    DeviceProtocolTracer(unsigned int maxMessages);

    /**
     * Clears the traced messages.
     */
    void clearTracedMessagesLocked();

    /**
     * Utility method that adds the AVS Directive or Event to the tracked message queue
     *
     * @param messageContent String with the contents of the message.
     */
    void traceMessageLocked(const std::string& messageContent);

    /// The mutex to synchronize the enable flag, the max number of messages, and the traced messages.
    std::mutex m_mutex;

    /// The flag to check if the protocol trace is enabled.
    bool m_isProtocolTraceEnabled;

    /// The maximum number of trace messages stored.
    unsigned int m_maxMessages;

    /// The traced messages.
    std::vector<std::string> m_tracedMessages;
};

}  // namespace diagnostics
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_DIAGNOSTICS_INCLUDE_DIAGNOSTICS_DEVICEPROTOCOLTRACER_H_
