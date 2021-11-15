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

#include "acsdkDavsClient/DavsHandler.h"

#include <AVSCommon/Utils/Error/FinallyGuard.h>
#include <AVSCommon/Utils/FileSystem/FileSystemUtils.h>
#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/Power/WakeGuard.h>

#include <future>

#include "acsdkAssetsCommon/Base64Url.h"
#include "acsdkAssetsCommon/CurlWrapper.h"
#include "acsdkAssetsCommon/JitterUtil.h"
#include "acsdkAssetsCommon/ResponseSink.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace davs {

using namespace std;
using namespace chrono;
using namespace common;
using namespace commonInterfaces;
using namespace davsInterfaces;
using namespace alexaClientSDK::avsCommon::utils;
using namespace alexaClientSDK::avsCommon::utils::timing;
using namespace alexaClientSDK::avsCommon::utils::error;
using namespace alexaClientSDK::avsCommon::utils::power;
using namespace alexaClientSDK::avsCommon::utils::json;
using namespace alexaClientSDK::avsCommon::sdkInterfaces;

static const float JITTER_FACTOR = 0.3;

#if UNIT_TEST == 1
// For tests, because we don't want to wait hours for it to finish...
static constexpr auto MIN_UPDATE_INTERVAL = milliseconds(10);
static constexpr auto MAX_CHECK_RETRY = 2;
static constexpr auto MAX_DOWNLOAD_RETRY = 2;
static constexpr auto NETWORK_CONNECTION_WAIT_TIME = milliseconds(20);
#else
static constexpr auto MIN_UPDATE_INTERVAL = minutes(15);
static constexpr auto MAX_CHECK_RETRY = 4;
static constexpr auto MAX_DOWNLOAD_RETRY = 8;
static constexpr auto NETWORK_CONNECTION_WAIT_TIME = seconds(20);
#endif

static constexpr auto MS_IN_SEC = 1000;
static constexpr auto BYTES_IN_KB = 1024;

/// String to identify log entries originating from this file.
static const string TAG{"DavsHandler"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

const function<AmdMetricsWrapper()> DavsHandler::s_metrics = AmdMetricsWrapper::creator("davsHandler");

shared_ptr<DavsHandler> DavsHandler::create(
        shared_ptr<DavsRequest> artifactRequest,
        const shared_ptr<DavsDownloadCallbackInterface>& downloadRequester,
        const shared_ptr<DavsCheckCallbackInterface>& checkRequester,
        string workingDirectory,
        milliseconds baseBackOffTime,
        milliseconds maxBackOffTime,
        shared_ptr<AuthDelegateInterface> authDelegate,
        shared_ptr<DavsEndpointHandlerInterface> davsEndpointHandler,
        shared_ptr<PowerResource> powerResource,
        seconds forcedUpdateInterval) {
    if (artifactRequest == nullptr) {
        s_metrics().addCounter(METRIC_PREFIX_ERROR_CREATE("nullArtifactRequest"));
        ACSDK_ERROR(LX("create").m("Null Artifact Request"));
        return nullptr;
    }

    if (downloadRequester == nullptr) {
        s_metrics().addCounter(METRIC_PREFIX_ERROR_CREATE("nullDownloadRequestor"));
        ACSDK_ERROR(LX("create").m("Null Download Requester"));
        return nullptr;
    }

    if (checkRequester == nullptr) {
        s_metrics().addCounter(METRIC_PREFIX_ERROR_CREATE("nullCheckRequester"));
        ACSDK_ERROR(LX("create").m("Null Check Requester"));
        return nullptr;
    }
    if (workingDirectory.empty()) {
        s_metrics().addCounter(METRIC_PREFIX_ERROR_CREATE("invalidWorkingDirectory"));
        ACSDK_ERROR(LX("create").m("Invalid working directory"));
        return nullptr;
    }

    if (baseBackOffTime <= milliseconds::zero()) {
        s_metrics().addCounter(METRIC_PREFIX_ERROR_CREATE("invalidBackoffTime"));
        ACSDK_ERROR(LX("create").m("Invalid base backoff time"));
        return nullptr;
    }

    if (maxBackOffTime <= milliseconds::zero() || maxBackOffTime < baseBackOffTime) {
        s_metrics().addCounter(METRIC_PREFIX_ERROR_CREATE("invalidBackoffTime"));
        ACSDK_ERROR(LX("create").m("Invalid max backoff time"));
        return nullptr;
    }

    if (authDelegate == nullptr) {
        s_metrics().addCounter(METRIC_PREFIX_ERROR_CREATE("invalidAuthDelegate"));
        ACSDK_ERROR(LX("create").m("Invalid Auth Delegate"));
        return nullptr;
    }

    if (davsEndpointHandler == nullptr) {
        s_metrics().addCounter(METRIC_PREFIX_ERROR_CREATE("invalidDavsEndpointHandler"));
        ACSDK_ERROR(LX("create").m("Invalid DAVS Endpoint Handler"));
        return nullptr;
    }

    return unique_ptr<DavsHandler>(new DavsHandler(
            move(artifactRequest),
            downloadRequester,
            checkRequester,
            move(workingDirectory),
            baseBackOffTime,
            maxBackOffTime,
            move(authDelegate),
            move(davsEndpointHandler),
            move(powerResource),
            max(seconds::zero(), forcedUpdateInterval)));
}

DavsHandler::DavsHandler(
        shared_ptr<DavsRequest> artifactRequest,
        const shared_ptr<DavsDownloadCallbackInterface>& downloadRequester,
        const shared_ptr<DavsCheckCallbackInterface>& checkRequester,
        string workingDirectory,
        milliseconds baseBackOffTime,
        milliseconds maxBackOffTime,
        shared_ptr<AuthDelegateInterface> authDelegate,
        shared_ptr<DavsEndpointHandlerInterface> davsEndpointHandler,
        shared_ptr<PowerResource> powerResource,
        seconds forcedUpdateInterval) :
        m_artifactRequest(move(artifactRequest)),
        m_downloadRequester(downloadRequester),
        m_checkRequester(checkRequester),
        m_workingDirectory(move(workingDirectory)),
        m_baseBackOffTime(baseBackOffTime),
        m_maxBackOffTime(maxBackOffTime),
        m_authDelegate(move(authDelegate)),
        m_davsEndpointHandler(move(davsEndpointHandler)),
        m_powerResource(move(powerResource)),
        m_forcedUpdateInterval(forcedUpdateInterval),
        m_firstBackOffTime(milliseconds::zero()),
        m_shutdown(false),
        m_updateEnabled(false),
        m_throttled(false),
        m_networkConnected(false),
        m_unpack(this->m_artifactRequest->needsUnpacking()) {
    m_running.clear();
}

DavsHandler::~DavsHandler() {
    ACSDK_DEBUG(LX("~DavsHandler").m("Cancelling current request"));
    cancel();
}

void DavsHandler::requestAndDownload(bool isUserInitiated) {
    if (!isRelevant()) {
        ACSDK_WARN(LX("requestAndDownload").m("Requesting a download to a Davs Handler that's no longer relevant"));
        return;
    }

    s_metrics().addCounter("requestAndDownload-throttled", isThrottled() ? 1 : 0);
    s_metrics().addCounter("requestAndDownload");

    lock_guard<mutex> lock(m_eventMutex);
    if (!m_running.test_and_set()) {
        ACSDK_DEBUG(LX("requestAndDownload")
                            .m("Launching new check/download thread")
                            .d("request", m_artifactRequest->getSummary()));
        m_taskFuture = async(launch::async, &DavsHandler::runner, this, isUserInitiated);
        m_shutdown = false;
    }
}

void DavsHandler::handleUpdateLocked() {
    if (!m_updateEnabled) {
        ACSDK_DEBUG(LX("handleUpdateLocked")
                            .m("Removing scheduled update task")
                            .d("request", m_artifactRequest->getSummary()));
        m_scheduler.stop();
        return;
    }

    milliseconds nextChk = m_forcedUpdateInterval > seconds::zero()
                                   ? m_forcedUpdateInterval
                                   : max(duration_cast<milliseconds>(MIN_UPDATE_INTERVAL),
                                         duration_cast<milliseconds>(m_artifactExpiry - system_clock::now()));
    auto nextJitteredCheck = jitterUtil::jitter(nextChk, JITTER_FACTOR);
    ACSDK_DEBUG(LX("handleUpdateLocked")
                        .m("Scheduling another check")
                        .d("request", m_artifactRequest->getSummary())
                        .d("jitteredIntervalMS", nextJitteredCheck.count())
                        .d("baseMS", nextChk.count()));
    m_scheduler.stop();
    m_scheduler.start(nextJitteredCheck, Timer::PeriodType::RELATIVE, Timer::getForever(), [this]() {
        this->requestAndDownload(false);
    });
}

string DavsHandler::parseFileFromLink(const string& url, const string& defaultValue) {
    // handle s3 links
    if (url.find("amazonaws.com") != string::npos) {
        auto lastSlash = url.find_last_of('/');
        if (lastSlash != string::npos) {
            auto firstQuestion = url.find('?', lastSlash);
            auto size = (firstQuestion != string::npos) ? firstQuestion - lastSlash - 1 : string::npos;
            auto result = url.substr(lastSlash + 1, size);
            return result.empty() ? defaultValue : result;
        }
    }

    ACSDK_WARN(LX("parseFileFromLink").m("Couldn't parse a filename from the given url"));
    s_metrics().addCounter(METRIC_PREFIX_ERROR("parseFileFromLink")).addString("url", url);
    return defaultValue;
}

milliseconds DavsHandler::getBackOffTime(milliseconds prevBackOffTime) {
    // set the sleep to base backoff for first retry
    if (prevBackOffTime < m_baseBackOffTime) {
        return m_baseBackOffTime;
    } else if (prevBackOffTime >= m_maxBackOffTime) {
        return m_maxBackOffTime;
    } else {
        return jitterUtil::expJitter(prevBackOffTime, JITTER_FACTOR);
    }
}

void DavsHandler::runner(bool isUserInitiated) {
    WakeGuard guard(m_powerResource);
    FinallyGuard afterRun([this]() {
        lock_guard<mutex> lock(m_eventMutex);
        handleUpdateLocked();
        m_running.clear();
        filesystem::removeAll(getTmpParentDirectory());
    });
    shared_ptr<VendableArtifact> artifact;
    auto checkResult = checkWithRetryLoop(artifact);

    if (checkResult != ResultCode::SUCCESS || artifact == nullptr) {
        sendOnCheckFailure(checkResult);
        return;
    }

    m_artifactExpiry = artifact->getArtifactExpiry();
    // TODO this is insufficient when handling multiple requests
    auto availableSpace = filesystem::availableSpace(m_workingDirectory);
    auto artifactSize = artifact->getArtifactSizeBytes();
    if (m_unpack) {
        // Attempt to free up 1.5x the compressed file to ensure that we have enough space for the data.
        artifactSize *= 3 / 2;
    }
    auto spaceNeeded = availableSpace > artifactSize ? 0 : artifactSize - availableSpace;

    if (!sendCheckIfOkToDownload(artifact, spaceNeeded)) {
        return;
    }
    //    So we don't call download after multipart artifact is downloaded
    if (artifact->isMultipart()) {
        auto parentDirectory = getTmpParentDirectory();
        auto path = parentDirectory + "/" + artifact->getId();
        sendOnArtifactDownloaded(artifact, path);
        return;
    }
    downloadWithRetryLoop(artifact, isUserInitiated);
}

bool DavsHandler::waitForRetry(chrono::milliseconds& waitTime) {
    unique_lock<mutex> lock(m_eventMutex);
    m_eventTrigger.wait_for(lock, waitTime, [this] { return m_shutdown; });
    if (m_shutdown) {
        return false;
    }

    waitTime = getBackOffTime(waitTime);
    return true;
}

bool DavsHandler::waitForNetworkConnection() {
    unique_lock<mutex> lock(m_eventMutex);
    return m_eventTrigger.wait_for(lock, NETWORK_CONNECTION_WAIT_TIME, [this] { return m_networkConnected; });
}

ResultCode DavsHandler::checkWithRetryLoop(shared_ptr<VendableArtifact>& artifact) {
    auto result = ResultCode::CATASTROPHIC_FAILURE;
    auto timeToNextAttempt = m_firstBackOffTime;
    auto summary = m_artifactRequest->getSummary();

    if (!waitForNetworkConnection()) {
        ACSDK_WARN(LX("checkWithRetryLoop").m("No network connection, will attempt regardless"));
    }

    for (auto retryAttempt = 0; retryAttempt <= MAX_CHECK_RETRY && getChecker() != nullptr; ++retryAttempt) {
        // sleep with backoff starting at 0 and grow exponentially
        if (!waitForRetry(timeToNextAttempt)) {
            ACSDK_INFO(LX("checkWithRetryLoop").m("Cancelling check"));
            return result;
        }

        result = checkArtifact(artifact);

        if (result == ResultCode::SUCCESS) {
            ACSDK_DEBUG(LX("checkWithRetryLoop").m("Received a valid response").d("request", summary));
            s_metrics().addCounter("downloadCheckAttempt" + to_string(retryAttempt));
            return result;
        }

        if (result == ResultCode::NO_ARTIFACT_FOUND || result == ResultCode::FORBIDDEN) {
            ACSDK_ERROR(LX("checkWithRetryLoop").m("Didn't find artifact in DAVS").d("artifact", summary));
            return result;
        }

        if (result == ResultCode::CATASTROPHIC_FAILURE) {
            ACSDK_ERROR(LX("checkWithRetryLoop").m("Check utterly failed").d("request", summary));
            return result;
        }

        ACSDK_WARN(LX("checkWithRetryLoop").m("Check failed").d("artifact", summary).d("code", result));
        s_metrics().addCounter(METRIC_PREFIX_ERROR("checkFailedWithRetry"));
    }

    ACSDK_ERROR(LX("checkWithRetryLoop").m("Check failed for too long, giving up").d("artifact", summary));

    return result;
}

void DavsHandler::downloadWithRetryLoop(const shared_ptr<VendableArtifact>& artifact, bool isUserInitiated) {
    string path;
    auto timeToNextAttempt = m_firstBackOffTime;
    ResultCode downloadResult = ResultCode::CATASTROPHIC_FAILURE;
    FinallyGuard removeTmpPath([&path, this] {
        filesystem::removeAll(path);
        filesystem::removeAll(getTmpParentDirectory());
    });

    if (!sendOnStartAndCheckIfAvailable()) {
        return;
    }

    auto startTime = steady_clock::now();

    // Download has started, while running in a thread, will retry on failed attempts with back-off sleep
    for (auto retryAttempt = 0; retryAttempt <= MAX_DOWNLOAD_RETRY && getDownloader() != nullptr; ++retryAttempt) {
        // sleep with backoff starting at 0 and grow exponentially
        if (!waitForRetry(timeToNextAttempt)) {
            ACSDK_INFO(LX("downloadWithRetryLoop").m("Cancelling download"));
            return;
        }

        downloadResult = downloadArtifact(artifact, isUserInitiated, path);

        if (downloadResult == ResultCode::SUCCESS) {
            sendOnArtifactDownloaded(artifact, path);
            auto duration = max(milliseconds(1), duration_cast<milliseconds>(steady_clock::now() - startTime));
            auto rate = artifact->getArtifactSizeBytes() / BYTES_IN_KB * MS_IN_SEC / duration.count();
            s_metrics()
                    .addCounter("downloadAttempt" + to_string(retryAttempt))
                    .addCounter("downloadRateKBps", static_cast<int>(rate))
                    .addTimer("downloadTime", duration);
            return;
        }

        if (downloadResult == ResultCode::CATASTROPHIC_FAILURE) {
            sendOnDownloadFailure(downloadResult);
            return;
        }

        // Retry the download on all network failures
        ACSDK_WARN(LX("downloadWithRetryLoop")
                           .m("Check failed")
                           .d("request", m_artifactRequest->getSummary())
                           .d("code", static_cast<int>(downloadResult)));

        s_metrics().addCounter(METRIC_PREFIX_ERROR("downloadFailedWithRetry"));
    }

    ACSDK_ERROR(LX("downloadWithRetryLoop")
                        .m("Download failed for too long, giving up")
                        .d("request", m_artifactRequest->getSummary()));

    sendOnDownloadFailure(downloadResult);
}

void DavsHandler::cancel() {
    ACSDK_INFO(LX("cancel").m("DavsHandler: cancel requested"));
    unique_lock<mutex> lock(m_eventMutex);
    m_shutdown = true;
    lock.unlock();
    m_eventTrigger.notify_all();

    if (m_taskFuture.valid()) {
        m_taskFuture.wait();
    }
}

static inline bool isValidMimeType(const string& contentType) {
    ACSDK_DEBUG(LX("isValidMimeType").d("Content-Type", contentType));

    return contentType.find("application/json") != string::npos ||
           contentType.find("application/octet-stream") != string::npos ||
           contentType.find("multipart/mixed") != string::npos;
}
static inline bool isMultpart(const string& contentType) {
    return contentType.find("multipart/mixed") != string::npos;
}

ResultCode DavsHandler::checkArtifact(shared_ptr<VendableArtifact>& artifact) {
    stringstream response;
    string contentType;

    auto wrapper = CurlWrapper::create(false, m_authDelegate);
    if (wrapper == nullptr) {
        s_metrics().addCounter(METRIC_PREFIX_ERROR("checkArtifactFailed"));
        ACSDK_ERROR(LX("checkArtifact").m("Can't create CurlWrapper"));
        return ResultCode::ILLEGAL_ARGUMENT;
    }
    auto requestUrl = m_davsEndpointHandler->getDavsUrl(m_artifactRequest);
    if (requestUrl.empty()) {
        s_metrics().addCounter(METRIC_PREFIX_ERROR("checkArtifactFailed"));
        ACSDK_ERROR(LX("checkArtifact").m("Failed to generate a URL request"));
        return ResultCode::ILLEGAL_ARGUMENT;
    }
    auto headerResult = wrapper->getHeadersAuthorized(requestUrl);
    string headerString;
    if (headerResult.status() == ResultCode::SUCCESS) {
        headerString = headerResult.value();
        ACSDK_DEBUG(LX("checkArtifact").d("headerResponse", headerString));
    }
    contentType = CurlWrapper::getValueFromHeaders(headerString, "Content-Type");
    if (!headerString.empty() && !isValidMimeType(contentType)) {
        s_metrics().addCounter(METRIC_PREFIX_ERROR("checkArtifactFailed"));
        ACSDK_ERROR(LX("checkArtifact").m("Got back unhandled MIME Type from DAVS").d("content-type", contentType));
        return ResultCode::UNHANDLED_MIME_TYPE;
    }
    shared_ptr<VendableArtifact> responseArtifact;
    if (headerString.empty() || !isMultpart(contentType)) {
        auto resultCode = wrapper->get(requestUrl, response, shared_from_this());
        if (resultCode != ResultCode::SUCCESS) {
            s_metrics().addCounter(METRIC_PREFIX_ERROR("checkArtifactFailed"));
            ACSDK_ERROR(LX("checkArtifact").m("Failed to get response from DAVS"));

            return resultCode;
        }
        responseArtifact = VendableArtifact::create(m_artifactRequest, response.str());
    } else {
        shared_ptr<ResponseSink> sink = make_shared<ResponseSink>(m_artifactRequest, getTmpParentDirectory());
        auto resultCode = wrapper->getAndDownloadMultipart(requestUrl, sink, shared_from_this());
        if (resultCode != ResultCode::SUCCESS) {
            s_metrics().addCounter(METRIC_PREFIX_ERROR("checkArtifactFailed"));
            ACSDK_ERROR(LX("checkArtifact").m("Failed to get response from DAVS"));

            return resultCode;
        }
        responseArtifact = sink->getArtifact();
    }
    if (responseArtifact == nullptr) {
        s_metrics().addCounter(METRIC_PREFIX_ERROR("checkArtifactFailed"));
        ACSDK_ERROR(LX("checkArtifact").m("Failed to create Vendable Artifact"));

        return ResultCode::CATASTROPHIC_FAILURE;
    }

    artifact = move(responseArtifact);
    return ResultCode::SUCCESS;
}

ResultCode DavsHandler::downloadArtifact(
        const shared_ptr<VendableArtifact>& artifact,
        bool isUserInitiated,
        string& path) {
    if (artifact == nullptr) {
        return ResultCode::CATASTROPHIC_FAILURE;
    }

    auto currentTime = chrono::system_clock::now();
    if (artifact->getUrlExpiry() < currentTime) {
        s_metrics().addCounter(METRIC_PREFIX_ERROR("invalidUrlExpiryTime"));
        ACSDK_ERROR(LX("downloadArtifact")
                            .m("Invalid URL expiry time, Expiry time set before current time")
                            .d("expiry time", artifact->getUrlExpiry().time_since_epoch().count())
                            .d("current time", currentTime.time_since_epoch().count()));

        return ResultCode::ILLEGAL_ARGUMENT;
    }

    auto wrapper = CurlWrapper::create(!isUserInitiated && m_throttled, m_authDelegate);
    if (wrapper == nullptr) {
        ACSDK_ERROR(LX("downloadArtifact").m("Can't create CurlWrapper"));

        return ResultCode::ILLEGAL_ARGUMENT;
    }

    auto parentDirectory = getTmpParentDirectory();
    filesystem::makeDirectory(parentDirectory);
    if (!m_unpack) {
        path = parentDirectory + "/" + parseFileFromLink(artifact->getS3Url(), artifact->getId());
    } else {
        path = parentDirectory + "/" + "unpacked";
    }
    if (!filesystem::pathContainsPrefix(path, parentDirectory)) {
        // path wasn't under parentDirectory, link may have excessive directory traversal characters ../
        ACSDK_ERROR(LX("downloadArtifact").m("Invalid URL file path").d("path", path));

        return ResultCode::ILLEGAL_ARGUMENT;
    }

    if (m_unpack) {
        filesystem::makeDirectory(path);
    }
    auto downloadResult = wrapper->download(
            artifact->getS3Url(), path, shared_from_this(), m_unpack, artifact->getArtifactSizeBytes());

    if (downloadResult != ResultCode::SUCCESS) {
        s_metrics().addCounter(METRIC_PREFIX_ERROR("downloadArtifactFailed"));
        filesystem::removeAll(path);
        return downloadResult;
    }

    // clear user initiated flag for next attempt
    return ResultCode::SUCCESS;
}

bool DavsHandler::onProgressUpdate(long dlTotal, long dlNow, long ulTotal, long ulNow) {
    // Check whether cancel has been requested
    // Returning false will cause libcurl to abort the transfer and return
    lock_guard<mutex> lock(m_eventMutex);
    return (!m_shutdown);
}

bool DavsHandler::isThrottled() const {
    return m_throttled;
}

void DavsHandler::setThrottled(const bool throttle) {
    m_throttled = throttle;
}

void DavsHandler::setConnectionState(bool connected) {
    lock_guard<mutex> lock(m_eventMutex);
    m_networkConnected = connected;
    m_eventTrigger.notify_all();
}

}  // namespace davs
}  // namespace acsdkAssets
}  // namespace alexaClientSDK
