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

#include <rapidjson/writer.h>

#include <AVSCommon/Utils/JSON/JSONUtils.h>

#include "IPCServerSampleApp/AlexaPresentation/APLPayloadParser.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
using namespace avsCommon::utils::json;

static const std::string TAG{"APLPayloadParser"};
#define LX(event) avsCommon::utils::logger::LogEntry(TAG, event)

/// Identifier for the document sent in an APL directive
static const std::string DOCUMENT_FIELD = "document";

/// Identifier for the datasources sent in an APL directive
static const std::string DATASOURCES_FIELD = "datasources";

/// Identifier for the supportedViewports array sent in an APL directive
static const std::string SUPPORTED_VIEWPORTS_FIELD = "supportedViewports";

/// Empty JSON for unparsed values
static const std::string EMPTY_JSON = "{}";

/// Identifier for the presentationSession sent in a RenderDocument directive
static const std::string PRESENTATION_SESSION_FIELD{"presentationSession"};

/// Identifier for the skilld in presentationSession
static const std::string SKILL_ID{"skillId"};

/// Identifier for the id in presentationSession
static const std::string PRESENTATION_SESSION_ID{"id"};

/// Identifier for the grantedExtensions in presentationSession
static const std::string PRESENTATION_SESSION_GRANTEDEXTENSIONS{"grantedExtensions"};

/// Identifier for the autoInitializedExtensions in presentationSession
static const std::string PRESENTATION_SESSION_AUTOINITIALIZEDEXTENSIONS{"autoInitializedExtensions"};

//// Identifier for the uri in grantedExtensions or autoInitializedExtensions
static const std::string PRESENTATION_SESSION_URI{"uri"};

/// Identifier for the settings in autoInitializedExtensions
static const std::string PRESENTATION_SESSION_SETTINGS{"settings"};

/// Identifier for the timeoutType sent in a RenderDocument directive
static const std::string TIMEOUTTYPE_FIELD = "timeoutType";

std::string APLPayloadParser::extractDocument(const rapidjson::Document& document) {
    std::string aplDocument;
    if (!jsonUtils::retrieveValue(document, DOCUMENT_FIELD, &aplDocument)) {
        aplDocument = EMPTY_JSON;
    }

    return aplDocument;
}

std::string APLPayloadParser::extractDatasources(const rapidjson::Document& document) {
    std::string aplData;
    if (!jsonUtils::retrieveValue(document, DATASOURCES_FIELD, &aplData)) {
        aplData = EMPTY_JSON;
    }

    return aplData;
}

std::string APLPayloadParser::extractSupportedViewports(const rapidjson::Document& document) {
    std::string supportedViewports;
    rapidjson::Value::ConstMemberIterator jsonIt;
    if (!jsonUtils::findNode(document, SUPPORTED_VIEWPORTS_FIELD, &jsonIt)) {
        supportedViewports = EMPTY_JSON;
    } else {
        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);

        if (!jsonIt->value.Accept(writer)) {
            ACSDK_ERROR(LX("extractSupportedViewportsFailed").d("reason", "Error serializing json iterator payload"));
            return EMPTY_JSON;
        }

        supportedViewports = sb.GetString();
    }

    return supportedViewports;
}

aplCapabilityCommonInterfaces::APLTimeoutType APLPayloadParser::extractTimeoutType(
    const rapidjson::Document& document) {
    std::string timeoutTypeStr;
    if (!jsonUtils::retrieveValue(document, TIMEOUTTYPE_FIELD, &timeoutTypeStr)) {
        ACSDK_WARN(LX("extractTimeoutTypeFailed").d("reason", "Missing timeoutType field, using SHORT"));
        return aplCapabilityCommonInterfaces::APLTimeoutType::SHORT;
    }

    auto timeoutType = aplCapabilityCommonInterfaces::convertToTimeoutType(timeoutTypeStr);

    if (!timeoutType.hasValue()) {
        ACSDK_WARN(LX("extractTimeoutTypeFailed").d("reason", "Invalid timeoutType field, using SHORT"));
    }

    return timeoutType.valueOr(aplCapabilityCommonInterfaces::APLTimeoutType::SHORT);
}

const aplCapabilityCommonInterfaces::PresentationSession APLPayloadParser::extractPresentationSession(
    const rapidjson::Document& document) {
    rapidjson::Value::ConstMemberIterator iterator;
    std::string presentationSessionPayload;
    if (!jsonUtils::findNode(document, PRESENTATION_SESSION_FIELD, &iterator) ||
        !jsonUtils::convertToValue(iterator->value, &presentationSessionPayload)) {
        ACSDK_WARN(
            LX("extractPresentationSessionFailed").d("reason", "Unable to retrieve presentationSession payload"));
        return {};
    }

    std::string skillId;
    if (!jsonUtils::retrieveValue(presentationSessionPayload, SKILL_ID, &skillId)) {
        ACSDK_WARN(LX(__func__).m("Failed to find presentationSession skillId"));
    }

    std::string id;
    if (!jsonUtils::retrieveValue(presentationSessionPayload, PRESENTATION_SESSION_ID, &id)) {
        ACSDK_WARN(LX(__func__).m("Failed to find presentationSession id"));
    }

    rapidjson::Document doc;
    doc.Parse(presentationSessionPayload);
    std::vector<aplCapabilityCommonInterfaces::GrantedExtension> grantedExtensions;
    if (doc.HasMember(PRESENTATION_SESSION_GRANTEDEXTENSIONS) &&
        doc[PRESENTATION_SESSION_GRANTEDEXTENSIONS].IsArray()) {
        auto grantExtensionArray = doc[PRESENTATION_SESSION_GRANTEDEXTENSIONS].GetArray();
        for (auto& itr : grantExtensionArray) {
            aplCapabilityCommonInterfaces::GrantedExtension grantedExtension;
            if (itr.HasMember(PRESENTATION_SESSION_URI) && itr[PRESENTATION_SESSION_URI].IsString()) {
                grantedExtension.uri = itr[PRESENTATION_SESSION_URI].GetString();
                grantedExtensions.push_back(grantedExtension);
            } else {
                ACSDK_WARN(LX(__func__).m("Error parsing grantedExtensions"));
            }
        }
    } else {
        ACSDK_WARN(LX(__func__).m("Failed to find presentationSession grantedExtensions"));
    }

    std::vector<aplCapabilityCommonInterfaces::AutoInitializedExtension> autoInitializedExtensions;
    if (doc.HasMember(PRESENTATION_SESSION_AUTOINITIALIZEDEXTENSIONS) &&
        doc[PRESENTATION_SESSION_AUTOINITIALIZEDEXTENSIONS].IsArray()) {
        auto autoInitializedExtensionArray = doc[PRESENTATION_SESSION_AUTOINITIALIZEDEXTENSIONS].GetArray();
        for (auto& itr : autoInitializedExtensionArray) {
            aplCapabilityCommonInterfaces::AutoInitializedExtension autoInitializedExtension;
            if (itr.HasMember(PRESENTATION_SESSION_URI) && itr[PRESENTATION_SESSION_URI].IsString() &&
                itr.HasMember(PRESENTATION_SESSION_SETTINGS) && itr[PRESENTATION_SESSION_SETTINGS].IsString()) {
                autoInitializedExtension.uri = itr[PRESENTATION_SESSION_URI].GetString();
                autoInitializedExtension.settings = itr[PRESENTATION_SESSION_SETTINGS].GetString();
                autoInitializedExtensions.push_back(autoInitializedExtension);
            } else {
                ACSDK_WARN(LX(__func__).m("Error parsing autoInitializedExtensions"));
            }
        }
    } else {
        ACSDK_WARN(LX(__func__).m("Failed to find presentationSession autoInitializedExtensions"));
    }

    aplCapabilityCommonInterfaces::PresentationSession presentationSession = {
        skillId, id, grantedExtensions, autoInitializedExtensions};

    return presentationSession;
}
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
