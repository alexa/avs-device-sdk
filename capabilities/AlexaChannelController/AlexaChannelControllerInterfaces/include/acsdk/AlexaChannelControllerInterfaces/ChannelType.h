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
#ifndef ACSDK_ALEXACHANNELCONTROLLERINTERFACES_CHANNELTYPE_H_
#define ACSDK_ALEXACHANNELCONTROLLERINTERFACES_CHANNELTYPE_H_

#include <memory>
#include <string>

#include <AVSCommon/Utils/Logger/LoggerUtils.h>
#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace alexaChannelControllerTypes {

/**
 * A channel type that represents the identifying data for a channel.
 *
 */
class Channel {
public:
    /**
     * Creates a new Channel object.
     *
     * @param number The number for the channel.
     * @param callSign The call sign for the channel.
     * @param affiliateCallSign The affiliate call sign for the channel.
     * @param uri The URI of the channel
     * @param name The name of the channel
     * @param imageURL The URL for image or logo for the channel
     * @note For a channel object to be valid, it must contain at least one of number, callSign,
     * affiliateCallSign, uri, or name fields.
     * @return @c nullptr if the object is invalid, else a new instance of @c Channel
     */
    static std::unique_ptr<Channel> Create(
        std::string number,
        std::string callSign,
        std::string affiliateCallSign,
        std::string uri,
        std::string name,
        std::string imageURL) {
        if (number.empty() && callSign.empty() && affiliateCallSign.empty() && uri.empty() && name.empty()) {
            avsCommon::utils::logger::acsdkError(
                avsCommon::utils::logger::LogEntry("Channel", "CreateFailed").d("reason", "InvalidParams"));
            return nullptr;
        }
        return std::unique_ptr<Channel>(new Channel(
            std::move(number),
            std::move(callSign),
            std::move(affiliateCallSign),
            std::move(uri),
            std::move(name),
            std::move(imageURL)));
    }

    /*
     * Get the number for the channel.
     *
     * @return empty string if the number is empty, otherwise the channel number.
     */
    std::string getNumber() const {
        return number.value();
    }

    /*
     * Get the call sign for the channel.
     *
     * @return empty string if the call sign is empty, otherwise the channel call sign.
     */
    std::string getCallSign() const {
        return callSign.value();
    }

    /*
     * Get the affiliate call sign for the channel.
     *
     * @return empty string if the affiliate call sign is empty, otherwise the channel affiliate call sign.
     */
    std::string getAffiliateCallSign() const {
        return affiliateCallSign.value();
    }

    /*
     * Get the URI for the channel.
     *
     * @return empty string if the URI is empty, otherwise the channel URI.
     */
    std::string getURI() const {
        return uri.value();
    }

    /*
     * Get the name for the channel.
     *
     * @return empty string if the name is empty, otherwise the channel name.
     */
    std::string getName() const {
        return name.value();
    }

    /*
     * Get the image or logo URL for the channel.
     *
     * @return empty string if the image or logo URL is empty, otherwise the image URL.
     */
    std::string getImageURL() const {
        return imageURL.value();
    }

private:
    /**
     * Constructor.
     *
     * @param number The number for the channel.
     * @param callSign The call sign for the channel.
     * @param affiliateCallSign The affiliate call sign for the channel.
     * @param uri The URI of the channel
     * @param name The name of the channel
     * @param imageURL The URL for image or logo for the channel
     */
    Channel(
        std::string number,
        std::string callSign,
        std::string affiliateCallSign,
        std::string uri,
        std::string name,
        std::string imageURL) :
            number{std::move(number)},
            callSign{std::move(callSign)},
            affiliateCallSign{std::move(affiliateCallSign)},
            uri{std::move(uri)},
            name{std::move(name)},
            imageURL{std::move(imageURL)} {
    }

    /// The number for the channel, such as "256" or "13.1"
    avsCommon::utils::Optional<std::string> number;
    /// The call sign for the channel.
    avsCommon::utils::Optional<std::string> callSign;
    /// The affiliate call sign for the channel, typically associated with a local broadcaster for that channel.
    avsCommon::utils::Optional<std::string> affiliateCallSign;
    /// The URI of the channel, which identifies content, such as
    /// entity://provider/channel/<number>.
    avsCommon::utils::Optional<std::string> uri;
    /// The name for the channel.
    avsCommon::utils::Optional<std::string> name;
    /// The URL of an image or logo for the channel.
    avsCommon::utils::Optional<std::string> imageURL;
};

}  // namespace alexaChannelControllerTypes
}  // namespace alexaClientSDK
#endif  // ACSDK_ALEXACHANNELCONTROLLERINTERFACES_CHANNELTYPE_H_
