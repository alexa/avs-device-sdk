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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_AVSMESSAGE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_AVSMESSAGE_H_

#include "AVSMessageHeader.h"

#include <memory>
#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

/**
 * This is a base class which allows us to represent a message sent or received from AVS.
 * This class encapsulates the common data elements for all such messages.
 */
class AVSMessage {
public:
    /**
     * Constructor.
     *
     * @param avsMessageHeader An object that contains the necessary header fields of an AVS message.
     *                         NOTE: This parameter MUST NOT be null.
     * @param payload The payload associated with an AVS message. This is expected to be in the JSON format.
     */
    AVSMessage(std::shared_ptr<AVSMessageHeader> avsMessageHeader, std::string payload);

    /**
     * Destructor.
     */
    virtual ~AVSMessage() = default;

    /**
     * Returns The namespace of the message.
     *
     * @return The namespace.
     */
    std::string getNamespace() const;

    /**
     * Returns The name of the message, which describes the intent.
     *
     * @return The name.
     */
    std::string getName() const;

    /**
     * Returns The message ID of the message.
     *
     * @return The message ID, a unique ID used to identify a specific message.
     */
    std::string getMessageId() const;

    /**
     * Returns The dialog request ID of the message.
     *
     * @return The dialog request ID, a unique ID for the messages that are part of the same dialog.
     */
    std::string getDialogRequestId() const;

    /**
     * Returns the payload of the message.
     *
     * @return The payload.
     */
    std::string getPayload() const;

    /**
     * Return a string representation of this @c AVSMessage's header.
     *
     * @return A string representation of this @c AVSMessage's header.
     */
    std::string getHeaderAsString() const;

private:
    /// The fields that represent the common items in the header of an AVS message.
    std::shared_ptr<AVSMessageHeader> m_header;
    /// The payload of an AVS message.
    std::string m_payload;
};

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_AVSMESSAGE_H_
