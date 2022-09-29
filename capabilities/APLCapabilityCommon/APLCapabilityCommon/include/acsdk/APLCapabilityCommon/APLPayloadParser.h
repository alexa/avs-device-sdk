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

#ifndef ACSDK_APLCAPABILITYCOMMON_APLPAYLOADPARSER_H_
#define ACSDK_APLCAPABILITYCOMMON_APLPAYLOADPARSER_H_

#include <string>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/document.h>

#include <acsdk/APLCapabilityCommonInterfaces/PresentationOptions.h>
#include <acsdk/APLCapabilityCommonInterfaces/PresentationSession.h>
#include <acsdk/APLCapabilityCommonInterfaces/APLTimeoutType.h>

namespace alexaClientSDK {
namespace aplCapabilityCommon {

class APLPayloadParser {
public:
    /**
     * Extract the document from APL payload.
     *
     * @param jsonPayload rapidjson DOM instance of payload.
     * @return string containing the json value of document, empty string will be set if parsing fails
     */
    static std::string extractDocument(const rapidjson::Document& jsonPayload);

    /**
     * Extract the datasourse from APL payload.
     *
     * @param jsonPayload rapidjson DOM instance of payload.
     * @return string containing the json value of datasource
     */
    static std::string extractDatasources(const rapidjson::Document& jsonPayload);

    /**
     * Extract the supported viewports from APL payload.
     *
     * @param jsonPayload rapidjson DOM instance of payload.
     * @return string containing the json value of supported viewports.
     */
    static std::string extractSupportedViewports(const rapidjson::Document& jsonPayload);

    /**
     * Extract the timeout type from APL payload.
     *
     * @param jsonPayload rapidjson DOM instance of payload.
     * @return string containing the lifespan of the APL payload.
     */
    static std::string extractAPLTimeoutType(const rapidjson::Document& jsonPayload);

    /**
     * Extract the presentation session instance from APL payload.
     *
     * @param jsonPayload rapidjson DOM instance of payload.
     * @return Instance of @c PresentationSession object.
     */
    static const alexaClientSDK::aplCapabilityCommonInterfaces::PresentationSession extractPresentationSession(
        const std::string& skillIdFieldName,
        const std::string& presentationSkilId,
        const rapidjson::Document& jsonPayload);
    /**
     * This function deserializes a @c Directive's payload into a @c rapidjson::Document.
     *
     * @param jsonPayload rapidjson DOM instance of payload.
     * @param[out] document The @c rapidjson::Document to parse the payload into.
     * @return @c true if parsing was successful, else @c false.
     */
    static bool parseDirectivePayload(const std::string& jsonPayload, rapidjson::Document* document);
    /**
     * Extract the presentation session instance from APL payload.
     *
     * @param jsonPayload rapidjson DOM instance of payload.
     * @param[out] token string containing the token
     *
     */
    static bool extractPresentationToken(const rapidjson::Document& jsonPayload, std::string& token);
    /**
     * Get the target windowId from payload of renderDocument message with APL document
     *
     * @param jsonPayload rapidjson DOM instance of payload.
     * @return The windowId for APL payload, empty string otherwise.
     */
    static const std::string extractWindowId(const rapidjson::Document& jsonPayload);
};
}  // namespace aplCapabilityCommon
}  // namespace alexaClientSDK
#endif  // ACSDK_APLCAPABILITYCOMMON_APLPAYLOADPARSER_H_
