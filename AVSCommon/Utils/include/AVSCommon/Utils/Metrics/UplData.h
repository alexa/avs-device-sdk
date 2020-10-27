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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_UPLDATA_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_UPLDATA_H_

#include <chrono>
#include <string>
#include <unordered_map>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace metrics {

/**
 * This class represents the UplData shared between UPL Calculators
 */
class UplData {
public:
    /// Alias for convenience
    using UplTimePoint = std::chrono::steady_clock::time_point;

    /**
     * Add a metric TimePoint to the UplTimePointsMap.
     * If an entry with the same name is already present, it will overwrite the previous UplTimePoint.
     *
     * @param name Metric name
     * @param timepoint Time point of the metric
     */
    void addTimepoint(const std::string& name, const UplTimePoint& timepoint);

    /**
     * Returns a saved metric's TimePoint from the UplTimePointsMap.
     *
     * @param name Name of metric
     * @returns Time point of the metric
     */
    UplTimePoint getTimepoint(const std::string& name) const;

    /**
     * Adds the time point of a specific directive's PARSE_COMPLETE metric to the MessageIdToParseCompleteMap.
     * If an entry with the same directiveId is already present, it will overwrite the previous UplTimePoint.
     *
     * @param directiveId Id of directive parsed
     * @param timepoint Time point of the PARSE_COMPLETE metric
     */
    void addParseCompleteTimepoint(const std::string& directiveId, const UplTimePoint& timepoint);

    /**
     * Returns the time point of a specific directive's PARSE_COMPLETE metric from the MessageIdToParseCompleteMap.
     *
     * @param directiveId Id of directive parsed
     * @returns Time point of the PARSE_COMPLETE metric
     */
    UplTimePoint getParseCompleteTimepoint(const std::string& directiveId) const;

    /**
     * Adds the time point of a specific directive's DIRECTIVE_DISPATCHED metric to the
     * MessageIdToDirectiveDispatchedMap.
     * If an entry with the same directiveId is already present, it will overwrite the previous UplTimePoint.
     *
     * @param directiveId Id of directive dispatched
     * @param timepoint Time point of the DIRECTIVE_DISPATCHED metric
     */
    void addDirectiveDispatchedTimepoint(const std::string& directiveId, const UplTimePoint& timepoint);

    /**
     * Returns the time point of a specific directive's DIRECTIVE_DISPATCHED metric from the
     * MessageIdToDirectiveDispatchedMap.
     *
     * @param directiveId Id of directive dispatched
     * @returns Time point of the DIRECTIVE_DISPATCHED metric
     */
    UplTimePoint getDirectiveDispatchedTimepoint(const std::string& directiveId) const;

    /**
     * Adds a string data to OtherData.
     * If an entry with the same name is already present, it will overwrite the previous string data.
     *
     * @param name Name of the data
     * @param data Data string
     */
    void addStringData(const std::string& name, const std::string& data);

    /**
     * Returns a saved string data from OtherData.
     *
     * @param name Name of the data
     * @returns Data string
     */
    std::string getStringData(const std::string& name) const;

private:
    /// Map of metric names to their recorded time point
    std::unordered_map<std::string, UplTimePoint> m_uplTimePointsMap;

    /// Map of all PARSE_COMPLETE metrics' time point recorded by directive message ID
    std::unordered_map<std::string, UplTimePoint> m_messageIdToParseCompleteMap;

    /// Map of all DIRECTIVE_DISPATCHED metrics' time point recorded by directive message ID
    std::unordered_map<std::string, UplTimePoint> m_messageIdToDirectiveDispatchedMap;

    /// Map of all other relevant string data
    std::unordered_map<std::string, std::string> m_otherData;
};

}  // namespace metrics
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_METRICS_UPLDATA_H_