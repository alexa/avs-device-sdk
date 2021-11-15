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

#include "acsdkAssetsInterfaces/VendableArtifact.h"

#include <AVSCommon/Utils/Logger/Logger.h>
#include <rapidjson/document.h>

#include <chrono>

namespace alexaClientSDK {
namespace acsdkAssets {
namespace commonInterfaces {

using namespace std;
using namespace rapidjson;

/// String to identify log entries originating from this file.
static const std::string TAG{"VendableArtifact"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

unique_ptr<VendableArtifact> VendableArtifact::create(
        shared_ptr<DavsRequest> request,
        string id,
        size_t artifactSizeBytes,
        TimeEpoch artifactExpiry,
        string s3Url,
        TimeEpoch urlExpiry,
        size_t currentSizeBytes,
        bool multipart) {
    if (request == nullptr) {
        ACSDK_ERROR(LX("create").m("Null request"));
        return nullptr;
    }

    if (id.empty()) {
        ACSDK_ERROR(LX("create").m("Empty id"));
        return nullptr;
    }

    if (artifactSizeBytes == 0) {
        ACSDK_ERROR(LX("create").m("Artifact Size is zero"));
        return nullptr;
    }

    if (!multipart && s3Url.empty()) {
        ACSDK_ERROR(LX("create").m("Empty S3 URL"));
        return nullptr;
    }

    auto uuid = request->getType() + "_" + request->getKey() + "_" + id;
    if (request->needsUnpacking()) {
        uuid += "_unpack";
    }
    return unique_ptr<VendableArtifact>(new VendableArtifact(
            move(request),
            move(id),
            artifactSizeBytes,
            artifactExpiry,
            move(s3Url),
            urlExpiry,
            currentSizeBytes,
            move(uuid),
            multipart));
}

/**
 * Read JSON member into given string.
 * @param destination the string to write to
 * @param source the JSON object to read from
 * @param key the JSON key to read
 * @return true if the key exists and is a valid string
 */
static bool readStringMember(string& destination, const Document& source, const char* key) {
    if (!source.HasMember(key) || !source[key].IsString()) {
        return false;
    }

    string value = source[key].GetString();
    if (value.empty() || value == "null") {
        return false;
    }

    destination = value;
    return true;
}

/**
 * Read JSON member into given uint64_t.
 * @param destination the uint64_t to write to
 * @param source the JSON object to read from
 * @param key the JSON key to read
 * @return true if the key exists and is a valid number
 */
static bool readSizeMember(uint64_t& destination, const Document& source, const char* key) {
    if (!source.HasMember(key) || !source[key].IsUint64()) {
        return false;
    }

    destination = source[key].GetUint64();
    return true;
}

unique_ptr<VendableArtifact> VendableArtifact::create(
        std::shared_ptr<DavsRequest> request,
        const string& jsonString,
        const bool isMultipart) {
    Document document;
    document.Parse(jsonString);
    if (document.HasParseError() || !document.IsObject()) {
        ACSDK_ERROR(LX("create").m("Can't parse JSON").d("json", jsonString.c_str()));
        return nullptr;
    }

    string s3Url;
    string id;
    uint64_t urlExpiry;
    uint64_t ttl;
    uint64_t size;
    if (!isMultipart) {
        if (!readStringMember(s3Url, document, "downloadUrl")) {
            ACSDK_ERROR(LX("create").m("Failed to parse download URL"));
            return nullptr;
        }
        if (!readSizeMember(urlExpiry, document, "urlExpiryEpoch")) {
            ACSDK_ERROR(LX("create").m("Failed to parse URL Expiry Epoch"));
            return nullptr;
        }
    }
    if (!readStringMember(id, document, "artifactIdentifier")) {
        ACSDK_ERROR(LX("create").m("Failed to parse Artifact Identifier"));
        return nullptr;
    }
    if (!readSizeMember(ttl, document, "artifactTimeToLive") &&
        !readSizeMember(ttl, document, "suggestedPollInterval")) {
        ACSDK_ERROR(LX("create").m("Failed to parse TTL or Polling Interval"));
        return nullptr;
    }
    if (!readSizeMember(size, document, "artifactSize")) {
        ACSDK_ERROR(LX("create").m("Failed to parse Artifact Size"));
        return nullptr;
    }

    return create(
            move(request),
            id,
            size,
            TimeEpoch(chrono::milliseconds(ttl)),
            s3Url,
            TimeEpoch(chrono::milliseconds(urlExpiry)),
            0,
            isMultipart);
}

VendableArtifact::VendableArtifact(
        shared_ptr<DavsRequest> request,
        string id,
        size_t artifactSizeBytes,
        TimeEpoch artifactExpiry,
        string s3Url,
        TimeEpoch urlExpiry,
        size_t currentSizeBytes,
        string uuid,
        bool multipart) :
        m_request(move(request)),
        m_id(move(id)),
        m_artifactSizeBytes(artifactSizeBytes),
        m_artifactExpiry(artifactExpiry),
        m_s3Url(move(s3Url)),
        m_urlExpiry(urlExpiry),
        m_currentSizeBytes(currentSizeBytes),
        m_uuid(move(uuid)),
        m_multipart(multipart) {
}

}  // namespace commonInterfaces
}  // namespace acsdkAssets
}  // namespace alexaClientSDK
