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

#include "RequesterMetadata.h"

#include <AVSCommon/Utils/FileSystem/FileSystemUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <fstream>

#include "RequestFactory.h"
#include "acsdkAssetsCommon/AmdMetricWrapper.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace manager {

using namespace std;
using namespace chrono;
using namespace rapidjson;
using namespace common;
using namespace commonInterfaces;
using namespace alexaClientSDK::avsCommon::utils;

static const char* RESOURCE_ID = "resourceId";
static const char* USED_TIMESTAMP = "usedTimestamp";

static const string TMP_FILE_POSTFIX = ".tmp";

static const auto s_metrics = AmdMetricsWrapper::creator("requesterMetadata");

/// String to identify log entries originating from this file.
static const std::string TAG{"RequesterMetadata"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

unique_ptr<RequesterMetadata> RequesterMetadata::create(
        shared_ptr<ArtifactRequest> request,
        string resourceId,
        milliseconds lastUsed) {
    if (request == nullptr) {
        ACSDK_ERROR(LX("create").m("metadata null"));
        return nullptr;
    }

    return unique_ptr<RequesterMetadata>(new RequesterMetadata(move(request), move(resourceId), lastUsed));
}

RequesterMetadata::RequesterMetadata(shared_ptr<ArtifactRequest> request, string resourceId, milliseconds lastUsed) :
        m_request(move(request)),
        m_resourceId(move(resourceId)),
        m_lastUsed(lastUsed) {
}

unique_ptr<RequesterMetadata> RequesterMetadata::createFromFile(const string& metadataFile) {
    Document document;

    if (equal(TMP_FILE_POSTFIX.rbegin(), TMP_FILE_POSTFIX.rend(), metadataFile.rbegin())) {
        ACSDK_ERROR(LX("createFromFile").m("Cannot use a temp file"));
        s_metrics().addCounter(METRIC_PREFIX_ERROR("tmpMetadataFound")).addString("file", metadataFile);
        return nullptr;
    }

    ifstream ifs(metadataFile);
    IStreamWrapper is(ifs);

    if (document.ParseStream(is).HasParseError()) {
        ACSDK_ERROR(LX("createFromFile").m("Error parsing the metadata file"));
        return nullptr;
    }

    string resourceId;
    auto usedTimestamp = milliseconds::zero();

    auto request = RequestFactory::create(document);
    if (request == nullptr) {
        ACSDK_ERROR(LX("createFromFile").m("Could not create request from json document"));
        return nullptr;
    }
    if (document.HasMember(RESOURCE_ID)) {
        resourceId = document[RESOURCE_ID].GetString();
    } else {
        ACSDK_ERROR(LX("createFromFile").d("Missing member", RESOURCE_ID));
        return nullptr;
    }

    if (document.HasMember(USED_TIMESTAMP)) {
        auto& usedTimestampMember = document[USED_TIMESTAMP];
        if (usedTimestampMember.IsInt64()) {
            usedTimestamp = milliseconds(document[USED_TIMESTAMP].GetInt64());
        }
    } else {
        ACSDK_WARN(LX("createFromFile").d("Missing member", USED_TIMESTAMP).d("using default", usedTimestamp.count()));
    }

    return unique_ptr<RequesterMetadata>(new RequesterMetadata(move(request), move(resourceId), usedTimestamp));
}

bool RequesterMetadata::saveToFile(const string& metadataFile) {
    Document document;
    document.Parse(m_request->toJsonString());
    auto& allocator = document.GetAllocator();

    document.AddMember(StringRef(RESOURCE_ID), StringRef(m_resourceId), allocator);
    document.AddMember(StringRef(USED_TIMESTAMP), static_cast<uint64_t>(m_lastUsed.count()), allocator);

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    document.Accept(writer);

    auto tmpFile = metadataFile + TMP_FILE_POSTFIX;
    ofstream fileHandle(tmpFile, ios::out);
    if (!fileHandle.is_open()) {
        ACSDK_ERROR(LX("saveToFile").m("Failed to open metadata file"));
        return false;
    }
    fileHandle << buffer.GetString();
    fileHandle.close();
    if (!fileHandle.good() || !filesystem::move(tmpFile, metadataFile)) {
        ACSDK_ERROR(LX("saveToFile").m("Failed to close the file").d("file", metadataFile.c_str()));
        s_metrics().addCounter(METRIC_PREFIX_ERROR("metadataSave")).addString("file", metadataFile);
        return false;
    }
    return true;
}

}  // namespace manager
}  // namespace acsdkAssets
}  // namespace alexaClientSDK