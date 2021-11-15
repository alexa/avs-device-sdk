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

#ifndef ACSDKASSETSCOMMON_CURLWRAPPER_H_
#define ACSDKASSETSCOMMON_CURLWRAPPER_H_

#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>
#include <AVSCommon/Utils/Error/Result.h>
#include <curl/curl.h>

#include <atomic>
#include <memory>
#include <ostream>
#include <regex>
#include <string>

#include "DownloadChunkQueue.h"
#include "DownloadStream.h"
#include "ResponseSink.h"
#include "acsdkAssetsCommon/CurlProgressCallbackInterface.h"
#include "acsdkAssetsInterfaces/ResultCode.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace common {

/**
 * Wraps the libcurl library in a more C++ friendly manner.  Still undecided if this will be generic or specific to
 * DAVS. Likely will be DAVS specific.
 */
class CurlWrapper {
public:
    // Destructor takes care of freeing the CURL handle
    virtual ~CurlWrapper();

    /**
     * Creates and initializes the object.
     * @param isThrottled whether the download should be slowed down for throttling
     * @param authDelegate the Authentication Delegate to generate the authentication token
     * @param certPath - optional - a path to a local SSL cert to use instead of maplite
     * @return a valid smart pointer to a valid object that can be used, or nullptr in case of error
     */
    static std::unique_ptr<CurlWrapper> create(
            bool isThrottled,
            std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
            const std::string& certPath = "");
    /**
     * Executes HTTP GET request and populates the response.
     * @param url IN, the URL to execute, must be a https.  It should include any query parameters as well.
     * @param response OUT, the ostream to populate with the HTTP entity-body; it may end up empty if entity-body is
     * empty or not present.
     * @param callbackObj object that implements CurlProgressCallbackInterface
     * @return ResultCode of the response.
     */
    commonInterfaces::ResultCode get(
            const std::string& url,
            std::ostream& response,
            const std::weak_ptr<CurlProgressCallbackInterface>& callbackObj);
    /**
     * Synchronously download a remote URL to local file or directory (if unpack is needed)
     * @param url the URL to download from
     * @param path the absolute path to a file (or directory) to write to (if unpack is needed)
     * @param callbackObj object that implements CurlProgressCallbackInterface
     * @param unpack whether unpack is needed during download, size must be specified or download would fail.
     * @param size size of the file to be downloaded, if not specified or set to 0, size check will be skipped. (except
     * when unpack=1)
     * @return SUCCESS if successfully downloaded
     * Note: if return value is false, the file may be partially written to.
     */
    commonInterfaces::ResultCode download(
            const std::string& url,
            const std::string& path,
            const std::weak_ptr<CurlProgressCallbackInterface>& callbackObj,
            bool unpack = false,
            size_t size = 0);

    /// Return status for header APIs
    using HeaderResults = avsCommon::utils::error::Result<commonInterfaces::ResultCode, std::string>;

    /**
     * Executes HTTP HEAD request and populates the response.
     * @param url IN, the URL to execute, must use https.  It should include any
     * query parameters as well.
     * @return the string populated with the HTTP Headers, or an empty string upon failure.
     */
    HeaderResults getHeaders(const std::string& url);
    /**
     * Executes HTTP HEAD request and populates the response. Authorizes to access davs headers
     * @param url IN, the URL to execute, must use https.  It should include any
     * query parameters as well.
     * @return the string populated with the HTTP Headers, or an empty string upon failure.
     */
    HeaderResults getHeadersAuthorized(const std::string& url);

    /**
     * Executes HTTP GET request which returns a multipart response streams the
     * data in chunks and downloads them to file.
     * @param url IN, the URL to execute, must be a https.  It should include any
     * query parameters as well.
     * @param sink OUT, object that is used by the multipart parser to read in the
     * data.
     * @param callbackObj object that implements CurlProgressCallbackInterface
     * @return ResultCode of the response.
     */
    commonInterfaces::ResultCode getAndDownloadMultipart(
            const std::string& url,
            std::shared_ptr<ResponseSink> sink,
            const std::weak_ptr<CurlProgressCallbackInterface>& callbackObj);

    static std::string getValueFromHeaders(const std::string& headers, const std::string& key);

private:
    explicit CurlWrapper(
            bool isThrottled,
            std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
            std::string certPath = "");

    // Initializes the object, returns true if successful.
    bool init();

    /**
     * Gets MAP Authentication header.
     * @param header the string to populate
     * @return true if managed to read token from MAP
     */
    bool getAuthorizationHeader(std::string& header);

    /**
     * Streams the HTTP response into a given stream.
     * @param fullUrl the URL to download via GET
     * @param responseStream where to stream response
     * @param callbackObj object that implements CurlProgressCallbackInterface
     * @return SUCCESS if the data transfer completed successfully
     * Note: if return value is false, the stream may be partially written to.
     */
    commonInterfaces::ResultCode stream(
            const std::string& fullUrl,
            std::ostream& responseStream,
            const std::weak_ptr<CurlProgressCallbackInterface>& callbackObj);
    /**
     * Streams the HTTP response into a given file.
     * @param downloadUrl the URL to download via GET
     * @param filePath file path for the downloaded file
     * @param size expected size of the file to be downloaded
     * @param callbackObj object that implements CurlProgressCallbackInterface
     * @return SUCCESS if the data transfer completed successfully
     * Note: if return value is false, the stream may be partially written to.
     */
    commonInterfaces::ResultCode streamToFile(
            const std::string& downloadUrl,
            const std::string& filePath,
            size_t size,
            const std::weak_ptr<CurlProgressCallbackInterface>& callbackObj);

    /**
     * Streams the HTTP response into download chunk queue
     * @param fullUrl the URL to download via GET
     * @param downloadChunkQueue queue to hold downloaded data chunks
     * @param callbackObj object that implements CurlProgressCallbackInterface
     * @return SUCCESS if the data transfer completed successfully
     */
    commonInterfaces::ResultCode streamToQueue(
            const std::string& fullUrl,
            std::shared_ptr<DownloadChunkQueue> downloadChunkQueue,
            const std::weak_ptr<CurlProgressCallbackInterface>& callbackObj);

    /**
     * Unpack downloaded chunks from download chunk queue
     * @param downloadChunkQueue queue to hold downloaded data chunks
     * @param path parent directory of unpacked files
     * @return true if unpack (and download) completed successfully
     */
    bool unpack(std::shared_ptr<DownloadChunkQueue> downloadChunkQueue, const std::string& path);

    /**
     * Check HTTP return code for the previous curl operation, and map the HTTP response code to ResultCode
     * @param logError true if an error should be logged and a metric created, false if errors can be ignored
     * @return HTTP response code mapped to ResultCode
     */
    commonInterfaces::ResultCode checkHTTPStatusCode(bool logError = true);

    /**
     * Calls getAuthorizationHeader and sets the httpHeader curl option.
     * @return the result code for the ability to set the HTTP Header
     */
    commonInterfaces::ResultCode setHTTPHEADER();

private:
    bool m_isThrottled;
    std::string m_certPath;
    CURLcode m_code;
    CURL* m_handle;
    char m_errorBuffer[CURL_ERROR_SIZE]{};
    std::string m_header;
    // the curl handle does not take ownership of the memory in headers, and the handle re-uses headers
    std::unique_ptr<curl_slist, void (*)(curl_slist*)> m_headers;
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthDelegateInterface> m_authDelegate;
};
}  // namespace common
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKASSETSCOMMON_CURLWRAPPER_H_
