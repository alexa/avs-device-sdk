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

#include <fstream>
#include <unordered_map>

#include <AVSCommon/Utils/JSON/JSONUtils.h>

#include "IPCServerSampleApp/CachingDownloadManager.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {

static const std::string TAG{"CachingDownloadManager"};
#define LX(event) avsCommon::utils::logger::LogEntry(TAG, event)

using namespace avsCommon::avs::attachment;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::json;
using namespace avsCommon::utils::libcurlUtils;
using namespace avsCommon::utils::sds;

/// Process attachment ID
static const std::string PROCESS_ATTACHMENT_ID = "import_download:";
/// A wait period for a polling loop that constantly check if a content fetcher finished fetching the payload or failed.
static const std::chrono::milliseconds WAIT_FOR_ACTIVITY_TIMEOUT{100};
/// Timeout to wait for am item to arrive from the content fetcher
static const std::chrono::minutes FETCH_TIMEOUT{5};
/// The number of bytes read from the attachment with each read in the read loop.
static const size_t CHUNK_SIZE(1024);
/// Component name for SmartScreenSampleApp
static const std::string COMPONENT_NAME = "IPCServerSampleApp";
/// Table name for APL packages
static const std::string TABLE_NAME = "Packages";
/// Delimiter to separate package content and import time, since we currently have only one column for value in misc
/// storage
static const std::string DELIMITER = "||||";
/// The number of retries when downloading a package from source
static const int DOWNLOAD_FROM_SOURCE_RETRY_ATTEMPTS = 3;

CachingDownloadManager::CachingDownloadManager(
    std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface>
        httpContentFetcherInterfaceFactoryInterface,
    unsigned long cachePeriodInSeconds,
    unsigned long maxCacheSize,
    std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> miscStorage,
    const std::shared_ptr<registrationManager::CustomerDataManagerInterface> customerDataManager) :
        CustomerDataHandler{customerDataManager},
        m_contentFetcherFactory{httpContentFetcherInterfaceFactoryInterface},
        m_cachePeriod{std::chrono::seconds(cachePeriodInSeconds)},
        m_maxCacheSize{maxCacheSize},
        m_miscStorage(miscStorage) {
    bool doesTableExist = false;
    if (!m_miscStorage->tableExists(COMPONENT_NAME, TABLE_NAME, &doesTableExist)) {
        ACSDK_ERROR(LX(__func__).m("Cannot check for table existence."));
    }

    if (!doesTableExist) {
        if (!m_miscStorage->createTable(
                COMPONENT_NAME,
                TABLE_NAME,
                avsCommon::sdkInterfaces::storage::MiscStorageInterface::KeyType::STRING_KEY,
                avsCommon::sdkInterfaces::storage::MiscStorageInterface::ValueType::STRING_VALUE)) {
            ACSDK_ERROR(LX(__func__).m("Cannot create table for storing APL packages."));
        }
    } else {
        std::unordered_map<std::string, std::string> packageMap;
        if (!m_miscStorage->load(COMPONENT_NAME, TABLE_NAME, &packageMap)) {
            ACSDK_ERROR(LX(__func__).m("Cannot load downloaded packages."));
        }
        for (auto& kvp : packageMap) {
            size_t delimiterPos = kvp.second.find(DELIMITER);
            if (delimiterPos == std::string::npos) {
                ACSDK_ERROR(LX(__func__).m("Package content for " + kvp.first + " is corrupted."));
            } else {
                auto timeStamp = kvp.second.substr(0, delimiterPos);
                auto packageContent = kvp.second.substr(delimiterPos + DELIMITER.length());
                auto importTime =
                    std::chrono::time_point<std::chrono::system_clock>(std::chrono::seconds(std::stol(timeStamp)));
                if (std::chrono::system_clock::now() - importTime < m_cachePeriod) {
                    ACSDK_DEBUG9(LX(__func__).m("Loaded package " + kvp.first + " from misc storage"));
                    cachedContentMap[kvp.first] = CachedContent(importTime, packageContent);
                }
            }
        }
    }
}

CachingDownloadManager::CachedContent::CachedContent(
    std::chrono::system_clock::time_point importTime,
    const std::string& content) {
    this->importTime = importTime;
    this->content = content;
}

std::string CachingDownloadManager::retrieveContent(const std::string& source, std::shared_ptr<Observer> observer) {
    {
        const std::lock_guard<std::mutex> lock(cachedContentMapMutex);
        if (cachedContentMap.find(source) != cachedContentMap.end()) {
            if ((std::chrono::system_clock::now() - cachedContentMap[source].importTime) < m_cachePeriod) {
                ACSDK_DEBUG9(LX("retrieveContent").d("contentSource", "returnedFromCache"));
                if (observer) {
                    observer->onCacheHit();
                }
                return cachedContentMap[source].content;
            }
        }
    }

    std::string content;

    for (int i = 0; i < DOWNLOAD_FROM_SOURCE_RETRY_ATTEMPTS; i++) {
        content = downloadFromSource(source, observer);
        ACSDK_DEBUG9(LX("retrieveContent").d("contentSource", "downloadedFromSource"));

        if (!content.empty()) {
            auto cachedContent = CachedContent(std::chrono::system_clock::now(), content);
            {
                const std::lock_guard<std::mutex> lock(cachedContentMapMutex);
                cachedContentMap[source] = cachedContent;
                cleanUpCache();
            }

            writeToStorage(source, cachedContent);
            return content;
        }
    }
    ACSDK_ERROR(LX("retrieveContent").d("contentSource", "downloadedFromSourceFailedAllRetries"));

    return content;
}

void CachingDownloadManager::writeToStorage(std::string source, CachingDownloadManager::CachedContent content) {
    m_executor.submit([this, source, content] {
        if (!m_miscStorage->put(COMPONENT_NAME, TABLE_NAME, source, cachedContentToString(content, DELIMITER))) {
            ACSDK_ERROR(LX(__func__).m("Failed to write package to disk storage."));
        } else {
            ACSDK_DEBUG9(LX(__func__).m("Successfully stored " + source + " to disk"));
        }
    });
}

void CachingDownloadManager::cleanUpCache() {
    std::chrono::system_clock::time_point oldestTime = std::chrono::system_clock::time_point::max();
    std::string oldestSource;

    for (auto it = cachedContentMap.begin(); it != cachedContentMap.end();) {
        if ((std::chrono::system_clock::now() - it->second.importTime) > m_cachePeriod) {
            removeFromStorage(it->first);
            it = cachedContentMap.erase(it);
            ACSDK_DEBUG9(LX("cleanUpCache").d("deletedCacheEntry", "entryExpired"));
        } else {
            if (it->second.importTime < oldestTime) {
                oldestTime = it->second.importTime;
                oldestSource = it->first;
            }
            it++;
        }
    }

    if (cachedContentMap.size() > m_maxCacheSize) {
        cachedContentMap.erase(oldestSource);
        ACSDK_DEBUG9(LX("cleanUpCache").d("deletedCacheEntry", "maxCacheSizeReached"));
    }
}

void CachingDownloadManager::clearData() {
    ACSDK_DEBUG5(LX(__func__));
    if (!m_miscStorage->clearTable(COMPONENT_NAME, TABLE_NAME)) {
        ACSDK_ERROR(LX("clearTableFailed").d("reason", "unable to clear the table from the database"));
    }
}

void CachingDownloadManager::removeFromStorage(std::string source) {
    m_executor.submit([this, source] {
        if (!m_miscStorage->remove(COMPONENT_NAME, TABLE_NAME, source)) {
            ACSDK_ERROR(LX(__func__).m("Failed to remove package " + source + " from disk."));
        } else {
            ACSDK_DEBUG9(LX(__func__).m("Removed package " + source + " from disk."));
        }
    });
}

std::string CachingDownloadManager::downloadFromSource(const std::string& source, std::shared_ptr<Observer> observer) {
    if (observer) {
        observer->onDownloadStarted();
    }
    auto contentFetcher = m_contentFetcherFactory->create(source);
    contentFetcher->getContent(HTTPContentFetcherInterface::FetchOptions::ENTIRE_BODY);

    HTTPContentFetcherInterface::Header header = contentFetcher->getHeader(nullptr);
    if (!header.successful) {
        ACSDK_ERROR(LX(__func__).sensitive("source", source).m("getHeaderFailed"));
        if (observer) {
            observer->onDownloadFailed();
        }
        return "";
    }

    if (!isStatusCodeSuccess(header.responseCode)) {
        ACSDK_ERROR(LX("downloadFromSourceFailed")
                        .d("statusCode", header.responseCode)
                        .sensitive("url", source)
                        .d("reason", "nonSuccessStatusCodeFromGetHeader"));
        if (observer) {
            observer->onDownloadFailed();
        }
        return "";
    }

    ACSDK_DEBUG9(LX("downloadFromSource")
                     .d("contentType", header.contentType)
                     .d("statusCode", header.responseCode)
                     .sensitive("url", source)
                     .m("headersReceived"));

    auto stream = std::make_shared<InProcessAttachment>(PROCESS_ATTACHMENT_ID);
    std::shared_ptr<AttachmentWriter> streamWriter = stream->createWriter(WriterPolicy::BLOCKING);

    if (!contentFetcher->getBody(streamWriter)) {
        ACSDK_ERROR(LX("downloadFromSourceFailed").d("reason", "getBodyFailed"));
        if (observer) {
            observer->onDownloadFailed();
        }
        return "";
    }

    auto startTime = std::chrono::steady_clock::now();
    auto elapsedTime = std::chrono::steady_clock::now() - startTime;
    HTTPContentFetcherInterface::State contentFetcherState = contentFetcher->getState();
    while ((FETCH_TIMEOUT > elapsedTime) && (HTTPContentFetcherInterface::State::BODY_DONE != contentFetcherState) &&
           (HTTPContentFetcherInterface::State::ERROR != contentFetcherState)) {
        std::this_thread::sleep_for(WAIT_FOR_ACTIVITY_TIMEOUT);
        elapsedTime = std::chrono::steady_clock::now() - startTime;
        contentFetcherState = contentFetcher->getState();
    }

    if (FETCH_TIMEOUT <= elapsedTime) {
        ACSDK_ERROR(LX("downloadFromSourceFailed").d("reason", "waitTimeout"));
        if (observer) {
            observer->onDownloadFailed();
        }
        return "";
    }

    if (HTTPContentFetcherInterface::State::ERROR == contentFetcherState) {
        ACSDK_ERROR(LX("downloadFromSourceFailed").d("reason", "receivingBodyFailed"));
        if (observer) {
            observer->onDownloadFailed();
        }
        return "";
    }

    std::unique_ptr<AttachmentReader> reader = stream->createReader(ReaderPolicy::NONBLOCKING);

    std::string content;
    auto readStatus = AttachmentReader::ReadStatus::OK;
    std::vector<char> buffer(CHUNK_SIZE, 0);
    bool streamClosed = false;
    AttachmentReader::ReadStatus previousStatus = AttachmentReader::ReadStatus::OK_TIMEDOUT;
    ssize_t bytesRead = -1;
    while (!streamClosed && bytesRead != 0) {
        bytesRead = reader->read(buffer.data(), buffer.size(), &readStatus);
        if (previousStatus != readStatus) {
            ACSDK_DEBUG9(LX(__func__).d("readStatus", readStatus));
            previousStatus = readStatus;
        }
        switch (readStatus) {
            case AttachmentReader::ReadStatus::CLOSED:
                streamClosed = true;
                if (bytesRead == 0) {
                    break;
                }
                /* FALL THROUGH - to add any data received even if closed */
            case AttachmentReader::ReadStatus::OK:
            case AttachmentReader::ReadStatus::OK_WOULDBLOCK:
            case AttachmentReader::ReadStatus::OK_TIMEDOUT:
                content.append(buffer.data(), bytesRead);
                break;
            case AttachmentReader::ReadStatus::OK_OVERRUN_RESET:
                // Current AttachmentReader policy renders this outcome impossible.
                ACSDK_ERROR(LX("downloadFromSourceFailed").d("reason", "overrunReset"));
                break;
            case AttachmentReader::ReadStatus::ERROR_OVERRUN:
            case AttachmentReader::ReadStatus::ERROR_BYTES_LESS_THAN_WORD_SIZE:
            case AttachmentReader::ReadStatus::ERROR_INTERNAL:
                ACSDK_ERROR(LX("downloadFromSourceFailed").d("reason", "readError"));
                if (observer) {
                    observer->onDownloadFailed();
                }
                return "";
        }
        if (0 == bytesRead) {
            ACSDK_DEBUG9(LX("downloadFromSource").m("alreadyReadAllBytes"));
        } else if (observer) {
            observer->onBytesRead(bytesRead);
        }
    }

    ACSDK_DEBUG9(LX("downloadFromSource").d("URL", contentFetcher->getUrl()));

    if (observer) {
        observer->onDownloadComplete();
    }
    return content;
}

}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
