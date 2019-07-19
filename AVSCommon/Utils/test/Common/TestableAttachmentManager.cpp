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

#include "AVSCommon/Utils/Common/TestableAttachmentManager.h"
#include "AVSCommon/Utils/Common/TestableAttachmentWriter.h"

#include "AVSCommon/Utils/SDS/InProcessSDS.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {

/// A small value so we can create objects with a valid SDS (even if unused).
static const int DUMMY_SDS_BUFFER_SIZE = 100;

using namespace avsCommon::avs::attachment;
using namespace avsCommon::utils::sds;

using SDSType = avsCommon::utils::sds::InProcessSDS;
using SDSBufferType = avsCommon::utils::sds::InProcessSDSTraits::Buffer;

TestableAttachmentManager::TestableAttachmentManager() :
        AttachmentManager{AttachmentManager::AttachmentType::IN_PROCESS},
        m_manager{
            std::unique_ptr<AttachmentManager>(new AttachmentManager(AttachmentManager::AttachmentType::IN_PROCESS))} {
}

std::string TestableAttachmentManager::generateAttachmentId(const std::string& contextId, const std::string& contentId)
    const {
    return m_manager->generateAttachmentId(contextId, contentId);
}

bool TestableAttachmentManager::setAttachmentTimeoutMinutes(std::chrono::minutes timeoutMinutes) {
    return m_manager->setAttachmentTimeoutMinutes(timeoutMinutes);
}

std::unique_ptr<AttachmentWriter> TestableAttachmentManager::createWriter(
    const std::string& attachmentId,
    WriterPolicy policy) {
    // First, let's create a dummy SDS.  Otherwise we need to intantiate the writer with nullptr, which is
    // probably not a good idea.
    auto buffSize = SDSType::calculateBufferSize(DUMMY_SDS_BUFFER_SIZE);
    auto buff = std::make_shared<SDSBufferType>(buffSize);
    std::shared_ptr<SDSType> dummySDS = SDSType::create(buff);

    // Create the writer we will encapsulate in a TestableAttachmentWriter.
    auto writer = m_manager->createWriter(attachmentId);
    // Now create the wrapper class.
    std::unique_ptr<AttachmentWriter> testableWriter =
        std::unique_ptr<TestableAttachmentWriter>(new TestableAttachmentWriter(dummySDS, std::move(writer)));

    // Done!
    return testableWriter;
}

std::unique_ptr<AttachmentReader> TestableAttachmentManager::createReader(
    const std::string& attachmentId,
    ReaderPolicy policy) {
    return m_manager->createReader(attachmentId, policy);
}

}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
