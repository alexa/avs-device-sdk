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

#ifndef ACSDKASSETSINTERFACES_VENDABLEARTIFACT_H_
#define ACSDKASSETSINTERFACES_VENDABLEARTIFACT_H_

#include <chrono>

#include "DavsRequest.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace commonInterfaces {

class VendableArtifact {
public:
    using TimeEpoch = std::chrono::system_clock::time_point;

    /**
     * Creates a Vendable Artifact containing all the information about a given artifact and how to get it.
     *
     * @param request REQUIRED, contains the original request that's parsed by DAVS.
     * @param id REQUIRED, uniquely identifies an artifact (hash of the file itself).
     * @param artifactSizeBytes REQUIRED, size of the artifact that is being downloaded.
     * @param artifactExpiry REQUIRED, epoch when the artifact should be checked again for update.
     * @param s3Url REQUIRED, signed url where we can download the artifact.
     * @param urlExpiry REQUIRED, epoch when the url for the artifact download will expire.
     * @param currentSizeBytes OPTIONAL? TODO: still not sure what this is...
     * @param multipart is the vendable artifact constructed by a multipart response
     * @return NULLABLE, a smart pointer to Vendable Artifact if all parameters are valid.
     */
    static std::unique_ptr<VendableArtifact> create(
            std::shared_ptr<DavsRequest> request,
            std::string id,
            size_t artifactSizeBytes,
            TimeEpoch artifactExpiry,
            std::string s3Url,
            TimeEpoch urlExpiry,
            size_t currentSizeBytes,
            bool multipart);

    /**
     * Creates a Vendable Artifact from JSON string.
     *
     * @param request REQUIRED, contains the original request that's parsed by DAVS.
     * @param jsonString REQUIRED, contains the JSON string to parse and read values from.
     * @param isMultipart Optional, defaults to false. If the artifact is a multipart artifact.
     * @return NULLABLE, a smart pointer to Vendable Artifact if given JSON is valid, otherwise nullptr.
     */
    static std::unique_ptr<VendableArtifact> create(
            std::shared_ptr<DavsRequest> request,
            const std::string& jsonString,
            const bool isMultipart = false);

    inline const std::shared_ptr<DavsRequest>& getRequest() const {
        return m_request;
    }

    inline const std::string& getId() const {
        return m_id;
    }

    inline const std::string& getS3Url() const {
        return m_s3Url;
    }

    inline size_t getArtifactSizeBytes() const {
        return m_artifactSizeBytes;
    }

    inline const TimeEpoch& getArtifactExpiry() const {
        return m_artifactExpiry;
    }

    inline const TimeEpoch& getUrlExpiry() const {
        return m_urlExpiry;
    }

    inline size_t getCurrentSizeBytes() const {
        return m_currentSizeBytes;
    }

    inline const std::string& getUniqueIdentifier() const {
        return m_uuid;
    }

    inline bool isMultipart() const {
        return m_multipart;
    }

private:
    VendableArtifact(
            std::shared_ptr<DavsRequest> request,
            std::string id,
            size_t artifactSizeBytes,
            TimeEpoch artifactExpiry,
            std::string s3Url,
            TimeEpoch urlExpiry,
            size_t currentSizeBytes,
            std::string uuid,
            bool multipart);

private:
    const std::shared_ptr<DavsRequest> m_request;
    const std::string m_id;
    const size_t m_artifactSizeBytes;
    const TimeEpoch m_artifactExpiry;
    const std::string m_s3Url;
    const TimeEpoch m_urlExpiry;
    const size_t m_currentSizeBytes;
    const std::string m_uuid;
    const bool m_multipart;
};

}  // namespace commonInterfaces
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKASSETSINTERFACES_VENDABLEARTIFACT_H_
