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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_LOCALEASSETSMANAGERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_LOCALEASSETSMANAGERINTERFACE_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include <AVSCommon/SDKInterfaces/CapabilityConfigurationChangeObserverInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointRegistrationManagerInterface.h>
#include <AVSCommon/SDKInterfaces/LocaleAssetsObserverInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * Interface for locale sensitive assets manager.
 *
 * A @c LocaleAssetsManagerInterface implementation should provide methods to get the supported locales and wake words
 * in the device. It should also implement a mechanism to change the current device locale and wake words.
 *
 * The methods @c prepareAssets and @c cancelPrepare MUST be thread safe.
 */
class LocaleAssetsManagerInterface : public CapabilityConfigurationChangeObserverInterface {
public:
    /**
     * Alias for the locale. The locale should follow <a href="https://tools.ietf.org/html/bcp47">BCP 47 format</a> and
     * it's composed of a language tag and a region tag, e.g.: en-CA.
     */
    using Locale = std::string;

    /**
     * Alias for the language tag. The language tag should follow <a href="https://tools.ietf.org/html/bcp47">BCP 47
     * format</a>.
     */
    using LanguageTag = std::string;

    /**
     * Represent a collection of wake words.
     */
    using WakeWords = std::set<std::string>;

    /**
     * Represent a set of wake words.
     */
    using WakeWordsSets = std::set<WakeWords>;

    /**
     * Represent a collection of locale.  This needs to be a vector because the collection is ordered, the locale at
     * index zero is the primary locale.
     */
    using Locales = std::vector<Locale>;

    /**
     * Represent the combinations of locales.
     */
    using LocaleCombinations = std::set<Locales>;

    /**
     * Method to change the current assets.
     *
     * @param locales The locales.
     * @param wakeWords The wake words that should be available and enabled.
     * @return @c true if it succeeds; @c false otherwise.
     */
    virtual bool changeAssets(const Locales& locales, const WakeWords& wakeWords) = 0;

    /**
     * Method used to cancel an ongoing @c prepareAssets call.
     *
     * This will get called when there is a change to the required assets. It's up to the implementation to decide how
     * and when to cancel the ongoing operation.
     */
    virtual void cancelOngoingChange() = 0;

    /**
     * Get the supported locales.
     *
     * @return A set with all the supported locales.
     */
    virtual std::set<Locale> getSupportedLocales() const = 0;

    /**
     * Get the supported locales combinations.
     *
     * @return A vector with all the supported locales combinations.
     *
     * @note Order matters for the locale combination, as the first locale presented is the primary locale.  Each
     * combination will have more than one locale, and each locale in the combination must be a supported locale.
     */
    virtual LocaleCombinations getSupportedLocaleCombinations() const = 0;

    /**
     * Get the default locale.
     *
     * @deprecated Use getDefaultLocales
     *
     * @return The default locale.
     */
    virtual Locale getDefaultLocale() const = 0;

    /**
     * Get the default multilingual locales.
     *
     * @return The default multilingual locales.
     */
    virtual Locales getDefaultLocales() const;

    /**
     * Get the default valid concurrent wake words sets.
     *
     * @return The valid concurrent wake words sets supported in most locales.
     * @note See @c getLocaleSpecificWakeWords() for locales that have a different set of supported wake words.
     * @warning The supported set of wake words MUST include ALEXA.
     */
    virtual WakeWordsSets getDefaultSupportedWakeWords() const = 0;

    /**
     * Function that return valid concurrent wake words sets per language (if and only if locale support a set of wake
     * words that's different than the default set).
     *
     * @return A map of valid concurrent wake words sets per language.
     * @warning The supported wake words MUST include ALEXA for all languages.ExecutorTest.cpp
     */
    virtual std::map<LanguageTag, WakeWordsSets> getLanguageSpecificWakeWords() const = 0;

    /**
     * Function that return the valid concurrent wake words sets per locale (if and only if locale support a set of wake
     * words that's different than the default set or the language set).
     *
     * @return A map of valid concurrent wake words sets per locale.
     * @warning The supported wake words MUST include ALEXA for all locales.
     */
    virtual std::map<Locale, WakeWordsSets> getLocaleSpecificWakeWords() const = 0;

    /**
     * Get the valid concurrent wake words sets for the given locale.
     *
     * @param locale The target locale.
     * @return The valid concurrent wake words sets of the given locale.
     */
    virtual WakeWordsSets getSupportedWakeWords(const Locale& locale) const = 0;

    /**
     * Add a locale assets observer to be notified when locale assets have updated.
     *
     * @param observer The observer to add.
     */
    virtual void addLocaleAssetsObserver(const std::shared_ptr<LocaleAssetsObserverInterface>& observer) = 0;

    /**
     * Remove a previously registered observer.
     *
     * @param observer The observer to be removed.
     */
    virtual void removeLocaleAssetsObserver(const std::shared_ptr<LocaleAssetsObserverInterface>& observer) = 0;

    /**
     * Set the @c EndpointRegistrationManager to update locales/wakewords capabilities.
     *
     * @param manager The pointer to @c EndpointRegistrationManager.
     */
    virtual void setEndpointRegistrationManager(
        const std::shared_ptr<endpoints::EndpointRegistrationManagerInterface>& manager) = 0;

    /**
     * Destructor.
     */
    virtual ~LocaleAssetsManagerInterface() = default;
};

inline LocaleAssetsManagerInterface::Locales LocaleAssetsManagerInterface::getDefaultLocales() const {
    return {getDefaultLocale()};
}

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_LOCALEASSETSMANAGERINTERFACE_H_
