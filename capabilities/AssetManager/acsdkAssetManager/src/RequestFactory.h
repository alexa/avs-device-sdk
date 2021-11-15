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

#ifndef ACSDKASSETMANAGER_SRC_REQUESTFACTORY_H_
#define ACSDKASSETMANAGER_SRC_REQUESTFACTORY_H_

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "acsdkAssetsInterfaces/DavsRequest.h"
#include "acsdkAssetsInterfaces/UrlRequest.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace manager {

class RequestFactory {
public:
    /**
     * Creates an Artifact Request from a json Document.
     *
     * @param document REQUIRED, document containing json information for the request.
     * @return NULLABLE, a smart pointer to a request of the document if parsed correctly.
     */
    static std::shared_ptr<commonInterfaces::ArtifactRequest> create(const rapidjson::Document& document);

    /**
     * Creates an Artifact Request from a json string.
     *
     * @param json REQUIRED, json string containing information for the request.
     * @return NULLABLE, a smart pointer to a request of the json string if parsed correctly.
     */
    static std::shared_ptr<commonInterfaces::ArtifactRequest> create(const std::string& jsonString);
};

}  // namespace manager
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKASSETMANAGER_SRC_REQUESTFACTORY_H_
