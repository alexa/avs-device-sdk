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

#ifndef ACSDKASSETMANAGERCLIENT_AMD_H_
#define ACSDKASSETMANAGERCLIENT_AMD_H_

namespace alexaClientSDK {
namespace acsdkAssets {
namespace client {

class AMD {
public:
    static constexpr const char* PUBLISHER = "com.amazon.assetmgrd";
    static constexpr const char* REGISTER_PROP = "RegisterArtifact";
    static constexpr const char* REMOVE_PROP = "RemoveArtifact";
    static constexpr const char* INITIALIZATION_PROP = "Initialization";

    static constexpr const char* ACCEPT_UPDATE_PROP = "AcceptUpdate";
    static constexpr const char* REJECT_UPDATE_PROP = "RejectUpdate";

    static constexpr const char* STATE_SUFFIX = "_State";
    static constexpr const char* PRIORITY_SUFFIX = "_Priority";
    static constexpr const char* PATH_SUFFIX = "_Path";
    static constexpr const char* UPDATE_SUFFIX = "_Update";
};

}  // namespace client
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKASSETMANAGERCLIENT_AMD_H_
