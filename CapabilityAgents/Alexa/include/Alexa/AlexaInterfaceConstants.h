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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALEXA_INCLUDE_ALEXA_ALEXAINTERFACECONSTANTS_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALEXA_INCLUDE_ALEXA_ALEXAINTERFACECONSTANTS_H_

#include <string>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alexa {

/// AlexaInterface interface type
static const std::string ALEXA_INTERFACE_TYPE = "AlexaInterface";

/// AlexaInterface interface name
static const std::string ALEXA_INTERFACE_NAME = "Alexa";

/// AlexaInterface interface version.
static const std::string ALEXA_INTERFACE_VERSION = "3";

}  // namespace alexa
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALEXA_INCLUDE_ALEXA_ALEXAINTERFACECONSTANTS_H_
