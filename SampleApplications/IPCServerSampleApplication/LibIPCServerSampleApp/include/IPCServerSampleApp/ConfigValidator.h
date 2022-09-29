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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_CONFIGVALIDATOR_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_CONFIGVALIDATOR_H_

#include <unordered_map>

#include <AVSCommon/Utils/Logger/Logger.h>

#include <rapidjson/document.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {

class ConfigValidator {
public:
    /**
     * Constructor.
     */
    ConfigValidator();

    /**
     * Default destructor.
     */
    ~ConfigValidator() = default;

    /**
     * Validates a configuration node using the json schema file.
     *
     * @param configuration The @c ConfigurationNode configuration object. The configuration object will be validated
     * against the schema file for errors.
     * @param jsonSchema The json schema that contains the representation of the configuration object.
     * This schema is expected to comply with JSON Schema Draft v4 specification which is currently the latest version
     * of JSON Schema supported by rapidjson v1.1. For documentation details, refer to:
     * http://json-schema.org/specification.html
     * @return boolean Indicates if validation of the configuration object was successful
     */
    bool validate(avsCommon::utils::configuration::ConfigurationNode& configuration, rapidjson::Document& jsonSchema);

    /**
     * Create a new ConfigValidator.
     *
     * @return The ConfigValidator object
     */
    static std::shared_ptr<ConfigValidator> create();
};

}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_CONFIGVALIDATOR_H_
