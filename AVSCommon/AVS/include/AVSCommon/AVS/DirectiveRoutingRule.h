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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_DIRECTIVEROUTINGRULE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_DIRECTIVEROUTINGRULE_H_

#include "AVSCommon/AVS/CapabilityTag.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace directiveRoutingRule {

/**
 * Alias used to define a directive routing rule. Routing rules may have wildcards to match any value in a given field.
 *
 * The following rules will be accepted.
 *
 *   - Per directive: {EndpointId, Instance, Namespace, Name}
 *   - Per namespace: {EndpointId, Instance, Namespace, *}
 *   - Per instance: {EndpointId, Instance, *, *}
 *   - Per namespace for any instance: {EndpointId, *, Namespace, *}
 *   - Per endpoint: {EndpointId, *, *, *}
 *
 * Note that we used '*' to represent wildcards.
 */
using DirectiveRoutingRule = CapabilityTag;

/**
 * Function used to create a directive routing rule that matches one specific directive.
 *
 * @param endpointId The directive target endpoint identifier.
 * @param instance The directive instance if available.
 * @param nameSpace The directive namespace.
 * @param name The directive name.
 */
DirectiveRoutingRule routingRulePerDirective(
    const std::string& endpointId,
    const utils::Optional<std::string>& instance,
    const std::string& nameSpace,
    const std::string& name);

/**
 * Function used to create a directive routing rule that matches one specific directive.
 *
 * @param endpointId The directive target endpoint identifier.
 * @param instance The directive instance if available.
 * @param nameSpace The directive namespace.
 */
DirectiveRoutingRule routingRulePerNamespace(
    const std::string& endpointId,
    const utils::Optional<std::string>& instance,
    const std::string& nameSpace);

/**
 * Function used to create a directive routing rule that matches one specific directive.
 *
 * @param endpointId The directive target endpoint identifier.
 * @param instance The directive instance if available.
 */
DirectiveRoutingRule routingRulePerInstance(
    const std::string& endpointId,
    const utils::Optional<std::string>& instance);

/**
 * Function used to create a directive routing rule that matches a specific namespace independently from their instance.
 *
 * @param endpointId The directive target endpoint identifier.
 * @param nameSpace The directive namespace.
 */
DirectiveRoutingRule routingRulePerNamespaceAnyInstance(const std::string& endpointId, const std::string& nameSpace);

/**
 * Function used to create a directive routing rule that matches one specific directive.
 *
 * @param endpointId The directive target endpoint identifier.
 */
DirectiveRoutingRule routingRulePerEndpoint(const std::string& endpointId);

/**
 * Function used to validate that a directive routing rule is valid.
 *
 * @param rule The rule to validate.
 * @return Whether the rule is valid or not.
 */
bool isDirectiveRoutingRuleValid(const DirectiveRoutingRule& rule);

}  // namespace directiveRoutingRule
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_DIRECTIVEROUTINGRULE_H_
