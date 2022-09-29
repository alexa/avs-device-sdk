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

#include <AVSCommon/AVS/EventBuilder.h>
#include <AVSCommon/Utils/JSON/JSONGenerator.h>

#include "System/StateReportGenerator.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {

using namespace avsCommon;
using namespace avsCommon::utils;
using namespace avsCommon::utils::json;

/// String to identify log entries originating from this file.
#define TAG "StateReportGenerator"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) logger::LogEntry(TAG, event)

StateReportGenerator::StateReportGenerator(const std::vector<std::function<std::string()>>& reportFunctions) :
        m_reportFunctions{reportFunctions} {
}

std::vector<std::string> StateReportGenerator::generateReport() {
    std::vector<std::string> states;
    for (auto& reportFn : m_reportFunctions) {
        auto state = reportFn();
        if (!state.empty()) {
            states.push_back(state);
        }
    }
    return states;
}

std::string StateReportGenerator::generateSettingStateReport(
    const settings::SettingEventMetadata& metadata,
    const std::string& value) {
    if (value.empty()) {
        ACSDK_DEBUG5(LX(__func__).d("emptySetting", metadata.settingName));
        return std::string();
    }

    JsonGenerator jsonGenerator;
    jsonGenerator.startObject(avs::constants::HEADER_KEY_STRING);
    {
        jsonGenerator.addMember(avs::constants::NAMESPACE_KEY_STRING, metadata.eventNamespace);
        jsonGenerator.addMember(avs::constants::NAME_KEY_STRING, metadata.eventReportName);
    }
    jsonGenerator.finishObject();

    jsonGenerator.startObject(avs::constants::PAYLOAD_KEY_STRING);
    { jsonGenerator.addRawJsonMember(metadata.settingName, value); }
    jsonGenerator.finishObject();

    auto report = jsonGenerator.toString();
    ACSDK_DEBUG5(LX(__func__).sensitive("settingReport", report));
    return report;
}

}  // namespace system
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
