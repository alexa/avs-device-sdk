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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_CACHINGDOWNLOADMANAGER_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_CACHINGDOWNLOADMANAGER_H_

#include <cstdint>
#include <memory>

#include <AVSCommon/Utils/LibcurlUtils/HTTPContentFetcherFactory.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>

#include <RegistrationManager/CustomerDataHandler.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {

class CachingDownloadManager : public registrationManager::CustomerDataHandler {
public:
    class Observer {
    public:
        Observer() = default;
        virtual ~Observer() = default;

        /**
         * Called at the start of a download, when a resource is not found in the cache.
         */
        virtual void onDownloadStarted(){};

        /**
         * Called when a resource was not found in the cache and has successfully been downloaded.
         */
        virtual void onDownloadComplete(){};

        /**
         * Called when a resource was not found in the cache and the attempt to download it has failed.
         */
        virtual void onDownloadFailed(){};

        /**
         * Called when a resource was found in the cache and downloading is not attempted.
         */
        virtual void onCacheHit(){};

        /**
         * Called during the download of a resource. Observers should expect multiple calls
         * to this method for a single download.
         *
         * @param numberOfBytes The number of bytes that have been downloaded.
         */
        virtual void onBytesRead(uint64_t numberOfBytes){};
    };

    /**
     * Constructor.
     *
     * @param httpContentFetcherFactory Pointer to a http content fetcher factory for making download requests
     * @param cachePeriodInSeconds Number of seconds to reuse cache for downloaded packages
     * @param maxCacheSize Maximum cache size for caching downloaded packages
     * @param miscStorage Wrapper to read and write to misc stor=age database
     */
    CachingDownloadManager(
        std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface>
            httpContentFetcherInterfaceFactoryInterface,
        unsigned long cachePeriodInSeconds,
        unsigned long maxCacheSize,
        std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> miscStorage,
        const std::shared_ptr<registrationManager::CustomerDataManagerInterface> customerDataManager);

    /**
     * Method that should be called when requesting content
     *
     * @param source URL
     * @param observer An observer to notify of the download, or @c nullptr to disable notifications.
     * @return content - either from cache or from source
     */
    std::string retrieveContent(const std::string& source, std::shared_ptr<Observer> observer = nullptr);

    /**
     * Class to define a cached content item
     */
    class CachedContent {
    public:
        /**
         * Time when the content was put into cache
         */
        std::chrono::system_clock::time_point importTime;
        /**
         * Content of the item
         */
        std::string content;

        CachedContent() = default;

        /**
         * Constructor
         *
         * @param importTime Time when the item was inserted into cache
         * @param content The content of the item
         */
        CachedContent(std::chrono::system_clock::time_point importTime, const std::string& content);
    };

private:
    /**
     * Downloads content requested by import from provided URL from source.
     * @param source URL
     * @param observer An observer to notify of the download, or @c nullptr to disable notifications.
     * @return content from source
     */
    std::string downloadFromSource(const std::string& source, std::shared_ptr<Observer> observer);
    /**
     * Scans the cache to remove all expired entries, and evict the oldest entry if cache is full
     */
    void cleanUpCache();
    /**
     * Write the downloaded content to storage.
     * @param source URL.
     * @param content CachedContent object.
     */
    void writeToStorage(std::string source, CachedContent content);

    /// @name CustomerDataHandler Function
    /// @{
    void clearData() override;
    /// @}

    /**
     * Remove the downloaded content from storage.
     * @param source  source URL.
     */
    void removeFromStorage(std::string source);
    /**
     * Used to create objects that can fetch remote HTTP content.
     */
    std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> m_contentFetcherFactory;
    /**
     * Reuse time for caching of downloaded content
     */
    std::chrono::duration<double> m_cachePeriod;
    /**
     * Max numbers of entries in cache for downloaded content
     */
    unsigned long m_maxCacheSize;
    /**
     * The hashmap that maps the source url to a CachedContent
     */
    std::unordered_map<std::string, CachingDownloadManager::CachedContent> cachedContentMap;
    /**
     * The mutex for cachedContentMap
     */
    std::mutex cachedContentMapMutex;
    /**
     * The wrapper to read and write to local misc storage.
     */
    std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> m_miscStorage;
    /**
     * An internal executor that performs execution of callable objects passed to it sequentially but asynchronously.
     */
    avsCommon::utils::threading::Executor m_executor;
};

/**
 * This is a function to convert the @c CachedContent to a string.
 */
inline std::string cachedContentToString(CachingDownloadManager::CachedContent content, std::string delimiter) {
    auto time = std::chrono::duration_cast<std::chrono::seconds>(content.importTime.time_since_epoch()).count();
    return std::to_string(time) + delimiter + content.content;
}

}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_CACHINGDOWNLOADMANAGER_H_
