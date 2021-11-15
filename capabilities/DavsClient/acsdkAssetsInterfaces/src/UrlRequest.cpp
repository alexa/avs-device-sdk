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

#include "acsdkAssetsInterfaces/UrlRequest.h"

#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include <algorithm>

namespace alexaClientSDK {
namespace acsdkAssets {
namespace commonInterfaces {

using namespace std;
using namespace avsCommon::utils::json;

static const char* ARTIFACT_URL = "url";
static const char* ARTIFACT_FILENAME = "filename";
static const char* ARTIFACT_UNPACK = "unpack";
static const char* ARTIFACT_CERT_PATH = "certPath";

/// String to identify log entries originating from this file.
static const std::string TAG{"UrlRequest"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

static std::string getHash(const std::string& str) {
    return std::to_string(std::hash<std::string>{}(str));
};

shared_ptr<UrlRequest> UrlRequest::create(string url, string filename, bool unpack, string certPath) {
    if (url.empty()) {
        ACSDK_ERROR(LX("create").m("Empty url"));
        return nullptr;
    }
    if (filename.empty()) {
        ACSDK_ERROR(LX("create").m("Empty filename"));
        return nullptr;
    }
    if (filename.find("..") != std::string::npos) {
        ACSDK_ERROR(LX("create").m("Filename containing '..' not allowed").d("file name", filename.c_str()));
        return nullptr;
    }
    if (!certPath.empty()) {
        ACSDK_INFO(LX("create").m("Using custom cert from path").d("path", certPath));
    }

    return unique_ptr<UrlRequest>(new UrlRequest(move(url), move(filename), unpack, move(certPath)));
}

UrlRequest::UrlRequest(string url, string filename, bool unpack, string certPath) :
        m_url(move(url)),
        m_filename(move(filename)),
        m_unpack(unpack),
        m_certPath(certPath),
        m_certHash(certPath.empty() ? "" : getHash(certPath)) {
    m_summary = "url_" + getHash(this->m_url) + "_" + this->m_filename;

    m_summary += this->m_certHash;

    if (m_unpack) {
        m_summary += "_unpacked";
    }

    // remove special characters that would be incompatible as property names or file names
    m_summary.erase(
            remove_if(m_summary.begin(), m_summary.end(), [](char c) { return c != '_' && !isalnum(c); }),
            m_summary.end());
}

string UrlRequest::toJsonString() const {
    JsonGenerator generator;
    generator.addMember(ARTIFACT_URL, m_url);
    generator.addMember(ARTIFACT_FILENAME, m_filename);
    generator.addMember(ARTIFACT_CERT_PATH, m_certPath);
    generator.addMember(ARTIFACT_UNPACK, m_unpack);
    return generator.toString();
}

}  // namespace commonInterfaces
}  // namespace acsdkAssets
}  // namespace alexaClientSDK
