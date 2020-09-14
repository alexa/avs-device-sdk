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

#include <algorithm>

#include <AVSCommon/AVS/CapabilityResources.h>
#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

/// String to identify log entries originating from this file.
static const std::string TAG("CapabilityResources");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) utils::logger::LogEntry(TAG, event)

/// Maximum length of the friendly name.
static constexpr size_t MAX_FRIENDLY_NAME_LENGTH = 16000;

CapabilityResources::CapabilityResources() : m_isValid{true} {
}

bool CapabilityResources::FriendlyName::operator==(const CapabilityResources::FriendlyName& rhs) const {
    return value == rhs.value && locale.valueOr("") == rhs.locale.valueOr("");
}

bool CapabilityResources::addFriendlyNameWithAssetId(const resources::AlexaAssetId& assetId) {
    if (assetId.empty()) {
        ACSDK_ERROR(LX("addFriendlyNameWithAssetIdFailed").d("reason", "invalidAssetId"));
        m_isValid = false;
        return false;
    }

    if (std::find(
            m_items.begin(),
            m_items.end(),
            CapabilityResources::FriendlyName(
                {assetId, utils::Optional<sdkInterfaces::LocaleAssetsManagerInterface::Locale>()})) != m_items.end()) {
        ACSDK_ERROR(LX("addFriendlyNameWithAssetIdFailed").d("reason", "duplicateAssetId").d("assetId", assetId));
        m_isValid = false;
        return false;
    }

    m_items.push_back({assetId, utils::Optional<sdkInterfaces::LocaleAssetsManagerInterface::Locale>()});

    return true;
}

bool CapabilityResources::addFriendlyNameWithText(
    const std::string& text,
    const sdkInterfaces::LocaleAssetsManagerInterface::Locale& locale) {
    if (text.length() == 0 || text.length() > MAX_FRIENDLY_NAME_LENGTH) {
        ACSDK_ERROR(LX("addFriendlyNameWithTextFailed").d("reason", "invalidText"));
        m_isValid = false;
        return false;
    }
    if (locale.empty()) {
        ACSDK_ERROR(LX("addFriendlyNameWithTextFailed").d("reason", "invalidLocale"));
        m_isValid = false;
        return false;
    }
    if (std::find(
            m_items.begin(),
            m_items.end(),
            CapabilityResources::FriendlyName(
                {text, utils::Optional<sdkInterfaces::LocaleAssetsManagerInterface::Locale>(locale)})) !=
        m_items.end()) {
        ACSDK_ERROR(LX("addFriendlyNameWithTextFailed")
                        .d("reason", "duplicateText")
                        .sensitive("text", text)
                        .sensitive("locale", locale));
        m_isValid = false;
        return false;
    }

    m_items.push_back({text, utils::Optional<sdkInterfaces::LocaleAssetsManagerInterface::Locale>(locale)});

    return true;
}

bool CapabilityResources::isValid() const {
    return m_isValid && m_items.size() > 0;
}

std::string CapabilityResources::toJson() const {
    if (!isValid()) {
        ACSDK_ERROR(LX("toJsonFailed").d("reason", "invalidCapabilityResources"));
        return "{}";
    }

    utils::json::JsonGenerator jsonGenerator;
    jsonGenerator.startArray("friendlyNames");
    for (const auto& item : m_items) {
        jsonGenerator.startArrayElement();
        if (item.locale.hasValue()) {
            jsonGenerator.addMember("@type", "text");
            jsonGenerator.startObject("value");
            jsonGenerator.addMember("text", item.value);
            jsonGenerator.addMember("locale", item.locale.value());
        } else {
            jsonGenerator.addMember("@type", "asset");
            jsonGenerator.startObject("value");
            jsonGenerator.addMember("assetId", item.value);
        }
        jsonGenerator.finishObject();  // value
        jsonGenerator.finishArrayElement();
    }
    jsonGenerator.finishArray();

    return jsonGenerator.toString();
}

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
