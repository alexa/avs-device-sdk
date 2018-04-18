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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ATTACHMENT_ATTACHMENTUTILS_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ATTACHMENT_ATTACHMENTUTILS_H_

#include <memory>
#include <vector>

#include "AttachmentReader.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace attachment {

/**
 * A collection of util functions related to Attachment classes.
 */
class AttachmentUtils {
public:
    /**
     * Create an @c AttachmentReader with a preset content of a @c std::vector of @c char.
     *
     * @param srcBuffer The reader content.
     * @return A new @c AttachmentReader or @c nullptr if the operation failed.
     */
    static std::unique_ptr<AttachmentReader> createAttachmentReader(const std::vector<char>& srcBuffer);
};

}  // namespace attachment
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ATTACHMENT_ATTACHMENTUTILS_H_
