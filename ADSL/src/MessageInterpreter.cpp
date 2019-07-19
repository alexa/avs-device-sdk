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

#include "ADSL/MessageInterpreter.h"

#include <AVSCommon/Utils/Metrics.h>

#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace adsl {

using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;

/// String to identify log entries originating from this file.
static const std::string TAG("MessageInterpreter");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

MessageInterpreter::MessageInterpreter(
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
    std::shared_ptr<DirectiveSequencerInterface> directiveSequencer,
    std::shared_ptr<AttachmentManagerInterface> attachmentManager) :
        m_exceptionEncounteredSender{exceptionEncounteredSender},
        m_directiveSequencer{directiveSequencer},
        m_attachmentManager{attachmentManager} {
}

void MessageInterpreter::receive(const std::string& contextId, const std::string& message) {
    auto createResult = AVSDirective::create(message, m_attachmentManager, contextId);
    std::shared_ptr<AVSDirective> avsDirective{std::move(createResult.first)};
    if (!avsDirective) {
        if (m_exceptionEncounteredSender) {
            const std::string errorDescription =
                "Unable to parse Directive - JSON error:" + avsDirectiveParseStatusToString(createResult.second);
            ACSDK_ERROR(LX("receiveFailed").m(errorDescription));
            m_exceptionEncounteredSender->sendExceptionEncountered(
                message, ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED, errorDescription);
        } else {
            ACSDK_ERROR(LX("receiveFailed").m("unable to send AVS Exception due to nullptr sender."));
        }

        return;
    }

    if (avsDirective->getName() == "StopCapture" || avsDirective->getName() == "Speak") {
        ACSDK_METRIC_MSG(TAG, avsDirective, Metrics::Location::ADSL_ENQUEUE);
    }

    m_directiveSequencer->onDirective(avsDirective);
}

}  // namespace adsl
}  // namespace alexaClientSDK
