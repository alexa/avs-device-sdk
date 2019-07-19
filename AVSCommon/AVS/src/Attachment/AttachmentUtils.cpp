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

#include <cstdlib>

#include <AVSCommon/AVS/Attachment/InProcessAttachmentReader.h>
#include "AVSCommon/AVS/Attachment/AttachmentUtils.h"
#include <AVSCommon/Utils/SDS/InProcessSDS.h>
#include "AVSCommon/Utils/Logger/Logger.h"

using namespace alexaClientSDK::avsCommon::utils::sds;

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace attachment {

/// String to identify log entries originating from this file.
static const std::string TAG("AttachmentUtils");

/// Maximum size of the reader is 4KB.
static const std::size_t MAX_READER_SIZE = 4 * 1024;

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::unique_ptr<AttachmentReader> AttachmentUtils::createAttachmentReader(const std::vector<char>& srcBuffer) {
    auto bufferSize = InProcessSDS::calculateBufferSize(srcBuffer.size());
    auto buffer = std::make_shared<InProcessSDS::Buffer>(bufferSize);
    auto stream = InProcessSDS::create(buffer);

    if (!stream) {
        ACSDK_ERROR(LX("createAttachmentReaderFailed").d("reason", "Failed to create stream"));
        return nullptr;
    }

    auto writer = stream->createWriter(InProcessSDS::Writer::Policy::BLOCKING);
    if (!writer) {
        ACSDK_ERROR(LX("createAttachmentReaderFailed").d("reason", "Failed to create writer"));
        return nullptr;
    }

    if (srcBuffer.size() > MAX_READER_SIZE) {
        ACSDK_WARN(LX("createAttachmentReaderExceptionSize").d("size", srcBuffer.size()));
    }

    auto numWritten = writer->write(srcBuffer.data(), srcBuffer.size());
    if (numWritten < 0 || static_cast<size_t>(numWritten) != srcBuffer.size()) {
        ACSDK_ERROR(LX("createAttachmentReaderFailed")
                        .d("reason", "writing failed")
                        .d("buffer size", srcBuffer.size())
                        .d("numWritten", numWritten));
        return nullptr;
    }

    auto attachmentReader =
        InProcessAttachmentReader::create(InProcessSDS::Reader::Policy::BLOCKING, std::move(stream));

    return std::move(attachmentReader);
}
}  // namespace attachment
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
