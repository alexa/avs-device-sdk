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

#ifndef ACSDKASSETSINTERFACES_ARTIFACTREQUEST_H_
#define ACSDKASSETSINTERFACES_ARTIFACTREQUEST_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "Type.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace commonInterfaces {

/**
 * Artifact Request will be a common parent class for all types of request that will be supported by Asset Manager
 * and will maintain the common interface needed to represent a unique request.
 */
class ArtifactRequest {
public:
    static constexpr auto UNPACK = true;

    /**
     * Destructor.
     */
    virtual ~ArtifactRequest() = default;

    /**
     * @return the type of the request.
     */
    virtual Type getRequestType() const = 0;

    /**
     * @return weather the artifact needs to be unpacked or not.
     */
    virtual bool needsUnpacking() const = 0;

    /**
     * @return a concatenated string that describes the request.
     */
    virtual std::string getSummary() const = 0;

    /**
     * @return a JSON representation of this request that includes all of its component.
     */
    virtual std::string toJsonString() const = 0;
};

/** Use summary to determine if two requests are identical */
inline bool operator==(const ArtifactRequest& lhs, const ArtifactRequest& rhs) {
    return lhs.getSummary() == rhs.getSummary();
}

inline bool operator!=(const ArtifactRequest& lhs, const ArtifactRequest& rhs) {
    return !(lhs == rhs);
}

}  // namespace commonInterfaces
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKASSETSINTERFACES_ARTIFACTREQUEST_H_
