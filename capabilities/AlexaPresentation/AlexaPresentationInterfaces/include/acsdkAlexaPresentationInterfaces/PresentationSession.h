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

#ifndef ACSDKALEXAPRESENTATIONINTERFACES_PRESENTATIONSESSION_H_
#define ACSDKALEXAPRESENTATIONINTERFACES_PRESENTATIONSESSION_H_

#include <string>
#include <vector>

namespace alexaClientSDK {
namespace acsdkAlexaPresentationInterfaces {

struct GrantedExtension {
    // URI of extension to allow in document.  If the APL document requests an
    // extension that is not part of grantedExtensions list, extension will be
    // denied.
    std::string uri;
};

struct AutoInitializedExtension {
    // URI of extension to auto initialize
    std::string uri;

    // Extension settings for initializing the extension on the device.
    std::string settings;
};

struct PresentationSession {
    /// ID of skill that owns the presentation session.
    std::string skillId;

    /// Unique string identifying the instance of a skill.
    std::string id;

    /// List of extensions that are granted for use by this APL document
    std::vector<GrantedExtension> grantedExtensions;

    /// List of extensions that are initialized in the APL runtime for this
    /// document.
    std::vector<AutoInitializedExtension> autoInitializedExtensions;
};

}  // namespace acsdkAlexaPresentationInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKALEXAPRESENTATIONINTERFACES_PRESENTATIONSESSION_H_