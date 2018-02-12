/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_TEST_ATTACHMENT_COMMON_COMMON_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_TEST_ATTACHMENT_COMMON_COMMON_H_

#include <memory>
#include <vector>

#include "AVSCommon/Utils/SDS/InProcessSDS.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace test {

/// A test AttachmentId string.
static const std::string TEST_ATTACHMENT_ID_STRING_ONE = "testAttachmentId_1";
/// A second test AttachmentId string.
static const std::string TEST_ATTACHMENT_ID_STRING_TWO = "testAttachmentId_2";
/// A third test AttachmentId string.
static const std::string TEST_ATTACHMENT_ID_STRING_THREE = "testAttachmentId_3";
/// A test buffer size.
static const int TEST_SDS_BUFFER_SIZE_IN_BYTES = 400;
/// A test buffer write size.
static const int TEST_SDS_PARTIAL_WRITE_AMOUNT_IN_BYTES = 150;
/// A test buffer read size.
static const int TEST_SDS_PARTIAL_READ_AMOUNT_IN_BYTES = 150;

/**
 * A utility function to create an SDS.
 *
 * @param size The desired size of the data segment of the SDS.
 * @return An SDS for testing.
 */
std::unique_ptr<avsCommon::utils::sds::InProcessSDS> createSDS(int size);

/**
 * A utility function to create a random pattern.
 *
 * @param size The size of the random pattern desired.
 * @return A vector containing a random pattern of data.
 */
std::vector<uint8_t> createTestPattern(int size);

}  // namespace test
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_TEST_ATTACHMENT_COMMON_COMMON_H_
