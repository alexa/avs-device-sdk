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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_CAPABILITYRESOURCES_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_CAPABILITYRESOURCES_H_

#include <vector>
#include <string>
#include <tuple>

#include <AVSCommon/AVS/AlexaAssetId.h>
#include <AVSCommon/SDKInterfaces/LocaleAssetsManagerInterface.h>
#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

/**
 * This class represents the resources used by a Capability, communicated as friendly names to AVS.
 * @see https://developer.amazon.com/docs/alexa/device-apis/resources-and-assets.html#capability-resources
 */
class CapabilityResources {
public:
    /**
     * The constructor.
     */
    CapabilityResources();

    /**
     * Function to add friendly name with asset id.
     *
     * @note It is recommended to use asset identifier whenever available, as the
     * friendly name are already localized into all Alexa supported languages.
     *
     * @param assetId The asset id of the friendly name using @c AlexaAssetId.
     * @return Return @c true if successful in adding the asset id, otherwise false.
     */
    bool addFriendlyNameWithAssetId(const resources::AlexaAssetId& assetId);

    /**
     * Function to add friendly name with text value and its locale.
     *
     * @note When using this method it is recommended to provide the friendly names
     * in all the Alexa supported languages. See the class-level link to find the currently
     * supported languages.
     *
     * @note Providing an unsupported locale will result in Discovery failure.
     *
     * @param text The text of the friendly name.
     * @param locale The locale of the friendly name @c Locale.
     * @return Return @c true if successful in adding the text and locale, otherwise false.
     */
    bool addFriendlyNameWithText(
        const std::string& text,
        const sdkInterfaces::LocaleAssetsManagerInterface::Locale& locale);

    /**
     * Function to check if the @c CapabilityResources is valid.
     *
     * @return Return @c true if valid, otherwise @c false.
     */
    bool isValid() const;

    /**
     * Helper function to convert a @c FriendlyName to a json string.
     *
     * @note This follows the AVS discovery message format.
     *
     * @return A json string of @c FriendlyName.
     */
    std::string toJson() const;

private:
    /**
     * Struct to hold the friendly name data.
     */
    struct FriendlyName {
        /// The value to contain the text or the asset id of the friendly name.
        std::string value;

        /// The locale of the text friendly name, and empty object for asset.
        utils::Optional<sdkInterfaces::LocaleAssetsManagerInterface::Locale> locale;

        /**
         *  @name Comparison operator.
         *
         *  Compare the current FriendlyName against a second object.
         *  Defined for std::find.
         *
         *  @param rhs The object to compare against this.
         *  @return @c true if the comparison holds; @c false otherwise.
         */
        /// @{
        bool operator==(const FriendlyName& rhs) const;
        /// @}
    };

    /// Flag to indicate if there was any error noted.
    bool m_isValid;

    /// Vector holding the @c FriendlyName.
    std::vector<FriendlyName> m_items;
};

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_CAPABILITYRESOURCES_H_
