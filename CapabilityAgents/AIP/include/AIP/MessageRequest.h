/**
 * AudioInputProcessor.cpp
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <AVSCommon/AVS/MessageRequest.h>

#include "AIP/AudioInputProcessor.h"

namespace alexaClientSDK {
namespace capabilityAgent {
namespace aip {

class MessageRequest : public avsCommon::avs::MessageRequest {
public:
    /**
     * @copyDoc avsCommon::avs::MessageRequest()
     *
     * @param audioInputProcessor The AudioInputProcessor to notify when the avsCommon::avs::MessageRequest encounters
     *     an exception.
     */
    MessageRequest(
            std::shared_ptr<AudioInputProcessor> audioInputProcessor,
            const std::string& jsonContent,
            std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader);

    void onExceptionReceived(const std::string& exceptionMessage) override;

private:
    std::shared_ptr<AudioInputProcessor> m_audioInputProcessor;
};

} // namespace aip
} // namespace capabilityagent
} // namespace alexaClientSDK
