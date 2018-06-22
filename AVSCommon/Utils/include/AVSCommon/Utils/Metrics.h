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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_H_

#include <AVSCommon/Utils/Logger/LogEntry.h>
#include <AVSCommon/AVS/AVSMessage.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {

const std::string METRICS_TAG = ":METRICS:";

class Metrics {
public:
    /**
     * Enum indicating the location where the metric message was issued
     */
    enum Location {
        // Enqueue message in ADSL module
        ADSL_ENQUEUE,

        // Dequeue message in ADSL module
        ADSL_DEQUEUE,

        // SpeechSynthesizer receive the message
        SPEECH_SYNTHESIZER_RECEIVE,

        // AudioInputProcessor receive the message
        AIP_RECEIVE,

        // AudioInputProcessor send the message
        AIP_SEND,

        // Used when issuing an extra metric log for missing Ids
        BUILDING_MESSAGE
    };

    /**
     * Add @c Metric related info to a @c LogEntry
     * @param logEntry The @c LogEntry object to add the metric info.
     * @param name @c Event/@c Directive name.
     * @param messageId The message ID.
     * @param dialogRequestId The dialog request ID of the message
     * @param location The location in which the log was issued.
     * @return The given @c LogEntry with the metric info added.
     */
    static logger::LogEntry& d(
        alexaClientSDK::avsCommon::utils::logger::LogEntry& logEntry,
        const std::string& name,
        const std::string& messageId,
        const std::string& dialogRequestId,
        Location location);

    /**
     * Add metric related info to a @c LogEntry
     * @param logEntry The @c LogEntry object to add the metric info.
     * @param msg The @c AVSMessage related to this metric
     * @param location The location in which the log was issued.
     * @return The given @c LogEntry with the metric info added.
     */
    static logger::LogEntry& d(
        alexaClientSDK::avsCommon::utils::logger::LogEntry& logEntry,
        const std::shared_ptr<alexaClientSDK::avsCommon::avs::AVSMessage> msg,
        Location location);

private:
    /**
     * Translate @c Location into a string representation.
     * @param location A @c Location to translate.
     * @return a String representation of the given location.
     */
    static const std::string locationToString(Location location);
};

}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#ifdef ACSDK_LATENCY_LOG_ENABLED

/**
 * Send a METRIC log line.
 *
 * @param entry The text (or builder of the text) for the log entry.
 */
#define ACSDK_METRIC_WITH_ENTRY(entry) ACSDK_CRITICAL(entry)

/**
 * Send a Metric log line.
 *
 * @param TAG The name of the source of the log entry
 * @param name The Event \ Directive name.
 * @param messageId The message ID of the Event \ directive
 * @param dialogRequestId The dialog request ID of the Event \ Directive
 * @param location The location where this message was issued.
 */
#define ACSDK_METRIC_IDS(TAG, name, messageId, dialogRequestId, location)                                   \
    do {                                                                                                    \
        alexaClientSDK::avsCommon::utils::logger::LogEntry logEntry(                                        \
            TAG, __func__ + alexaClientSDK::avsCommon::utils::METRICS_TAG);                                 \
        alexaClientSDK::avsCommon::utils::Metrics::d(logEntry, name, messageId, dialogRequestId, location); \
        ACSDK_METRIC_WITH_ENTRY(logEntry);                                                                  \
    } while (false)

/**
 * Send a Metric log line.
 *
 * @param TAG The name of the source of the log entry
 * @param msg The text (or builder of the text) for the log entry.
 * @param location The location where this message was issued.
 */
#define ACSDK_METRIC_MSG(TAG, msg, location)                                   \
    do {                                                                       \
        alexaClientSDK::avsCommon::utils::logger::LogEntry logEntry(           \
            TAG, __func__ + alexaClientSDK::avsCommon::utils::METRICS_TAG);    \
        alexaClientSDK::avsCommon::utils::Metrics::d(logEntry, msg, location); \
        ACSDK_METRIC_WITH_ENTRY(logEntry);                                     \
    } while (false)

#else  // ACSDK_LATENCY_LOG_ENABLED

/**
 * Compile out a METRIC log line.
 *
 * @param TAG The name of the source of the log entry
 * @param name The Event \ Directive name.
 * @param messageId The message ID of the Event \ directive
 * @param dialogRequestId The dialog request ID of the Event \ Directive
 * @param location The location where this message was issued.
 */
#define ACSDK_METRIC_IDS(TAG, name, messageId, dialogRequestId, location)

/**
 * Compile out a METRIC log line.
 *
 * @param TAG The name of the source of the log entry
 * @param msg The text (or builder of the text) for the log entry.
 * @param location The location where this message was issued.
 */
#define ACSDK_METRIC_MSG(TAG, msg, location)

#endif  // ACSDK_LATENCY_LOG_ENABLED

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_H_
