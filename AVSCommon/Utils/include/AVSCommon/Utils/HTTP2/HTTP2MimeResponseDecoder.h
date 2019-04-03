/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP2_HTTP2MIMERESPONSEDECODER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP2_HTTP2MIMERESPONSEDECODER_H_

#include <memory>

#include <MultipartParser/MultipartReader.h>

#include "AVSCommon/Utils/HTTP2/HTTP2MimeResponseSinkInterface.h"
#include "AVSCommon/Utils/HTTP2/HTTP2ResponseSinkInterface.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace http2 {

/**
 * Class that adapts between HTTPResponseSinkInterface and HTTP2MimeResponseSinkInterface providing
 * mime decoding services.
 */
class HTTP2MimeResponseDecoder : public HTTP2ResponseSinkInterface {
public:
    /**
     * Constructor.
     *
     * @param sink Pointer to the object to receive the mime parts.
     */
    HTTP2MimeResponseDecoder(std::shared_ptr<HTTP2MimeResponseSinkInterface> sink);

    /**
     * Destructor.
     */
    ~HTTP2MimeResponseDecoder() = default;

    /// @name HTTP2ResponseSinkInterface methods.
    /// @{
    bool onReceiveResponseCode(long responseCode) override;
    bool onReceiveHeaderLine(const std::string& line) override;
    HTTP2ReceiveDataStatus onReceiveData(const char* bytes, size_t size) override;
    void onResponseFinished(HTTP2ResponseFinishedStatus status) override;
    /// @}

private:
    /** We will also need to implements the callbacks for @class MultipartReader **/
    static void partBeginCallback(const MultipartHeaders& headers, void* userData);
    static void partDataCallback(const char* buffer, size_t size, void* userData);
    static void partEndCallback(void* userData);

    /// MIMEResponseSinkInterface implementation to pass MIME data to
    std::shared_ptr<HTTP2MimeResponseSinkInterface> m_sink;
    /// Response code that has been received, or zero.
    long m_responseCode;
    /// Instance of a multipart MIME reader.
    MultipartReader m_multipartReader;
    /// Last parse status returned
    HTTP2ReceiveDataStatus m_lastStatus;
    /// MIME part callbacks on current(or last in case of PAUSE) chunk
    size_t m_index;
    /// Number of characters left to check at the beginning of the stream for a leading CRLF sequence.
    size_t m_leadingCRLFCharsLeftToRemove;
    /// needed to track if boundary has been set
    bool m_boundaryFound;
    /// last successful MIME part callback on current chunk
    size_t m_lastSuccessIndex;
};

}  // namespace http2
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP2_HTTP2MIMERESPONSEDECODER_H_
