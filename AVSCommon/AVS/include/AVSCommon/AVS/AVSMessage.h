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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_AVSMESSAGE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_AVSMESSAGE_H_

#include <memory>
#include <string>

#include <AVSCommon/Utils/Optional.h>

#include "AVSCommon/AVS/AVSMessageEndpoint.h"
#include "AVSCommon/AVS/AVSMessageHeader.h"

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
     * @param endpoint The attributes for the target endpoint if available.
     */
    AVSMessage(
        std::shared_ptr<AVSMessageHeader> avsMessageHeader,
        std::string payload,
        const utils::Optional<AVSMessageEndpoint>& endpoint = utils::Optional<AVSMessageEndpoint>());

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
     * Returns the correlation token of the message.
     *
     * @return The correlation token string.
     */
    std::string getCorrelationToken() const;

    /**
     * Returns the event correlation token of the message.
     *
     * @return The event correlation token string.
     */
    std::string getEventCorrelationToken() const;

    /**
     * Return the payload version in an AVS message.
     *
     * @return The payload version if present or an empty string if this field is not available.
     */
    std::string getPayloadVersion() const;

    /**
     * Return the instance in an AVS message.
     *
     * @return The target instance id if present or an empty string if this field is not available.
     */
    std::string getInstance() const;

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
     * Return a pointer to its header.
     *
     * @return A pointer to this @c AVSMessage's header.
     */
    std::shared_ptr<const AVSMessageHeader> getHeader() const;

    /**
     * Return a string representation of this @c AVSMessage's header.
     *
     * @return A string representation of this @c AVSMessage's header.
     */
    std::string getHeaderAsString() const;

    /**
     * Return the endpoint attributes included in this message.
     *
     * @return The endpoint attributes if present; otherwise an empty object.
     */
    utils::Optional<AVSMessageEndpoint> getEndpoint() const;

private:
    /// The fields that represent the common items in the header of an AVS message.
    const std::shared_ptr<AVSMessageHeader> m_header;

    /// The payload of an AVS message.
    const std::string m_payload;

    /// The endpoint attributes of this message.
    const utils::Optional<AVSMessageEndpoint> m_endpoint;
};

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_AVSMESSAGE_H_
