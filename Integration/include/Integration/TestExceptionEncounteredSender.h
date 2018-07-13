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

#ifndef ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_TESTEXCEPTIONENCOUNTEREDSENDER_H_
#define ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_TESTEXCEPTIONENCOUNTEREDSENDER_H_

#include <condition_variable>
#include <string>
#include <future>
#include <fstream>
#include <chrono>
#include <deque>
#include <mutex>
#include <unordered_map>

#include "AVSCommon/SDKInterfaces/DirectiveHandlerInterface.h"
#include "AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h"
#include "AVSCommon/AVS/Attachment/AttachmentManager.h"
#include "AVSCommon/Utils/JSON/JSONUtils.h"

using namespace alexaClientSDK::avsCommon;

namespace alexaClientSDK {
namespace integration {
namespace test {

/**
 * TestExceptionEncounteredSender is a mock of the @c ExceptionEncounteredSenderInterface and allows tests
 * to wait for invocations upon those interfaces and inspect the parameters of those invocations.
 */
class TestExceptionEncounteredSender : public avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface {
public:
    void sendExceptionEncountered(
        const std::string& unparsedDirective,
        avsCommon::avs::ExceptionErrorType error,
        const std::string& message) override;
    /**
     * Parse an @c AVSDirective from a JSON string.
     *
     * @param rawJSON The JSON to parse.
     * @param attachmentManager The @c AttachmentManager to initialize the new @c AVSDirective with.
     * @return A new @c AVSDirective, or nullptr if parsing the JSON fails.
     */
    std::shared_ptr<avs::AVSDirective> parseDirective(
        const std::string& rawJSON,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentManager> attachmentManager);

    /**
     * Class defining the parameters to calls to the mocked interfaces.
     */
    class ExceptionParams {
    public:
        /**
         * Constructor.
         */
        ExceptionParams();

        // Enum for the way the directive was passed.
        enum class Type {
            // Not yet set.
            UNSET,
            // Set when sendExceptionEncountered is called.
            EXCEPTION,
            // Set when waitForNext times out waiting for a directive.
            TIMEOUT
        };

        // Type of how the directive was passed.
        Type type;
        // AVSDirective passed from the Directive Sequencer.
        std::shared_ptr<avsCommon::avs::AVSDirective> directive;
        // Unparsed directive string passed to sendExceptionEncountered.
        std::string exceptionUnparsedDirective;
        // Error type passed to sendExceptionEncountered.
        avsCommon::avs::ExceptionErrorType exceptionError;
        // Additional information passed to sendExceptionEncountered.
        std::string exceptionMessage;
    };

    /**
     * Function to retrieve the next DirectiveParams in the test queue or time out if the queue is empty. Takes a
     * duration in seconds to wait before timing out.
     */
    ExceptionParams waitForNext(const std::chrono::seconds duration);

private:
    /// Mutex to protect m_queue.
    std::mutex m_mutex;
    /// Trigger to wake up waitForNext calls.
    std::condition_variable m_wakeTrigger;
    /// Queue of received directives that have not been waited on.
    std::deque<ExceptionParams> m_queue;
};

}  // namespace test
}  // namespace integration
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_TESTEXCEPTIONENCOUNTEREDSENDER_H_
