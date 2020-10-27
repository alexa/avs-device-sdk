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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_CAPABILITYSEMANTICS_ACTIONSTODIRECTIVEMAPPING_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_CAPABILITYSEMANTICS_ACTIONSTODIRECTIVEMAPPING_H_

#include <string>
#include <vector>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace capabilitySemantics {

/**
 * This class represents an "ActionsToDirective" type "actionMapping" in a semantic annotation for a capability
 * primitive.
 *
 * @sa https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/capability-primitives.html#semantic-annotation
 */
class ActionsToDirectiveMapping {
public:
    /**
     * The constructor.
     */
    ActionsToDirectiveMapping();

    /**
     * Adds the specified action identifier to the "actions" array of this mapping object. The action identifier
     * represents utterances that should trigger the directive specified in @c setDirective().
     *
     * @note See the class-level link for supported action identifiers.
     *
     * @param action The identifier of the action to add to the "actions" array.
     * @return @c true if the action was successfully added, else @c false.
     */
    bool addAction(const std::string& actionId);

    /**
     * Sets the "directive" field of this mapping object. The action IDs specified with calls to @c addAction()
     * correspond to this directive name and payload.
     *
     * @param name The name of the directive mapped to the action(s). Must be a valid directive of the capability
     *        interface to which the semantics object belongs.
     * @param payload The desired payload of the directive.
     * @return @c true if the directive was successfully set, else @c false.
     */
    bool setDirective(const std::string& name, const std::string& payload = "{}");

    /**
     * Checks whether this @c ActionsToDirectiveMapping is valid.
     *
     * @return @c true if valid, else @c false.
     */
    bool isValid() const;

    /**
     * Converts this @c ActionsToDirectiveMapping to a JSON string.
     *
     * @note This follows the AVS discovery message format.
     *
     * @return A JSON string of this @c ActionsToDirectiveMapping.
     */
    std::string toJson() const;

private:
    /// Indicates an error in construction.
    bool m_isValid;

    /// List of action IDs used in this mapping.
    std::vector<std::string> m_actions;

    /// The name of the directive used in this mapping.
    std::string m_directiveName;

    /// The directive payload used in this mapping.
    std::string m_directivePayload;
};

}  // namespace capabilitySemantics
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_CAPABILITYSEMANTICS_ACTIONSTODIRECTIVEMAPPING_H_
