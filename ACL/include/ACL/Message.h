/*
* Message.h
*
* Copyright 2016-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_MESSAGE_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_MESSAGE_H_

#include <istream>
#include <memory>
#include <string>
#include <AVSCommon/AttachmentManager.h>

namespace alexaClientSDK {
namespace acl {

/**
 * This class encapsulates a JSON string expressing content, and an optional istream which refers to binary data.
 */
class Message {
public:
    /**
     * Constructor.
     * Construct a message with the JSON content and the binary attachment. This constructor is used in order to
     * send an event to the AVS. The @c binaryContent is not required. When sending the "Recognize" event for example,
     * the @c binaryContent should be the recorded audio data.
     *
     * @param json The JSON content of the message.
     * @param binaryContent The binary attachment of the message.
     */
    Message(const std::string& json, std::shared_ptr<std::istream> binaryContent = nullptr);

    /**
     * Constuctor.
     *
     * @param json the JSON content of message.
     * @param attachmentManager attachment manager.
     */
    Message(const std::string& json, std::shared_ptr<avsCommon::AttachmentManagerInterface> attachmentManager);

    /**
     * Retrieve the JSON content.
     * If this Message represents an AVS Directive or Exception, then clients should parse it as per the specified
     * AVS interface.
     * If this Message represents an Attachment, this JSON content will contain one field 'cid' which represents
     * the Content-ID of the attachment binary content, for example:
     * { 'cid':'12345' }
     *
     * @return The JSON string.
     */
    std::string getJSONContent() const;

    /**
     * Retrieves the stream object representing the binary content.
     *
     * @return The stream object representing the binary content.
     */
    std::shared_ptr<std::istream> getAttachment();

    /**
     * Retrieves the attachment manager.
     *
     * @return instance that implements the attachment manager interface.
     */
    std::shared_ptr<avsCommon::AttachmentManagerInterface> getAttachmentManager() const;

private:
    /// The JSON content.
    std::string m_jsonContent;

    /// The stream object representing the binary content.
    std::shared_ptr<std::istream> m_binaryContent;

    /// The attachment manager that creates attachment reader and attachment writer.
    std::shared_ptr<avsCommon::AttachmentManagerInterface> m_attachmentManager;
};

} // namespace acl
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_MESSAGE_H_
