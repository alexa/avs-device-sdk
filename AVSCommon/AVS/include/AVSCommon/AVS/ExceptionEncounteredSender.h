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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_EXCEPTIONENCOUNTEREDSENDER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_EXCEPTIONENCOUNTEREDSENDER_H_

#include <memory>
#include <string>
#include <AVSCommon/AVS/ExceptionErrorType.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/Utils/Threading/Executor.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

/**
 * Class creates an ExceptionEncountered event and sends to AVS using @c MessageSenderInterface.
 */
class ExceptionEncounteredSender : public avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface {
public:
    /**
     * Creates a new @c ExceptionEncounteredSender instance.
     *
     * @param messageSender The object to use for sending events.
     * @return A @c std::unique_ptr to the new @c ExceptionEncounteredSender instance.
     */
    static std::unique_ptr<ExceptionEncounteredSender> create(
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender);

    /**
     * This function asks the @c ExceptionEncounteredSender to send a ExceptionEncountered event to AVS when
     * the client is unable to execute a directive.
     *
     * @param unparsedDirective The directive which the client was unable to execute, must be sent to AVS
     * @param error The @c ExceptionErrorType to be sent to AVS
     * @param errorDescription The error details to be sent for logging and troubleshooting.
     */
    void sendExceptionEncountered(
        const std::string& unparsedDirective,
        avs::ExceptionErrorType error,
        const std::string& errorDescription) override;

private:
    /**
     * Constructor.
     *
     * @param messageSender The object to use for sending events.
     */
    ExceptionEncounteredSender(std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender);

    /// The object to use for sending events.
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> m_messageSender;
};

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_EXCEPTIONENCOUNTEREDSENDER_H_
