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

#include "AVSCommon/Utils/Metrics.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {

using namespace alexaClientSDK::avsCommon::avs;
using namespace alexaClientSDK::avsCommon::utils::logger;

const std::string Metrics::locationToString(Metrics::Location location) {
    switch (location) {
        case ADSL_ENQUEUE:
            return "ADSL Enqueue";
        case ADSL_DEQUEUE:
            return "ADSL Dequeue";
        case SPEECH_SYNTHESIZER_RECEIVE:
            return "SpeechSynthesizer Receive";
        case AIP_RECEIVE:
            return "AIP Receive";
        case AIP_SEND:
            return "AIP Send";
        case BUILDING_MESSAGE:
            return "Building Message";
    }

    // UNREACHABLE
    return "unknown";
}

logger::LogEntry& Metrics::d(LogEntry& logEntry, const std::shared_ptr<AVSMessage> msg, Metrics::Location location) {
    return d(logEntry, msg->getName(), msg->getMessageId(), msg->getDialogRequestId(), location);
}

logger::LogEntry& Metrics::d(
    LogEntry& logEntry,
    const std::string& name,
    const std::string& messageId,
    const std::string& dialogRequestId,
    Location location) {
    logEntry.d("Location", locationToString(location)).d("NAME", name);

    if (!messageId.empty() || !dialogRequestId.empty()) {
        logEntry.d("MessageID", messageId).d("DialogRequestID", dialogRequestId);
    };

    return logEntry;
}

}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
