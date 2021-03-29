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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_EDITABLEMESSAGEREQUEST_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_EDITABLEMESSAGEREQUEST_H_

#include <AVSCommon/AVS/MessageRequest.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

/**
 * A specialized MessageRequest in which data fields are editable after creation.
 */
class EditableMessageRequest : public avsCommon::avs::MessageRequest {
public:
    /**
     * Constructor to construct an @c EditableMessageRequest which contains a copy of the data in @c MessageRequest.
     * @param messageRequest MessageRequest to copy from
     * @note Observers are not considered data and don't get copied by this constructor.
     */
    EditableMessageRequest(const MessageRequest& messageRequest);

    /**
     * Set json content of the message
     * @param json Json content
     */
    void setJsonContent(const std::string& json);

    /**
     * Set attachment readers of attachment data to be sent to AVS, which will replace the existing readers if there is
     * any, and invalid attachment readers will be ignored.
     * @param attachmentReaders Vector of named readers
     */
    void setAttachmentReaders(const std::vector<std::shared_ptr<NamedReader>>& attachmentReaders);

    /**
     * Set MessageRequest resolve function.
     * @param resolver Resolve function to set
     */
    void setMessageRequestResolveFunction(const MessageRequest::MessageRequestResolveFunction& resolver);
};

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_EDITABLEMESSAGEREQUEST_H_
