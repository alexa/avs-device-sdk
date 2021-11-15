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

#ifndef ACSDKASSETSINTERFACES_URLREQUEST_H_
#define ACSDKASSETSINTERFACES_URLREQUEST_H_

#include "ArtifactRequest.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace commonInterfaces {

class UrlRequest : public ArtifactRequest {
public:
    /**
     * Creates an Artifact Request that will contain all the necessary info to download a file using a url.
     *
     * @param url REQUIRED, url used to download the desired artifact.
     * @param filename REQUIRED, name of the resource to be stored on the device.
     * @param unpack OPTIONAL, if true, then artifact will be unpacked and the directory will be provided.
     * @param certPath OPTIONAL, if not-emtpy, then the download request will be made with the SSL cert at the cerPath.
     * @return NULLABLE, a smart pointer to a request if all params are valid.
     */
    static std::shared_ptr<UrlRequest> create(
            std::string url,
            std::string filename,
            bool unpack = false,
            std::string certPath = "");

    /**
     * @return the URL used to download the requested file.
     */
    const std::string& getUrl() const;

    /**
     * @return the filename to be used for this artifact in case we cannot fetch the name from HTTP headers.
     */
    const std::string& getFilename() const;

    /**
     * @return the optional filepath to the SSL Certificate which can be used for this request.
     */
    const std::string& getCertPath() const;

    /// @name { ArtifactRequest methods.
    /// @{
    Type getRequestType() const override;
    bool needsUnpacking() const override;
    std::string getSummary() const override;
    std::string toJsonString() const override;
    /// @}

private:
    UrlRequest(std::string url, std::string filename, bool unpack, std::string certPath);

private:
    const std::string m_url;
    const std::string m_filename;
    const bool m_unpack;
    std::string m_summary;
    const std::string m_certPath;
    std::string m_certHash;
};

inline const std::string& UrlRequest::getUrl() const {
    return m_url;
}

inline const std::string& UrlRequest::getFilename() const {
    return m_filename;
}

inline const std::string& UrlRequest::getCertPath() const {
    return m_certPath;
}

inline Type UrlRequest::getRequestType() const {
    return Type::URL;
}

inline bool UrlRequest::needsUnpacking() const {
    return m_unpack;
}

inline std::string UrlRequest::getSummary() const {
    return m_summary;
}

}  // namespace commonInterfaces
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKASSETSINTERFACES_URLREQUEST_H_
