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

#ifndef ACSDKDAVSCLIENT_DAVSHANDLER_H_
#define ACSDKDAVSCLIENT_DAVSHANDLER_H_

#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Power/PowerResource.h>
#include <AVSCommon/Utils/Timing/Timer.h>

#include <chrono>
#include <future>
#include <mutex>

#include "acsdkAssetsCommon/AmdMetricWrapper.h"
#include "acsdkAssetsCommon/CurlProgressCallbackInterface.h"
#include "acsdkAssetsInterfaces/VendableArtifact.h"
#include "acsdkDavsClientInterfaces/ArtifactHandlerInterface.h"
#include "acsdkDavsClientInterfaces/DavsEndpointHandlerInterface.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace davs {

class DavsHandler
        : public common::CurlProgressCallbackInterface
        , public std::enable_shared_from_this<DavsHandler> {
public:
    /**
     * Creates a Handler that will take care of the check and download requests from Davs.
     *
     * @param artifactRequest REQUIRED, a valid request containing information for the artifact to be downloaded.
     * @param downloadCallback REQUIRED, a listener that will handle what to do with the artifact when its downloaded or
     * failed.
     * @param checkCallback REQUIRED, a listener that will handle checking if the artifact should be downloaded.
     * @param baseBackOffTime REQUIRED, the starting backoff time we should wait after a first-time failure
     * @param maxBackOffTime REQUIRED,the maximum value of back-off we can tolerate
     * @param authDelegate REQUIRED, the Authentication Delegate to generate the authentication token
     * @param davsEndpointHandler REQUIRED, the endpoint handler which is used to generate the proper DAVS request URL.
     * @param powerResource OPTIONAL, if provided, then the PowerResource will be used to acquire the wakelock
     * @param forcedUpdateInterval OPTIONAL, sets the update interval for all artifact to a specific value. This is to
     * be use only for testing! your devices will be throttled if used in production!
     * @return a smart pointer to Davs Handler.
     */
    static std::shared_ptr<DavsHandler> create(
            std::shared_ptr<commonInterfaces::DavsRequest> artifactRequest,
            const std::shared_ptr<davsInterfaces::DavsDownloadCallbackInterface>& downloadRequester,
            const std::shared_ptr<davsInterfaces::DavsCheckCallbackInterface>& checkRequester,
            std::string workingDirectory,
            std::chrono::milliseconds baseBackOffTime,
            std::chrono::milliseconds maxBackOffTime,
            std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
            std::shared_ptr<davsInterfaces::DavsEndpointHandlerInterface> davsEndpointHandler,
            std::shared_ptr<alexaClientSDK::avsCommon::utils::power::PowerResource> powerResource = nullptr,
            std::chrono::seconds forcedUpdateInterval = std::chrono::seconds(0));
    /**
     * Destructor to terminate the thread.
     */
    ~DavsHandler() override;

    /**
     * @return if the listeners are still alive.
     */
    inline bool isRelevant() {
        return !m_checkRequester.expired() && !m_downloadRequester.expired();
    }

    /**
     * Performs a check and download operation to get and store the artifact.
     * @param isUserInitiated whether download is initiated by user, throttle or not
     */
    void requestAndDownload(bool isUserInitiated);

    /**
     * Cancel the current download/request and clean up.
     */
    void cancel();

    /**
     * Enables or disables checking for updates when the artifact's TTL is done.
     * @param enable whether update is enabled when the artifact's TTL is done
     */
    inline void enableUpdate(bool enable) {
        std::lock_guard<std::mutex> lock(m_eventMutex);
        m_updateEnabled = enable;
        handleUpdateLocked();
    }

    /**
     * If an artifact enabled download to perform update
     * @param None
     */
    inline bool isUpdateEnabled() {
        return m_updateEnabled;
    }

    /***
     * Expose the inside ArtifactRequest containing type and key to DavsClient check then update
     * @return ArtifactRequest
     */
    inline std::shared_ptr<commonInterfaces::DavsRequest> getDavsRequest() {
        return m_artifactRequest;
    }

    /**
     * If the first download needs to back off, set this. Otherwise its default is 0ms;
     */
    inline void setFirstBackOff(std::chrono::milliseconds firstBackOffTime) {
        m_firstBackOffTime = firstBackOffTime;
    }

    /**
     * Gets an exponentially growing time to backoff before another download
     * @param prevBackOffTime the backoff time resulting in current failure
     * @return
     */
    std::chrono::milliseconds getBackOffTime(std::chrono::milliseconds prevBackOffTime);

    /**
     * Takes an link and attempts to parse out the file name being provided.
     *
     * @param url to be parsed.
     * @param defaultValue that will be returned if the parsing fails.
     * @return filename if the parsing was successful, defaultValue otherwise.
     */
    static std::string parseFileFromLink(const std::string& url, const std::string& defaultValue);

    bool isThrottled() const;

    void setThrottled(bool throttle);

    void setConnectionState(bool connected);

private:
    DavsHandler(
            std::shared_ptr<commonInterfaces::DavsRequest> artifactRequest,
            const std::shared_ptr<davsInterfaces::DavsDownloadCallbackInterface>& downloadRequester,
            const std::shared_ptr<davsInterfaces::DavsCheckCallbackInterface>& checkRequester,
            std::string workingDirectory,
            std::chrono::milliseconds baseBackOffTime,
            std::chrono::milliseconds maxBackOffTime,
            std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
            std::shared_ptr<davsInterfaces::DavsEndpointHandlerInterface> davsEndpointHandler,
            std::shared_ptr<alexaClientSDK::avsCommon::utils::power::PowerResource> powerResource,
            std::chrono::seconds forcedUpdateInterval);

    /**
     * Main runner thread to handle the bulk of the logic.
     * @param isUserInitiated whether download is initiated by user, throttle or not
     */
    void runner(bool isUserInitiated);

    /**
     * Performs the GetArtifact request to get the artifact metadata.
     *
     * @param artifact OUT, information retrieved from DAVS if the call is successful.
     * @return Specific result code of the call.
     */
    commonInterfaces::ResultCode checkArtifact(std::shared_ptr<commonInterfaces::VendableArtifact>& artifact);

    /**
     * Performs the file download request and handles writing to disk.
     *
     * @param artifact REQUIRED, artifact to download given s3url and expiry.
     * @param isUserInitiated whether the download is an update from device or request from user
     * @param path OUT, path where the file is downloaded.
     * @return Specific result code of the call.
     */
    commonInterfaces::ResultCode downloadArtifact(
            const std::shared_ptr<commonInterfaces::VendableArtifact>& artifact,
            bool isUserInitiated,
            std::string& path);
    /**
     * Inside a runner, starts checking attempts with retries and exists if all attempts fails or the check succeeds.
     *
     * @param artifact OUT, information retrieved from DAVS if the call is successful.
     * @return Specific result code of the call.
     */
    commonInterfaces::ResultCode checkWithRetryLoop(std::shared_ptr<commonInterfaces::VendableArtifact>& artifact);

    /**
     * Inside a runner, starts download attempts with retries and exists if all attempts fails or there's a success
     *
     * @param artifact the object to download
     * @param isUserInitiated whether the download is an update from device or request from user
     */
    void downloadWithRetryLoop(
            const std::shared_ptr<commonInterfaces::VendableArtifact>& artifact,
            bool isUserInitiated);

    /**
     * Waits for a given amount of time or until shutdown is set. If shutdown is set, then return false, otherwise
     * return true. This also adjusts the given waitTime for the next time that it is called.
     *
     * @param waitTime time to wait, will be adjusted according to retry logic.
     * @return if we should continue or not.
     */
    bool waitForRetry(std::chrono::milliseconds& waitTime);

    /**
     * Waits for a wifi network to be connected until a certain timeout.
     *
     * @return if the network is connected or not.
     */
    bool waitForNetworkConnection();

    /**
     * Checks to see if we need to schedule an update or not.
     */
    void handleUpdateLocked();

    /**
     * @return the temporary directory for downloading artifacts
     */
    inline std::string getTmpParentDirectory() const {
        return m_workingDirectory + "/" + m_artifactRequest->getSummary();
    }

    inline std::shared_ptr<davsInterfaces::DavsCheckCallbackInterface> getChecker() {
        auto checker = m_checkRequester.lock();
        if (checker == nullptr) {
            ACSDK_WARN(alexaClientSDK::avsCommon::utils::logger::LogEntry("DavsHandler", "getChecker")
                               .m("Check requester is no longer available")
                               .d("request", m_artifactRequest->getSummary()));
            s_metrics().addCounter(METRIC_PREFIX_ERROR("checkRequesterNotAvailable"));
            return nullptr;
        }
        return checker;
    }

    inline std::shared_ptr<davsInterfaces::DavsDownloadCallbackInterface> getDownloader() {
        auto downloader = m_downloadRequester.lock();
        if (downloader == nullptr) {
            ACSDK_WARN(alexaClientSDK::avsCommon::utils::logger::LogEntry("DavsHandler", "getDownloader")
                               .m("Download requester is no longer available")
                               .d("request", m_artifactRequest->getSummary()));
            s_metrics().addCounter(METRIC_PREFIX_ERROR("downloadRequesterNotAvailable"));
            return nullptr;
        }
        return downloader;
    }

    inline void sendOnCheckFailure(commonInterfaces::ResultCode resultCode) {
        auto checker = getChecker();
        if (checker == nullptr) {
            return;
        }
        ACSDK_ERROR(alexaClientSDK::avsCommon::utils::logger::LogEntry("DavsHandler", "sendOnCheckFailure")
                            .m("Check failed")
                            .d("request", m_artifactRequest->getSummary()));
        s_metrics().addZeroCounter("downloadCheckSuccess").addCounter(METRIC_PREFIX_ERROR("downloadCheckFailed"));
        checker->onCheckFailure(resultCode);
    }

    inline bool sendCheckIfOkToDownload(
            const std::shared_ptr<commonInterfaces::VendableArtifact>& artifact,
            size_t spaceNeeded) {
        auto checker = getChecker();
        if (checker == nullptr) {
            return false;
        }
        s_metrics().addCounter("downloadCheckSuccess");
        if (!checker->checkIfOkToDownload(artifact, spaceNeeded)) {
            ACSDK_WARN(alexaClientSDK::avsCommon::utils::logger::LogEntry("DavsHandler", "sendCheckIfOkToDownload")
                               .m("Requester rejected download")
                               .d("request", m_artifactRequest->getSummary()));
            s_metrics().addCounter("downloadRejected");
            return false;
        }
        return true;
    }

    inline bool sendOnStartAndCheckIfAvailable() {
        auto downloader = getDownloader();
        if (downloader == nullptr) {
            return false;
        }
        ACSDK_INFO(alexaClientSDK::avsCommon::utils::logger::LogEntry("DavsHandler", "sendOnStartAndCheckIfAvailable")
                           .m("Download started")
                           .d("request", m_artifactRequest->getSummary()));
        downloader->onStart();
        return true;
    }

    inline void sendOnDownloadFailure(commonInterfaces::ResultCode resultCode) {
        auto downloader = getDownloader();
        if (downloader == nullptr) {
            return;
        }
        ACSDK_ERROR(alexaClientSDK::avsCommon::utils::logger::LogEntry("DavsHandler", "sendOnDownloadFailure")
                            .m("Download utterly failed")
                            .d("request", m_artifactRequest->getSummary()));
        s_metrics().addZeroCounter("downloadSuccess").addCounter(METRIC_PREFIX_ERROR("downloadFailed"));
        downloader->onDownloadFailure(resultCode);
    }

    inline void sendOnArtifactDownloaded(
            const std::shared_ptr<commonInterfaces::VendableArtifact>& artifact,
            const std::string& path) {
        auto downloader = m_downloadRequester.lock();
        if (downloader == nullptr) {
            return;
        }
        ACSDK_INFO(alexaClientSDK::avsCommon::utils::logger::LogEntry("DavsHandler", "sendOnArtifactDownloaded")
                           .m("Download succeeded")
                           .d("request", m_artifactRequest->getSummary()));
        s_metrics().addCounter("downloadSuccess");
        downloader->onArtifactDownloaded(artifact, path);
    }

    /// @name CurlProgressCallbackInterface Functions
    /// @{
    bool onProgressUpdate(long dlTotal, long dlNow, long ulTotal, long ulNow) override;
    /// @}
private:
    static const std::function<common::AmdMetricsWrapper()> s_metrics;

    const std::shared_ptr<commonInterfaces::DavsRequest> m_artifactRequest;
    const std::weak_ptr<davsInterfaces::DavsDownloadCallbackInterface> m_downloadRequester;
    const std::weak_ptr<davsInterfaces::DavsCheckCallbackInterface> m_checkRequester;
    const std::string m_workingDirectory;

    // controls retry logic and how much to backoff
    const std::chrono::milliseconds m_baseBackOffTime;
    const std::chrono::milliseconds m_maxBackOffTime;
    /// AuthDelegate that curlWrapper will use to get the Authentication Token
    const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthDelegateInterface> m_authDelegate;
    /// Endpoint handler which is used to generate the proper DAVS request URL.
    const std::shared_ptr<davsInterfaces::DavsEndpointHandlerInterface> m_davsEndpointHandler;
    /// PowerResource used to acquire/release the wakelock
    std::shared_ptr<alexaClientSDK::avsCommon::utils::power::PowerResource> m_powerResource;
    /// If set above 0, then this interval will be used to override the update polling interval dicated by the cloud TTL
    const std::chrono::seconds m_forcedUpdateInterval;
    std::chrono::milliseconds m_firstBackOffTime;

    // whether cancel request has been issued during DAVS download
    std::mutex m_eventMutex;
    std::condition_variable m_eventTrigger;
    bool m_shutdown;
    std::atomic_flag m_running;
    std::future<void> m_taskFuture;
    alexaClientSDK::avsCommon::utils::timing::Timer m_scheduler;

    bool m_updateEnabled;
    commonInterfaces::VendableArtifact::TimeEpoch m_artifactExpiry;

    bool m_throttled;
    bool m_networkConnected;

    /// whether the artifact needs to be unpacked after download
    bool m_unpack;
};

}  // namespace davs
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKDAVSCLIENT_DAVSHANDLER_H_
