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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_AVSDISCOVERYENDPOINTATTRIBUTES_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_AVSDISCOVERYENDPOINTATTRIBUTES_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <AVSCommon/SDKInterfaces/Endpoints/EndpointIdentifier.h>
#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

/**
 * The structure representing the endpoint attributes used for discovery.
 *
 * This structure mirrors the AVS definition which is documented here:
 * https://developer.amazon.com/docs/alexa/alexa-voice-service/alexa-discovery.html
 *
 * @note The following attributes will differ from the default endpoint, used to describe this Alexa client, to any
 * other endpoint controlled by this client. The differences are:
 *   - Registration field is only used in the default endpoint.
 *   - Friendly name should not be provided for the default endpoint, but it is require for all the other endpoints.
 */
struct AVSDiscoveryEndpointAttributes {
    /**
     * The registration structure to identify the default endpoint in AVS.
     */
    struct Registration {
        /// The productID of the device provided when registering the client on https://developer.amazon.com.
        std::string productId;

        /// The device serialNumber.
        std::string serialNumber;

        /// The device registration key value.
        std::string registrationKey;

        /// The device product id key value.
        std::string productIdKey;

        /**
         * Constructor.
         *
         * @param productId The product identifier for the Alexa client device.
         * @param serialNumber The device DSN.
         * @param registrationKey the registration key value.
         * @param productIdKey the product id key value.
         */
        Registration(
            const std::string& productId,
            const std::string& serialNumber,
            const std::string& registrationKey,
            const std::string& productIdKey);

        /**
         * Default constructor.
         */
        Registration() = default;

        /**
         * Operator == for @c Registration.
         * @param rhs The object to compare to @c this.
         * @return Whether the operation holds.
         */
        bool operator==(const Registration& rhs) {
            return (
                productId == rhs.productId && serialNumber == rhs.serialNumber &&
                registrationKey == rhs.registrationKey && productIdKey == rhs.productIdKey);
        }

        /**
         * Operator != for @c Registration.
         * @param rhs The object to compare to @c this.
         * @return Whether the operation holds.
         */
        bool operator!=(const Registration& rhs) {
            return !(*this == rhs);
        }
    };

    /**
     * The additional attributes structure that may be used to provide more information about an endpoint.
     *
     * @note Each field can contain up to 256 alphanumeric characters, and can contain punctuation.
     */
    struct AdditionalAttributes {
        /// The name of the manufacturer of the device.
        std::string manufacturer;
        /// The name of the model of the device.
        std::string model;
        /// The serial number of the device.
        std::string serialNumber;
        /// The firmware version of the device.
        std::string firmwareVersion;
        /// The software version of the device.
        std::string softwareVersion;
        /// Your custom identifier for the device.
        std::string customIdentifier;
    };

    /// Maximum length of each endpoint attribute:
    /// See format specification here:
    /// https://developer.amazon.com/docs/alexa/alexa-voice-service/alexa-discovery.html#addorupdatereport
    /// @{
    static constexpr size_t MAX_ENDPOINT_IDENTIFIER_LENGTH = 256;
    static constexpr size_t MAX_FRIENDLY_NAME_LENGTH = 128;
    static constexpr size_t MAX_MANUFACTURER_NAME_LENGTH = 128;
    static constexpr size_t MAX_DESCRIPTION_LENGTH = 128;
    static constexpr size_t MAX_ENDPOINT_ADDITIONAL_ATTRIBUTES_LENGTH = 256;
    static constexpr size_t MAX_CONNECTIONS_VALUE_LENGTH = 256;
    /// @}

    /// Cookies cannot exceed 5KB.
    static constexpr size_t MAX_COOKIES_SIZE_BYTES = 5000;

    /// A unique ID to identify the endpoint. See @c EndpointIdentifier documentation for more information.
    sdkInterfaces::endpoints::EndpointIdentifier endpointId;

    /// A non-empty string that defines a name that customers can use to interact with the endpoint.
    /// @note This should be an empty string for the default endpoint.
    std::string friendlyName;

    /// A non-empty string with a description about the endpoint.
    std::string description;

    /// A non-empty string identifying the endpoint manufacturer name.
    std::string manufacturerName;

    /// The display categories the device belongs to. This field should contain at least one category. See categories
    /// in this document: https://developer.amazon.com/docs/alexa/device-apis/alexa-discovery.html#display-categories
    /// @note: This value should only include ALEXA_VOICE_ENABLED for the default endpoint.
    std::vector<std::string> displayCategories;

    /// Registration field used to identify the default endpoint.
    utils::Optional<Registration> registration;

    /// Contains additional information that can be used to identify an endpoint.
    utils::Optional<AdditionalAttributes> additionalAttributes;

    /// The optional connections list describing how the endpoint is connected to the internet or smart home hub.
    /// You can find the values available here:
    /// https://developer.amazon.com/docs/alexa/alexa-voice-service/alexa-discovery.html#addorupdatereport
    std::vector<std::map<std::string, std::string>> connections;

    /// The optional custom key value pair used to store about the device. In the AVS documentation, this field name is
    /// 'cookie'.
    std::map<std::string, std::string> cookies;
};

inline AVSDiscoveryEndpointAttributes::Registration::Registration(
    const std::string& productId,
    const std::string& serialNumber,
    const std::string& registrationKey,
    const std::string& productIdKey) :
        productId(productId),
        serialNumber(serialNumber),
        registrationKey(registrationKey),
        productIdKey(productIdKey) {
}

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_AVSDISCOVERYENDPOINTATTRIBUTES_H_
