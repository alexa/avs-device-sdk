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

#include "RequestFactory.h"

#include <AVSCommon/Utils/Logger/Logger.h>

#include "acsdkAssetsInterfaces/DavsRequest.h"
#include "acsdkAssetsInterfaces/UrlRequest.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace manager {

using namespace std;
using namespace rapidjson;
using namespace commonInterfaces;

static const std::string TAG{"RequestFactory"};
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

static const char* ARTIFACT_TYPE = "artifactType";
static const char* ARTIFACT_KEY = "artifactKey";
static const char* ARTIFACT_FILTERS = "filters";
static const char* ARTIFACT_UNPACK = "unpack";
static const char* ARTIFACT_ENDPOINT = "endpoint";

static const char* ARTIFACT_URL = "url";
static const char* ARTIFACT_FILENAME = "filename";
static const char* ARTIFACT_CERT_PATH = "certPath";

shared_ptr<DavsRequest> createDavsRequest(const Document& document) {
    if (document.HasParseError() || !document.IsObject()) {
        ACSDK_ERROR(LX("createDavsRequest").m("Can't parse JSON Document"));
        return nullptr;
    }

    if (!document.HasMember(ARTIFACT_TYPE) || !document.HasMember(ARTIFACT_KEY) ||
        !document.HasMember(ARTIFACT_FILTERS)) {
        ACSDK_WARN(LX("createDavsRequest").m("Information missing from metadata, not a proper DAVS Request"));
        return nullptr;
    }

    DavsRequest::FilterMap filterMap;
    for (auto& filters : document[ARTIFACT_FILTERS].GetObject()) {
        auto& filterSet = filterMap[filters.name.GetString()];
        if (!filters.value.IsArray()) {
            filterSet.insert(filters.value.GetString());
            continue;
        }

        for (auto& filter : filters.value.GetArray()) {
            filterSet.insert(filter.GetString());
        }
    }

    // get optional fields
    auto endpoint = Region::NA;
    if (document.HasMember(ARTIFACT_ENDPOINT)) {
        auto& endpointMember = document[ARTIFACT_ENDPOINT];
        if (endpointMember.IsInt()) {
            endpoint = static_cast<Region>(endpointMember.GetInt());
        }
    }
    auto unpack = false;
    if (document.HasMember(ARTIFACT_UNPACK)) {
        auto& unpackMember = document[ARTIFACT_UNPACK];
        if (unpackMember.IsBool()) {
            unpack = unpackMember.GetBool();
        }
    };

    auto request = DavsRequest::create(
            document[ARTIFACT_TYPE].GetString(), document[ARTIFACT_KEY].GetString(), filterMap, endpoint, unpack);
    if (request == nullptr) {
        ACSDK_ERROR(LX("createDavsRequest").m("Could not create the appropriate Artifact Request"));
        return nullptr;
    }

    return request;
}

shared_ptr<UrlRequest> createUrlRequest(const Document& document) {
    if (document.HasParseError() || !document.IsObject()) {
        ACSDK_ERROR(LX("createUrlRequest").m("Can't parse JSON Document"));
        return nullptr;
    }

    if (!document.HasMember(ARTIFACT_URL) || !document.HasMember(ARTIFACT_FILENAME)) {
        ACSDK_WARN(LX("createUrlRequest").m("Information missing from metadata, not a proper URL Request"));
        return nullptr;
    }

    auto unpack = false;
    if (document.HasMember(ARTIFACT_UNPACK)) {
        auto& unpackMember = document[ARTIFACT_UNPACK];
        if (unpackMember.IsBool()) {
            unpack = unpackMember.GetBool();
        }
    }

    auto certPath = "";
    if (document.HasMember(ARTIFACT_CERT_PATH)) {
        ACSDK_INFO(LX("createUrlRequest")
                           .m("document using custom cert from path")
                           .d("Path", document[ARTIFACT_CERT_PATH].GetString()));
        certPath = document[ARTIFACT_CERT_PATH].GetString();
    }

    auto request = UrlRequest::create(
            document[ARTIFACT_URL].GetString(), document[ARTIFACT_FILENAME].GetString(), unpack, certPath);
    if (request == nullptr) {
        ACSDK_ERROR(LX("createUrlRequest").m("Could not create the appropriate Artifact Request"));
        return nullptr;
    }

    return request;
}

std::shared_ptr<ArtifactRequest> RequestFactory::create(const Document& document) {
    std::shared_ptr<ArtifactRequest> request = createDavsRequest(document);
    if (request != nullptr) {
        return request;
    }

    request = createUrlRequest(document);
    if (request != nullptr) {
        return request;
    }

    return nullptr;
}

std::shared_ptr<ArtifactRequest> RequestFactory::create(const string& jsonString) {
    rapidjson::Document document;
    return create(document.Parse(jsonString));
}

}  // namespace manager
}  // namespace acsdkAssets
}  // namespace alexaClientSDK