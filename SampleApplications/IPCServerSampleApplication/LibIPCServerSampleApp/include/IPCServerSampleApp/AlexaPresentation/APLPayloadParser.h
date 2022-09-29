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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_ALEXAPRESENTATION_APLPAYLOADPARSER_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_ALEXAPRESENTATION_APLPAYLOADPARSER_H_

#include <string>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/document.h>

#include <acsdk/APLCapabilityCommonInterfaces/APLCapabilityAgentInterface.h>
#include <acsdk/APLCapabilityCommonInterfaces/APLTimeoutType.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
class APLPayloadParser {
public:
    /**
     * Extract the document from APL payload.
     *
     * @param jsonPayload rapidjson DOM instance of payload.
     * @return string containing the json value of document
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
     * @return TimeoutType of the APL payload.
     */
    static aplCapabilityCommonInterfaces::APLTimeoutType extractTimeoutType(const rapidjson::Document& jsonPayload);

    /**
     * Extract the presentation session instance from APL payload.
     *
     * @param jsonPayload rapidjson DOM instance of payload.
     * @return Instance of @c PresentationSession object.
     */
    static const aplCapabilityCommonInterfaces::PresentationSession extractPresentationSession(
        const rapidjson::Document& jsonPayload);
};
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_ALEXAPRESENTATION_APLPAYLOADPARSER_H_
