/*
 * TestableAttachmentWriter.cpp
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "TestableAttachmentWriter.h"
#include "Common.h"

namespace alexaClientSDK {
namespace acl {
namespace test {

using namespace avsCommon::avs::attachment;
using namespace avsCommon::utils::sds;

TestableAttachmentWriter::TestableAttachmentWriter(
    std::shared_ptr<InProcessSDS> dummySDS,
    std::unique_ptr<AttachmentWriter> writer) :
        InProcessAttachmentWriter{dummySDS},
        m_writer{std::move(writer)},
        m_hasWriteBeenInvoked{false} {
}

std::size_t TestableAttachmentWriter::write(
    const void* buf,
    std::size_t numBytes,
    WriteStatus* writeStatus,
    std::chrono::milliseconds timeout) {
    bool simulatePause = false;

    if (!m_hasWriteBeenInvoked) {
        simulatePause = true;
        m_hasWriteBeenInvoked = true;
    } else {
        // Let's throw a random number, and simulate a pause if it's 1!
        simulatePause = (generateRandomNumber(0, 1) == 1);
    }

    if (simulatePause) {
        *writeStatus = WriteStatus::OK_BUFFER_FULL;
        return 0;
    }

    // Otherwise, let's have the encapsulated writer do the actual work.
    return m_writer->write(buf, numBytes, writeStatus);
}

void TestableAttachmentWriter::close() {
    m_writer->close();
}

}  // namespace test
}  // namespace acl
}  // namespace alexaClientSDK
