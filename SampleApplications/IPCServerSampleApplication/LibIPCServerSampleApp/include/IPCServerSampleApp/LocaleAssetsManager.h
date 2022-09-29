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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_LOCALEASSETSMANAGER_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_LOCALEASSETSMANAGER_H_

#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <unordered_set>

#include <acsdkShutdownManagerInterfaces/ShutdownNotifierInterface.h>
#include <AVSCommon/SDKInterfaces/LocaleAssetsManagerInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

#ifdef KWD
#ifdef ASSET_MANAGER
#include <acsdkAssetManager/AssetManager.h>
#endif
#include <acsdkKWDImplementations/AbstractKeywordDetector.h>
#include <AVSCommon/AVS/AudioInputStream.h>
#include <AVSCommon/Utils/AudioFormat.h>
#include <DefaultClient/DefaultClient.h>
#endif

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {

#if defined(KWD) && defined(ASSET_MANAGER)
using namespace acsdkAssets::manager;
#endif
/**
 * Sample implementation of an asset manager.
 *
 * This manager will use the @c AlexaClientSDKConfig.json to retrieve the supported locales. For devices with wake word
 * enabled this class will support "ALEXA" only.
 */
class LocaleAssetsManager
        : public avsCommon::sdkInterfaces::LocaleAssetsManagerInterface
        , public avsCommon::utils::RequiresShutdown {
public:
    /**
     * Create an instance of @c LocaleAssetsManagerInterface.
     *
     * @param configurationNode A pointer to the @c ConfigurationNode instance.
     * @param shutdownNotifier A pointer to the @c ShutdownNotifier instance.
     * @return A pointer to the @c LocaleAssetsManagerInterface.
     */
    static std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface> createLocaleAssetsManagerInterface(
        const std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>& configurationNode,
        const std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>& shutdownNotifier);

    /**
     * Create an instance of @c LocaleAssetsManager.
     *
     * @param configurationNode A pointer to the @c ConfigurationNode instance.
     * @return A pointer to a new LocaleAssetsManager object if it succeeds; otherwise, @c nullptr.
     */
    static std::shared_ptr<LocaleAssetsManager> createLocaleAssetsManager(
        const std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>& configurationNode);

    /**
     * Create a LocaleAssetsManager object.
     *
     * @deprecated
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
    Locales getDefaultLocales() const override;
    void addLocaleAssetsObserver(
        const std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsObserverInterface>& observer) override;
    void removeLocaleAssetsObserver(
        const std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsObserverInterface>& observer) override;
    void onConfigurationChanged(const avsCommon::avs::CapabilityConfiguration& configuration) override;
    void setEndpointRegistrationManager(
        const std::shared_ptr<avsCommon::sdkInterfaces::endpoints::EndpointRegistrationManagerInterface>& manager)
        override;
    /// @}

    /// @name RequiresShutdown methods
    /// @{
    void doShutdown() override;
    /// @}

#ifdef KWD
#ifdef ASSET_MANAGER
    /**
     * Create an instance of @c LocaleAssetsManagerInterface with KWD and DAVS Asset Manager support.
     *
     * @param configurationNode A pointer to the @c ConfigurationNode instance.
     * @param shutdownNotifier A pointer to the @c ShutdownNotifier instance.
     * @param audioInputStream A pointer to the @c AudioInputStream instance.
     * @param audioFormat A pointer to the @c AudioFormat instance.
     * @param keywordDetector A pointer to the @c AbstractKeywordDetector instance.
     * @param assetManager A pointer to the @c DavsAssetManager instance.
     * @return A pointer to the @c LocaleAssetsManagerInterface.
     */
    static std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface>
    createLocaleAssetsManagerInterfaceWithKwdAndDavs(
        const std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>& configurationNode,
        const std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>& shutdownNotifier,
        const std::shared_ptr<avsCommon::avs::AudioInputStream>& audioInputStream,
        const std::shared_ptr<avsCommon::utils::AudioFormat>& audioFormat,
        const std::shared_ptr<acsdkKWDImplementations::AbstractKeywordDetector>& keywordDetector,
        const std::shared_ptr<AssetManager>& assetManager);

    /**
     * Create an instance of @c LocaleAssetsManager with KWD and DAVS Asset Manager support.
     *
     * @param configurationNode A pointer to the @c ConfigurationNode instance.
     * @param audioInputStream A pointer to the @c AudioInputStream instance.
     * @param audioFormat A pointer to the @c AudioFormat instance.
     * @param keywordDetector A pointer to the @c AbstractKeywordDetector instance.
     * @param assetManager A pointer to the @c DavsAssetManager instance.
     * @return A pointer to a new LocaleAssetsManager object if it succeeds; otherwise, @c nullptr.
     */
    static std::shared_ptr<LocaleAssetsManager> createLocaleAssetsManagerWithKwdAndDavs(
        const std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>& configurationNode,
        const std::shared_ptr<avsCommon::avs::AudioInputStream>& audioInputStream,
        const std::shared_ptr<avsCommon::utils::AudioFormat>& audioFormat,
        const std::shared_ptr<acsdkKWDImplementations::AbstractKeywordDetector>& keywordDetector,
        const std::shared_ptr<AssetManager>& assetManager);
#else
    /**
     * Create an instance of @c LocaleAssetsManagerInterface with KWD support.
     *
     * @param configurationNode A pointer to the @c ConfigurationNode instance.
     * @param shutdownNotifier A pointer to the @c ShutdownNotifier instance.
     * @param audioInputStream A pointer to the @c AudioInputStream instance.
     * @param audioFormat A pointer to the @c AudioFormat instance.
     * @param keywordDetector A pointer to the @c AbstractKeywordDetector instance.
     * @return A pointer to the @c LocaleAssetsManagerInterface.
     */
    static std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface>
    createLocaleAssetsManagerInterfaceWithKwd(
        const std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>& configurationNode,
        const std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>& shutdownNotifier,
        const std::shared_ptr<avsCommon::avs::AudioInputStream>& audioInputStream,
        const std::shared_ptr<avsCommon::utils::AudioFormat>& audioFormat,
        const std::shared_ptr<acsdkKWDImplementations::AbstractKeywordDetector>& keywordDetector);

    /**
     * Create an instance of @c LocaleAssetsManager with KWD support.
     *
     * @param configurationNode A pointer to the @c ConfigurationNode instance.
     * @param audioInputStream A pointer to the @c AudioInputStream instance.
     * @param audioFormat A pointer to the @c AudioFormat instance.
     * @param keywordDetector A pointer to the @c AbstractKeywordDetector instance.
     * @return A pointer to a new LocaleAssetsManager object if it succeeds; otherwise, @c nullptr.
     */
    static std::shared_ptr<LocaleAssetsManager> createLocaleAssetsManagerWithKwd(
        const std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>& configurationNode,
        const std::shared_ptr<avsCommon::avs::AudioInputStream>& audioInputStream,
        const std::shared_ptr<avsCommon::utils::AudioFormat>& audioFormat,
        const std::shared_ptr<acsdkKWDImplementations::AbstractKeywordDetector>& keywordDetector);
#endif
    void setDefaultClient(const std::shared_ptr<defaultClient::DefaultClient>& defaultClient);
#endif
private:
    /**
     * Constructor.
     */
    LocaleAssetsManager();

#ifdef KWD
#ifdef ASSET_MANAGER
    /**
     * Constructor with KWD and DAVS Asset Manager support.
     *
     * @param audioInputStream AudioInputStream.
     * @param audioFormat AudioFormat.
     * @param keywordDetector AbstractKeywordDetector.
     * @param assetManager Davs AssetManager.
     */
    LocaleAssetsManager(
        const std::shared_ptr<avsCommon::avs::AudioInputStream>& audioInputStream,
        const std::shared_ptr<avsCommon::utils::AudioFormat>& audioFormat,
        const std::shared_ptr<acsdkKWDImplementations::AbstractKeywordDetector>& keywordDetector,
        const std::shared_ptr<AssetManager>& assetManager);
#else
    /**
     * Constructor with KWD support.
     *
     * @param audioFormat AudioInputStream.
     * @param audioFormat AudioFormat.
     * @param keywordDetector AbstractKeywordDetector.
     */
    LocaleAssetsManager(
        const std::shared_ptr<avsCommon::avs::AudioInputStream>& audioInputStream,
        const std::shared_ptr<avsCommon::utils::AudioFormat>& audioFormat,
        const std::shared_ptr<acsdkKWDImplementations::AbstractKeywordDetector>& keywordDetector);
#endif
#endif
    /**
     * Initialize the assets manager object.
     *
     * @param configurationNode The @c ConfigurationNode pointer.
     * @return @c true if it succeeds; otherwise, return @c false.
     */
    bool initialize(const std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>& configurationNode);

    /**
     * Initialize the assets manager object.
     *
     * @param enableWakeWord Indicates whether wake words are enabled in this device or not.
     * @param configurationNode The @c ConfigurationNode instance.
     * @return @c true if it succeeds; otherwise, return @c false.
     */
    bool initialize(bool enableWakeWord, const avsCommon::utils::configuration::ConfigurationNode& configurationNode);

    /// Set with the supported wake words. This object doesn't support different wake words per locale.
    WakeWordsSets m_supportedWakeWords;

    /// Set with the supported locales.
    std::set<Locale> m_supportedLocales;

    /// Vector of supported locales combinations.
    LocaleCombinations m_supportedLocalesCombinations;

    /// The default locale.
    Locale m_defaultLocale;

    /// The default multilingual locale.
    Locales m_defaultLocales;

#ifdef KWD
    /// The stream of audio data.
    const std::shared_ptr<avsCommon::avs::AudioInputStream> m_audioInputStream;
    /// The audio format.
    const std::shared_ptr<avsCommon::utils::AudioFormat> m_audioFormat;

    /// Mutex to synchronize access to m_DefaultClient.
    std::mutex m_defaultClientMutex;

    /// Mutex to synchronize access to m_keywordDetector.
    std::mutex m_keywordDetectorMutex;

    //// Members below are modified after initialization and hence should be synchronized.

    /// The default SDK client.
    std::shared_ptr<defaultClient::DefaultClient> m_DefaultClient;

    /// The Wakeword Detector which can wake up the client using audio input.
    std::shared_ptr<acsdkKWDImplementations::AbstractKeywordDetector> m_keywordDetector;
#ifdef ASSET_MANAGER
    /// Davs AssetManager.
    std::shared_ptr<AssetManager> m_assetManager;
#endif
#endif

    /// Mutex to synchronize access to observers.
    mutable std::mutex m_observersMutex;

    /// Mutex to synchronize access to @c EndpointRegistrationManager.
    std::mutex m_ermMutex;

    //// Members below are modified after initialization and hence should be synchronized.

    /// Set with observers.
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsObserverInterface>> m_observers;

    /// Dynamically update wakewords/locales capabilities changes.
    std::shared_ptr<avsCommon::sdkInterfaces::endpoints::EndpointRegistrationManagerInterface>
        m_endpointRegistrationManager;
};

}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_LOCALEASSETSMANAGER_H_
