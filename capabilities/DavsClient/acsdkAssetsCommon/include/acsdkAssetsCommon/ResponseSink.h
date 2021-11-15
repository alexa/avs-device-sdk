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

#ifndef ACSDKASSETSCOMMON_RESPONSESINK_H_
#define ACSDKASSETSCOMMON_RESPONSESINK_H_
#include <fstream>
#include <memory>

#include "DownloadChunkQueue.h"
#include "DownloadStream.h"
#include "acsdkAssetsInterfaces/DavsRequest.h"
#include "acsdkAssetsInterfaces/VendableArtifact.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace common {

/**
 * Handle Multipart DAVS Responses.
 *
 */
class ResponseSink {
public:
    /**
     * Constructor.
     *
     */
    ResponseSink(const std::shared_ptr<commonInterfaces::DavsRequest>& request, const std::string& workingDirectory);

    /**
     * Destructor.
     */
    virtual ~ResponseSink() = default;

    /**
     * get m_artifact
     * @return NULLABLE, returns m_artifact
     */
    std::shared_ptr<commonInterfaces::VendableArtifact> getArtifact();

    /**
     * get the full path to the artifact if it's downloaded properly
     * @return path to the artifact, empty if not downloaded or error
     */
    std::string getArtifactPath();

    /**
     * When we receive a header line search for boundary. If we find
     * the boundary set m_boundary and notify waiting threads.
     * @param header the header line which we search for the boundary.
     * @return
     */
    bool onHeader(const std::string& header);

    /**
     * Wrapper around the multipart Parser, used to set the parsers callbacks and
     * feed it data.
     * @param downloadChunkQueue handles streaming data chunk by chunk.
     * @return true if parsing of response succeeds
     */
    bool parser(std::shared_ptr<DownloadChunkQueue>& downloadChunkQueue);

private:
    /**
     * Sets the content type member variable
     * @param type the current content type.
     */
    void setContentType(const std::string& type);
    /**
     * Either appends value to m_jsonString if the content type is JSON or writes the
     * buffer to the DownloadStream if the content type is ATTACHMENT
     * @param buffer the data to be written
     * @param size the size of the data to be written.
     */
    void setData(const char* buffer, size_t size);
    /**
     * The function that is called at the end of receiving data. Will create an
     * artifact at the end of receiving json information.
     */
    void endData();

private:
    /// Types of mime parts
    enum class ContentType {
        /// The default value, indicating no data.
        NONE,
        /// The content represents a JSON formatted string.
        JSON,
        /// The content represents binary data.
        ATTACHMENT
    };

    /// Type of content in the current part.
    ContentType m_contentType;

    /// The json string in the multipart response.
    std::string m_jsonString;

    /// The download stream where the attachment will be written.
    std::shared_ptr<DownloadStream> m_attachment;

    /// The parent directory where the attachment will be written.
    const std::string m_parentDirectory;

    /// The full path to the artifact once it is downloaded
    std::string m_artifactPath;

    /// The current DAVs request, will be used to create the artifact.
    std::shared_ptr<commonInterfaces::DavsRequest> m_request;

    /// The artifact that is created from the response.
    std::shared_ptr<commonInterfaces::VendableArtifact> m_artifact;

    /// The boundary for the multipart response.
    std::string m_boundary;
};

}  // namespace common
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKASSETSCOMMON_RESPONSESINK_H_
