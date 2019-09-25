/*
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_LOCALEASSETSMANAGER_H_
#define ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_LOCALEASSETSMANAGER_H_

#include <map>
#include <memory>
#include <set>

#include <AVSCommon/SDKInterfaces/LocaleAssetsManagerInterface.h>

namespace alexaClientSDK {
namespace sampleApp {

/**
 * Sample implementation of an asset manager.
 *
 * This manager will use the @c AlexaClientSDKConfig.json to retrieve the supported locales. For devices with wake word
 * enabled this class will support "ALEXA" only.
 */
class LocaleAssetsManager : public avsCommon::sdkInterfaces::LocaleAssetsManagerInterface {
public:
    /**
     * Create a LocaleAssetsManager object.
     *
     * @param enableWakeWord Indicates whether wake words are enabled in this device or not.
     * @return A pointer to a new LocaleAssetsManager object if it succeeds; otherwise, @c nullptr.
     */
    static std::shared_ptr<LocaleAssetsManager> create(bool enableWakeWord);

    /// @name LocaleAssetsManagerInterface methods
    /// @{
    bool changeAssets(const Locales& locales, const WakeWords& wakeWords) override;
    void cancelOngoingChange() override;
    WakeWordsSets getDefaultSupportedWakeWords() const override;
    std::map<LanguageTag, WakeWordsSets> getLanguageSpecificWakeWords() const override;
    std::map<Locale, WakeWordsSets> getLocaleSpecificWakeWords() const override;
    WakeWordsSets getSupportedWakeWords(const Locale& locale) const override;
    std::set<Locale> getSupportedLocales() const override;
    LocaleCombinations getSupportedLocaleCombinations() const override;
    Locale getDefaultLocale() const override;
    /// @}
private:
    /**
     * Constructor.
     */
    LocaleAssetsManager();

    /**
     * Initialize the assets manager object.
     *
     * @param enableWakeWord Indicates whether wake words are enabled in this device or not.
     * @return @c true if it succeeds; otherwise, return @c false.
     */
    bool initialize(bool enableWakeWord);

    /// Set with the supported wake words. This object doesn't support different wake words per locale.
    WakeWordsSets m_supportedWakeWords;

    /// Set with the supported locales.
    std::set<Locale> m_supportedLocales;

    /// Vector of supported locales combinations.
    LocaleCombinations m_supportedLocalesCombinations;

    /// The default locale.
    Locale m_defaultLocale;
};

}  // namespace sampleApp
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_LOCALEASSETSMANAGER_H_
