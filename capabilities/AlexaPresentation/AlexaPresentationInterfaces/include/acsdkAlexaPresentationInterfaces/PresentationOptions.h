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

#ifndef ACSDKALEXAPRESENTATIONINTERFACES_PRESENTATIONOPTIONS_H_
#define ACSDKALEXAPRESENTATIONINTERFACES_PRESENTATIONOPTIONS_H_

#include <chrono>
#include <string>
#include <vector>

namespace alexaClientSDK {
namespace acsdkAlexaPresentationInterfaces {

enum class VisualFocusBehavior {
    /// The content can be backgrounded, if another presentation takes the window
    MAY_BACKGROUND,

    /// The content cannot be backgrounded, if another presentation takes focus
    /// this presentation must be dismissed
    MUST_DISMISS,

    /// The content is on top at all times, other presentations cannot take focus
    ALWAYS_FOREGROUND
};

enum class PresentationLifespan {
    /// A short lived presentation which cannot be backgrounded, upon timeout will be
    /// dismissed and the next SHORT, LONG or PERMANENT presentation will be resumed
    TRANSIENT,

    /// A short lived presentation, not generally backgrounded but can be if a transient
    /// presentation is displayed. Upon timeout the next LONG or PERMANENT presentation
    /// will be resumed
    SHORT,

    /// A long lived presentation, may not have a timeout attached to it - will be
    /// backgrounded if another presentation is displayed
    LONG,

    /// Special use case for applications that are always running and are not expected
    /// to terminate, for example home screens. Permanent presentations can be
    /// backgrounded but cannot be dismissed as a result of back navigation.
    PERMANENT
};

struct PresentationOptions {
    /// Window ID corresponding to a window reported in Alexa.Display
    std::string windowId;

    /// The timeout for the document
    std::chrono::milliseconds timeout;

    /// @deprecated
    /// Specifies how this presentation should behave if another presentation
    /// attempts to take foreground focus
    VisualFocusBehavior visualFocusBehavior;

    /// The AVS namespace associated with this presentation
    std::string interface;

    /// The presentation token that will be reported in Alexa.Display.WindowState
    std::string token;

    /// Specifies the lifespan type for this presentation
    PresentationLifespan lifespan;

    /// Specifies the timestamp when the document was received
    std::chrono::steady_clock::time_point documentReceivedTimestamp;
};

}  // namespace acsdkAlexaPresentationInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKALEXAPRESENTATIONINTERFACES_PRESENTATIONOPTIONS_H_
