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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_AVSMESSAGEHEADER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_AVSMESSAGEHEADER_H_

#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

/**
 * The AVS message header, which contains the common fields required for an AVS message.
 */
class AVSMessageHeader {
public:
    /**
     * Constructor.
     *
     * @param avsNamespace The namespace of an AVS message.
     * @param avsName The name within the namespace of an AVS message.
     * @param avsMessageId The message ID of an AVS message.
     * @param avsDialogRequestId The dialog request ID of an AVS message, which is optional.
     */
    AVSMessageHeader(
        const std::string& avsNamespace,
        const std::string& avsName,
        const std::string& avsMessageId,
        const std::string& avsDialogRequestId = "") :
            m_namespace{avsNamespace},
            m_name{avsName},
            m_messageId{avsMessageId},
            m_dialogRequestId{avsDialogRequestId} {
    }

    /**
     * Returns the namespace in an AVS message.
     *
     * @return The namespace.
     */
    std::string getNamespace() const;

    /**
     * Returns the name in an AVS message, which describes the intent of the message.
     *
     * @return The name.
     */
    std::string getName() const;

    /**
     * Returns the message ID in an AVS message.
     *
     * @return The message ID, a unique ID used to identify a specific message.
     */
    std::string getMessageId() const;

    /**
     * Returns the dialog request ID in an AVS message.
     *
     * @return The dialog request ID, a unique ID for the messages that are part of the same dialog.
     */
    std::string getDialogRequestId() const;

    /**
     * Return a string representation of this @c AVSMessage's header.
     *
     * @return A string representation of this @c AVSMessage's header.
     */
    std::string getAsString() const;

private:
    /// Namespace of the AVSMessage header.
    const std::string m_namespace;
    /// Name within the namespace of the AVSMessage, which describes the intent of the message.
    const std::string m_name;
    /// A unique ID used to identify a specific message.
    const std::string m_messageId;
    /// A unique ID for the messages that are part of the same dialog.
    const std::string m_dialogRequestId;
};

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_AVSMESSAGEHEADER_H_
