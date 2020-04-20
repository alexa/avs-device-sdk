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

#include <AVSCommon/AVS/DirectiveRoutingRule.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace directiveRoutingRule {

/// String to identify log entries originating from this file.
static const std::string TAG("DirectiveRoutingRule");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

// String used to represent a wildcard.
static const std::string WILDCARD = "*";

DirectiveRoutingRule routingRulePerDirective(
    const std::string& endpointId,
    const utils::Optional<std::string>& instance,
    const std::string& nameSpace,
    const std::string& name) {
    return DirectiveRoutingRule(nameSpace, name, endpointId, instance);
}

DirectiveRoutingRule routingRulePerNamespace(
    const std::string& endpointId,
    const utils::Optional<std::string>& instance,
    const std::string& nameSpace) {
    return DirectiveRoutingRule(nameSpace, WILDCARD, endpointId, instance);
}

DirectiveRoutingRule routingRulePerInstance(
    const std::string& endpointId,
    const utils::Optional<std::string>& instance) {
    return DirectiveRoutingRule(WILDCARD, WILDCARD, endpointId, instance);
}

DirectiveRoutingRule routingRulePerNamespaceAnyInstance(const std::string& endpointId, const std::string& nameSpace) {
    return DirectiveRoutingRule(nameSpace, WILDCARD, endpointId, WILDCARD);
}

DirectiveRoutingRule routingRulePerEndpoint(const std::string& endpointId) {
    return DirectiveRoutingRule(WILDCARD, WILDCARD, endpointId, WILDCARD);
}

bool isDirectiveRoutingRuleValid(const DirectiveRoutingRule& rule) {
    if (rule.endpointId == WILDCARD) {
        // No wildcard allowed for endpoint id.
        ACSDK_ERROR(LX("isDirectiveRoutingRuleValidFailed").d("result", "invalidEndpointWildcard"));
        return false;
    }
    if (((rule.instance.valueOr("") == WILDCARD) || (rule.nameSpace == WILDCARD)) && (rule.name != WILDCARD)) {
        // Name must be a wildcard for every rule that uses wildcard.
        ACSDK_ERROR(LX("isDirectiveRoutingRuleValidFailed")
                        .d("result", "invalidWildcardUsage")
                        .d("instance", rule.instance.valueOr(""))
                        .d("namespace", rule.nameSpace)
                        .d("name", rule.name));
        return false;
    }

    return true;
}

}  // namespace directiveRoutingRule
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
