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

#include "acsdkAssetsCommon/ResponseSink.h"

#include <AVSCommon/Utils/FileSystem/FileSystemUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <MultipartParser/MultipartReader.h>

#include "acsdkAssetsCommon/CurlWrapper.h"
#include "acsdkAssetsCommon/DownloadStream.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace common {

using namespace alexaClientSDK::avsCommon::utils;
using namespace commonInterfaces;

/// MIME type for JSON payloads
static const std::string MIME_JSON_CONTENT_TYPE = "application/json";

/// MIME type for binary streams
static const std::string MIME_OCTET_STREAM_CONTENT_TYPE = "application/octet-stream";
/// MIME boundary string prefix in HTTP header.
static const std::string BOUNDARY_PREFIX = "boundary=";
/// Size in chars of the MIME boundary string prefix
static const int BOUNDARY_PREFIX_SIZE = BOUNDARY_PREFIX.size();

/// String to identify log entries originating from this file.
static const std::string TAG{"ResponseSink"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

ResponseSink::ResponseSink(const std::shared_ptr<DavsRequest>& request, const std::string& workingDirectory) :
        m_parentDirectory(workingDirectory) {
    m_request = request;
    m_contentType = ContentType::NONE;
    m_boundary = "";
}
void ResponseSink::setContentType(const std::string& type) {
    if (type == MIME_JSON_CONTENT_TYPE) {
        m_contentType = ContentType::JSON;
    } else if (type == MIME_OCTET_STREAM_CONTENT_TYPE && nullptr == m_attachment) {
        m_contentType = ContentType::ATTACHMENT;
        filesystem::makeDirectory(m_parentDirectory);
        // file name is just id since there is no s3 URL.
        if (nullptr == m_artifact) {
            ACSDK_ERROR(LX("setContentType").m("Artifact was null can't create Attachment"));
            return;
        }
        m_artifactPath = m_parentDirectory + "/" + m_artifact->getId();
        if (!filesystem::pathContainsPrefix(m_artifactPath, m_parentDirectory)) {
            ACSDK_ERROR(LX("setContentType").m("Invalid URL file path").d("path", m_artifactPath));
        }
        m_attachment = DownloadStream::create(m_artifactPath, m_artifact->getArtifactSizeBytes());
    } else {
        ACSDK_ERROR(LX("setContentType").m("Unexpected Content Type for Multipart"));
        m_contentType = ContentType::NONE;
    }
}

void ResponseSink::setData(const char* buffer, size_t size) {
    if (nullptr == buffer) {
        ACSDK_ERROR(LX("setData").m("Data is null, can't set data"));
        return;
    }
    if (m_contentType == ContentType::JSON) {
        m_jsonString.append(buffer, size);
    } else if (m_contentType == ContentType::ATTACHMENT) {
        m_attachment->write(buffer, size);
    }
}

void ResponseSink::endData() {
    if (m_contentType == ContentType::JSON) {
        m_artifact = VendableArtifact::create(m_request, m_jsonString, true);
    } else if (m_contentType == ContentType::ATTACHMENT) {
        ACSDK_INFO(LX("endData").m("Close the attachment"));
    }
}
std::shared_ptr<VendableArtifact> ResponseSink::getArtifact() {
    if (nullptr == m_artifact) {
        ACSDK_ERROR(LX("getArtifact").m("Response Sink Artifact is null"));
        return nullptr;
    }
    return m_artifact;
}

std::string ResponseSink::getArtifactPath() {
    return m_artifactPath;
}

bool ResponseSink::onHeader(const std::string& header) {
    // If the boundary is found skip the rest of the checks
    // which are time consuming
    if (!m_boundary.empty()) {
        return true;
    }
    auto line = CurlWrapper::getValueFromHeaders(header, "Content-Type");
    if (line.find(BOUNDARY_PREFIX) != std::string::npos) {
        auto startOfBoundary = line.substr(line.find(BOUNDARY_PREFIX));
        m_boundary = startOfBoundary.substr(BOUNDARY_PREFIX_SIZE, startOfBoundary.find("\r\n") - BOUNDARY_PREFIX_SIZE);
        return true;
    }
    return false;
}
bool ResponseSink::parser(std::shared_ptr<DownloadChunkQueue>& downloadChunkQueue) {
    if (nullptr == downloadChunkQueue) {
        ACSDK_ERROR(LX("parser").m("Download Chunk Queue is null"));
        return false;
    }
    auto dataChunk = downloadChunkQueue->waitAndPop();
    if (nullptr == dataChunk) {
        ACSDK_ERROR(LX("parser").m("Data Chunk didn't populate "));
        return false;
    }

    auto parser = MultipartReader();
    parser.onPartData = [](const char* buffer, size_t size, void* userData) {
        static_cast<ResponseSink*>(userData)->setData(buffer, size);
    };
    parser.onPartBegin = [](const MultipartHeaders& headers, void* userData) {
        auto contentType = headers["Content-Type"];
        ACSDK_DEBUG(LX("parser").d("Starting part", contentType.c_str()));
        static_cast<ResponseSink*>(userData)->setContentType(contentType);
    };
    parser.onPartEnd = [](void* userData) {
        ACSDK_DEBUG(LX("parser").m("Ending Current Data Part"));
        static_cast<ResponseSink*>(userData)->endData();
    };
    parser.userData = this;
    parser.setBoundary(m_boundary);
    while (dataChunk) {
        parser.feed(dataChunk->data(), dataChunk->size());
        if (parser.hasError()) {
            ACSDK_ERROR(LX("parser").m("Multipart Parser Error").d("error message", parser.getErrorMessage()));
            return false;
        }
        dataChunk = downloadChunkQueue->waitAndPop();
    }
    if (!downloadChunkQueue->popComplete(true)) {
        ACSDK_ERROR(LX("parser").m("Pop didn't complete properly"));
        return false;
    }

    return true;
}

}  // namespace common
}  // namespace acsdkAssets
}  // namespace alexaClientSDK