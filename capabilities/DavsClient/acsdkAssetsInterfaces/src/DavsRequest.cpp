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

#include "acsdkAssetsInterfaces/DavsRequest.h"

#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include <algorithm>

namespace alexaClientSDK {
namespace acsdkAssets {
namespace commonInterfaces {

using namespace std;
using namespace avsCommon::utils::json;

static const char* ARTIFACT_TYPE = "artifactType";
static const char* ARTIFACT_KEY = "artifactKey";
static const char* ARTIFACT_FILTERS = "filters";
static const char* ARTIFACT_UNPACK = "unpack";
static const char* ARTIFACT_ENDPOINT = "endpoint";

/// String to identify log entries originating from this file.
static const std::string TAG{"DavsRequest"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

shared_ptr<DavsRequest> DavsRequest::create(string type, string key, FilterMap filters, Region endpoint, bool unpack) {
    if (type.empty()) {
        ACSDK_ERROR(LX("create").m("Empty type"));
        return nullptr;
    }

    if (key.empty()) {
        ACSDK_ERROR(LX("create").m("Empty key"));
        return nullptr;
    }

    for (const auto& filter : filters) {
        if (filter.first.empty()) {
            ACSDK_ERROR(LX("create").m("Empty filter key"));
            return nullptr;
        }
        if (filter.second.empty()) {
            ACSDK_ERROR(LX("create").m("Empty filter value for key").d("key", filter.first));
            return nullptr;
        }
    }

    return unique_ptr<DavsRequest>(new DavsRequest(move(type), move(key), move(filters), endpoint, unpack));
}

DavsRequest::DavsRequest(string type, string key, FilterMap filters, Region endpoint, bool unpack) :
        m_type(move(type)),
        m_key(move(key)),
        m_filters(move(filters)),
        m_region(endpoint),
        m_unpack(unpack) {
    m_summary = this->m_type + "_" + this->m_key;

    for (const auto& filter : this->m_filters) {
        for (const auto& item : filter.second) {
            m_summary += "_" + item;
        }
    }

    // NA endpoint can be left without a suffix, to mimic the url endpoint pattern
    if (endpoint == Region::EU) {
        m_summary += "_EU";
    } else if (endpoint == Region::FE) {
        m_summary += "_FE";
    }

    if (unpack) {
        m_summary += "_unpacked";
    }

    // remove special characters that would be incompatible as property names or file names
    m_summary.erase(
            remove_if(m_summary.begin(), m_summary.end(), [](char c) { return c != '_' && !isalnum(c); }),
            m_summary.end());
}

std::string DavsRequest::toJsonString() const {
    JsonGenerator generator;
    generator.addMember(ARTIFACT_TYPE, m_type);
    generator.addMember(ARTIFACT_KEY, m_key);
    generator.startObject(ARTIFACT_FILTERS);
    for (const auto& filter : m_filters) {
        generator.addStringArray(filter.first, filter.second);
    }
    generator.finishObject();
    generator.addMember(ARTIFACT_ENDPOINT, static_cast<int>(m_region));
    generator.addMember(ARTIFACT_UNPACK, m_unpack);
    return generator.toString();
}

}  // namespace commonInterfaces
}  // namespace acsdkAssets
}  // namespace alexaClientSDK