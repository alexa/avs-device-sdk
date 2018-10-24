/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCKMIMERESPONSESINK_H_
#define ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCKMIMERESPONSESINK_H_

#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <AVSCommon/Utils/HTTP2/HTTP2MimeResponseSinkInterface.h>

namespace alexaClientSDK {
namespace acl {
namespace test {

/**
 * A mock class of @c HTTP2MimeResponseSinkInterface that stores the contents of parsed of a MIME message.
 */
class MockMimeResponseSink : public avsCommon::utils::http2::HTTP2MimeResponseSinkInterface {
public:
    /**
     * Constructor.
     */
    MockMimeResponseSink();

    /**
     * Destructor.
     */
    virtual ~MockMimeResponseSink() = default;

    /// @name HTTP2MimeResponseSinkInterface methods
    /// @{
    bool onReceiveResponseCode(long responseCode) override;
    bool onReceiveHeaderLine(const std::string& line) override;
    bool onBeginMimePart(const std::multimap<std::string, std::string>& headers) override;
    avsCommon::utils::http2::HTTP2ReceiveDataStatus onReceiveMimeData(const char* bytes, size_t size) override;
    bool onEndMimePart() override;
    avsCommon::utils::http2::HTTP2ReceiveDataStatus onReceiveNonMimeData(const char* bytes, size_t size) override;
    void onResponseFinished(avsCommon::utils::http2::HTTP2ResponseFinishedStatus status) override;
    /// @}

    /**
     * Get the contents of a MIME part.
     *
     * @param part The index of MIME part starting from 0.
     * @return A buffer that contains the MIME part.
     */
    std::vector<char> getMimePart(unsigned part);

    /**
     * Get the total number of MIME parts parsed.
     *
     * @return The number of MIME parts parsed.
     */
    unsigned getCountOfMimeParts();

private:
    /// A buffer that contains of all the contents of the MIME parts.
    std::vector<std::vector<char>> m_mimeContents;

    /// A buffer that contains the current MIME part being parsed.
    std::vector<char> m_mimeCurrentContent;
};

}  // namespace test
}  // namespace acl
}  // namespace alexaClientSDK

#endif /* ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCKMIMERESPONSESINK_H_ */
