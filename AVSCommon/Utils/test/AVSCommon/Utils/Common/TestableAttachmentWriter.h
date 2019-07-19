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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_COMMON_TESTABLEATTACHMENTWRITER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_COMMON_TESTABLEATTACHMENTWRITER_H_

#include <AVSCommon/AVS/Attachment/InProcessAttachmentWriter.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {

/**
 * A version of the Decorator Pattern, this class allows us to simulate pausing writes without requiring
 * an actual (slow) AttachmentReader anywhere in the test code.  Besides this small change in functionality, all real
 * work is done by the encapsulated InProcessAttachmentWriter object.
 */
class TestableAttachmentWriter : public avsCommon::avs::attachment::InProcessAttachmentWriter {
public:
    /**
     * Constructor.
     *
     * @param dummySDS An SDS used to instantiate this class, although it will never be used.  This is to avert any
     * risk of this wrapper object being created with a nullptr.
     * @param writer The AttachmentWriter object to be wrapped by this class.
     */
    TestableAttachmentWriter(
        std::shared_ptr<avsCommon::utils::sds::InProcessSDS> dummySDS,
        std::unique_ptr<avsCommon::avs::attachment::AttachmentWriter> writer);

    std::size_t write(
        const void* buf,
        std::size_t numBytes,
        WriteStatus* writeStatus,
        std::chrono::milliseconds timeout) override;

    void close() override;

private:
    /// The AttachmentWriter object to be wrapped by this class.
    std::unique_ptr<avsCommon::avs::attachment::AttachmentWriter> m_writer;

    bool m_hasWriteBeenInvoked;
};

}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_COMMON_TESTABLEATTACHMENTWRITER_H_
