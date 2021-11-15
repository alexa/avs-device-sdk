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

#include "acsdkDavsClient/DavsEndpointHandlerV3.h"

#include <AVSCommon/Utils/Error/SuccessResult.h>
#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "acsdkAssetsCommon/Base64Url.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace davs {

using namespace std;
using namespace commonInterfaces;
using namespace common;
using namespace avsCommon::utils::json;
using namespace avsCommon::utils::error;

static const string TAG{"DavsEndpointHandlerV3"};
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Generate encoded filters using filter map from a request
static SuccessResult<string> generatedEncodedFilters(const DavsRequest::FilterMap& filtersMap) {
    if (filtersMap.empty()) {
        return SuccessResult<string>::success("");
    }

    JsonGenerator generator;
    for (const auto& filter : filtersMap) {
        generator.addStringArray(filter.first, filter.second);
    }

    string filtersEncoded;
    auto filtersJson = generator.toString();

    if (!Base64Url::encode(filtersJson, filtersEncoded)) {
        ACSDK_ERROR(LX("generatedEncodedFilters").m("Could not encode request"));
        return SuccessResult<string>::failure();
    }
    return SuccessResult<string>::success(filtersEncoded);
}

/// get the proper endpoint for davs per region
static string getUrlEndpoint(Region endpoint) {
    switch (endpoint) {
        case Region::NA:
            return "api.amazonalexa.com";
        case Region::EU:
            return "api.eu.amazonalexa.com";
        case Region::FE:
            return "api.fe.amazonalexa.com";
    }
    return "";
}

std::shared_ptr<DavsEndpointHandlerV3> DavsEndpointHandlerV3::create(
        const string& segmentId,
        const std::string& locale) {
    if (segmentId.empty()) {
        return nullptr;
    }

    return shared_ptr<DavsEndpointHandlerV3>(new DavsEndpointHandlerV3(segmentId, locale));
}

DavsEndpointHandlerV3::DavsEndpointHandlerV3(std::string segmentId, std::string locale) :
        m_segmentId(move(segmentId)),
        m_locale(move(locale)) {
}

string DavsEndpointHandlerV3::getDavsUrl(shared_ptr<DavsRequest> request) {
    if (request == nullptr) {
        ACSDK_ERROR(LX("getDavsUrl").m("Null DavsRequest"));
        return "";
    }

    auto encodedFilters = generatedEncodedFilters(request->getFilters());
    if (!encodedFilters.isSucceeded()) {
        ACSDK_ERROR(LX("getDavsUrl").m("Failed to generate encoded filters"));
        return "";
    }

    auto requestUrl = "https://" + getUrlEndpoint(request->getRegion()) + "/v3/segments/" + m_segmentId +
                      "/artifacts/" + request->getType() + "/" + request->getKey();
    if (!m_locale.empty()) {
        requestUrl += "?locale=" + m_locale;
    }

    if (!encodedFilters.value().empty()) {
        requestUrl += m_locale.empty() ? "?" : "&";
        requestUrl += "encodedFilters=" + encodedFilters.value();
    }

    return requestUrl;
}
void DavsEndpointHandlerV3::setLocale(std::string newLocale) {
    m_locale = move(newLocale);
}

}  // namespace davs
}  // namespace acsdkAssets
}  // namespace alexaClientSDK
