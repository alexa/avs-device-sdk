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

#include "acsdkAssetsCommon/CurlWrapper.h"

#include <AVSCommon/Utils/Error/FinallyGuard.h>
#include <AVSCommon/Utils/FileSystem/FileSystemUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/String/StringUtils.h>
#include <archive.h>
#include <archive_entry.h>

#include <string>
#include <thread>
#include <utility>

#include "acsdkAssetsCommon/AmdMetricWrapper.h"
#include "acsdkAssetsCommon/ArchiveWrapper.h"
#include "acsdkAssetsCommon/ResponseSink.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace common {

using namespace std;
using namespace chrono;
using namespace commonInterfaces;
using namespace alexaClientSDK::avsCommon::utils::filesystem;
using namespace alexaClientSDK::avsCommon::utils::string;
using namespace alexaClientSDK::avsCommon::utils::error;

using headers_ptr = unique_ptr<curl_slist, void (*)(curl_slist*)>;

static const auto s_metrics = AmdMetricsWrapper::creator("curlWrapper");

static const long HTTP_SERVER_ERROR = 500;

static const curl_off_t THROTTLED_SPEED_KB = 256 * 1024 / 8;  // 256 Kbits;

/// Number of data chunks in download (buffering) queue before we attempt to slow down download
static const size_t DOWNLOAD_QUEUE_SIZE_THRESHOLD = 50;

/// String to identify log entries originating from this file.
static const std::string TAG{"CurlWrapper"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// This is needed to get around some compiler errors in MinGW around using curl functions statically
static void curlSlistFreeAllDelegate(struct curl_slist* curSlist) {
    curl_slist_free_all(curSlist);
}

string CurlWrapper::getValueFromHeaders(const std::string& headers, const std::string& key) {
    auto keyLower = stringToLowerCase(key);
    istringstream headerStream(headers);
    string line;

    while (getline(headerStream, line)) {
        if (stringToLowerCase(line).find(keyLower) == string::npos) {
            continue;
        }
        auto separatorIndex = line.find(':');
        if (separatorIndex == string::npos) {
            continue;
        }
        line.erase(line.find_last_not_of(" \n\r\t") + 1);
        return ltrim(line.substr(separatorIndex + 1));
    }

    return "";
}

CurlWrapper::CurlWrapper(
        bool throttled,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
        string certPath) :
        m_isThrottled(throttled),
        m_certPath(move(certPath)),
        m_code(CURLE_OK),
        m_handle(nullptr),
        m_headers(unique_ptr<curl_slist, void (*)(curl_slist*)>(nullptr, curlSlistFreeAllDelegate)),
        m_authDelegate(move(authDelegate)) {
}

CurlWrapper::~CurlWrapper() {
    curl_easy_cleanup(m_handle);
}

bool CurlWrapper::init() {
    m_handle = curl_easy_init();
    if (!m_handle) return false;

    if (m_isThrottled) {
        if ((m_code = curl_easy_setopt(m_handle, CURLOPT_MAX_SEND_SPEED_LARGE, THROTTLED_SPEED_KB))) return false;
        if ((m_code = curl_easy_setopt(m_handle, CURLOPT_MAX_RECV_SPEED_LARGE, THROTTLED_SPEED_KB))) return false;
    }

    // allow up to 10 redirects
    if ((m_code = curl_easy_setopt(m_handle, CURLOPT_MAXREDIRS, 10L))) return false;
    // set a speed timeout to close connections if no data is transferred within 20 seconds
    if ((m_code = curl_easy_setopt(m_handle, CURLOPT_LOW_SPEED_LIMIT, 1))) return false;
    if ((m_code = curl_easy_setopt(m_handle, CURLOPT_LOW_SPEED_TIME, 20))) return false;
    // Setting the connection timeout to 30 seconds
    if ((m_code = curl_easy_setopt(m_handle, CURLOPT_CONNECTTIMEOUT, 30))) return false;
    // follow "Location:" headers
    if ((m_code = curl_easy_setopt(m_handle, CURLOPT_FOLLOWLOCATION, 1))) return false;
    // setup an error buffer
    if ((m_code = curl_easy_setopt(m_handle, CURLOPT_ERRORBUFFER, m_errorBuffer))) return false;
    // verify host
    if ((m_code = curl_easy_setopt(m_handle, CURLOPT_SSL_VERIFYHOST, 2L))) return false;
    // verify peer
    if ((m_code = curl_easy_setopt(m_handle, CURLOPT_SSL_VERIFYPEER, 1L))) return false;
    // Enable communication using TLS1.0 or later.  CURL_SSLVERSION_TLSv1_3 is available but seems new; Anvil risk
    // opened for confirmation.
    if ((m_code = curl_easy_setopt(m_handle, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2))) return false;

    if (!m_certPath.empty()) {
        ACSDK_DEBUG(LX("init").m("Using custom cert and to not verify host or peers").d("cert Path", m_certPath));
        if ((m_code = curl_easy_setopt(m_handle, CURLOPT_CAINFO, m_certPath.c_str()))) return false;
        // our cert is self-signed and is based on no known CA-Author
        if ((m_code = curl_easy_setopt(m_handle, CURLOPT_SSL_VERIFYPEER, 0L))) return false;
        if ((m_code = curl_easy_setopt(m_handle, CURLOPT_SSL_VERIFYHOST, 0L))) return false;
    }

    return true;
}

unique_ptr<CurlWrapper> CurlWrapper::create(
        bool isThrottled,
        shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
        const string& certPath) {
    if (authDelegate == nullptr && certPath.empty()) {
        ACSDK_ERROR(LX("create").m("Failed to initialize").d("error", "authDelegate is null and cert path is empty"));
        return nullptr;
    }

    auto wrapper = unique_ptr<CurlWrapper>(new CurlWrapper(isThrottled, authDelegate, certPath));

    if (!wrapper->init()) {
        ACSDK_ERROR(LX("create").m("Failed to initialize").d("error", wrapper->m_code));
        return nullptr;
    }

    return wrapper;
}

bool CurlWrapper::getAuthorizationHeader(string& header) {
    if (!m_authDelegate) {
        ACSDK_ERROR(LX("getAuthorizationHeader").m("Failed to set HTTPHEADER, null authDelegate"));
        return false;
    }
    auto token = m_authDelegate->getAuthToken();
    header = "Authorization: Bearer " + token;

    return true;
}

extern "C" {
typedef size_t (*CURL_WRITE_CALLBACK)(char* ptr, size_t size, size_t nmemb, void* userdata);
typedef int (*CURL_XFERINFOFUNCTION_CALLBACK)(
        void* userdata,
        curl_off_t dltotal,
        curl_off_t dlnow,
        curl_off_t ultotal,
        curl_off_t ulnow);
}

ResultCode CurlWrapper::get(
        const string& url,
        ostream& response,
        const weak_ptr<CurlProgressCallbackInterface>& callbackObj) {
    auto resultCode = setHTTPHEADER();
    if (resultCode != ResultCode::SUCCESS) {
        ACSDK_ERROR(LX("get").m("Failed to set HTTPHEADER"));
        return resultCode;
    }
    resultCode = stream(url, response, callbackObj);

    return resultCode;
}

ResultCode CurlWrapper::checkHTTPStatusCode(bool logError) {
    long httpStatusCode;
    if ((m_code = curl_easy_getinfo(m_handle, CURLINFO_RESPONSE_CODE, &httpStatusCode))) {
        return ResultCode::CONNECTION_FAILED;
    }
    if (httpStatusCode == static_cast<int>(ResultCode::SUCCESS)) {
        return ResultCode::SUCCESS;
    }

    if (logError) {
        ACSDK_ERROR(LX("checkHTTPStatusCode").d("HTTP returned code", httpStatusCode));
        s_metrics().addCounter("httpResponse_" + to_string(httpStatusCode));
    }

    switch (httpStatusCode) {
        case static_cast<int>(ResultCode::ILLEGAL_ARGUMENT):
            return ResultCode::ILLEGAL_ARGUMENT;
        case static_cast<int>(ResultCode::NO_ARTIFACT_FOUND):
            return ResultCode::NO_ARTIFACT_FOUND;
        case static_cast<int>(ResultCode::UNAUTHORIZED):
            return ResultCode::UNAUTHORIZED;
        case static_cast<int>(ResultCode::FORBIDDEN):
            return ResultCode::FORBIDDEN;
        case HTTP_SERVER_ERROR:
            return ResultCode::CONNECTION_FAILED;
        default:
            return ResultCode::CATASTROPHIC_FAILURE;
    }
}

ResultCode CurlWrapper::setHTTPHEADER() {
    if (!m_certPath.empty()) {
        ACSDK_INFO(LX("setHTTPHEADER").m("Using custom cert. Skipping attaching other auth-headers"));
        return ResultCode::SUCCESS;
    }
    if (!getAuthorizationHeader(m_header)) {
        return ResultCode::CONNECTION_FAILED;
    }
    m_headers = headers_ptr(curl_slist_append(nullptr, m_header.c_str()), curlSlistFreeAllDelegate);
    if (m_headers == nullptr) {
        s_metrics().addCounter("noAuthHeader");
        ACSDK_ERROR(LX("setHTTPHEADER").m("Can't append authorization header"));
        return ResultCode::CONNECTION_FAILED;
    }

    m_code = curl_easy_setopt(m_handle, CURLOPT_HTTPHEADER, m_headers.get());
    if (m_code != CURLE_OK) {
        s_metrics().addCounter("noAuthHeader");
        ACSDK_ERROR(LX("setHTTPHEADER").m("Failed to setopt the headers").d("code", m_code));
        return ResultCode::CONNECTION_FAILED;
    }
    return ResultCode::SUCCESS;
}

CurlWrapper::HeaderResults CurlWrapper::getHeadersAuthorized(const string& url) {
    auto results = setHTTPHEADER();
    if (results != ResultCode::SUCCESS) {
        ACSDK_ERROR(LX("getHeadersAuthorized").m("Couldn't set up HTTP Headers, won't be able to access infomation"));
        return {results};
    }
    return getHeaders(url);
}

CurlWrapper::HeaderResults CurlWrapper::getHeaders(const string& url) {
    auto writeFunction = [](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
        auto output = static_cast<ostream*>(userdata);
        output->write(ptr, size * nmemb);
        if (output->good()) {
            return nmemb;
        } else {
            // we could use ftell() to figure out how much is written, but no need since anything < nmemb is a failure
            return 0;
        }
    };

    if ((m_code = curl_easy_setopt(m_handle, CURLOPT_URL, url.c_str()))) {
        return {ResultCode::CONNECTION_FAILED};
    };

    if ((m_code = curl_easy_setopt(m_handle, CURLOPT_HEADER, 1))) {
        return {ResultCode::CONNECTION_FAILED};
    };
    if ((m_code = curl_easy_setopt(m_handle, CURLOPT_NOBODY, 1))) {
        curl_easy_setopt(m_handle, CURLOPT_HEADER, 0);
        return {ResultCode::CONNECTION_FAILED};
    };

    if ((m_code = curl_easy_setopt(m_handle, CURLOPT_WRITEFUNCTION, static_cast<CURL_WRITE_CALLBACK>(writeFunction)))) {
        curl_easy_setopt(m_handle, CURLOPT_HEADER, 0);
        curl_easy_setopt(m_handle, CURLOPT_NOBODY, 0);
        return {ResultCode::CONNECTION_FAILED};
    };

    stringstream headerStream;
    if ((m_code = curl_easy_setopt(m_handle, CURLOPT_WRITEDATA, &headerStream))) {
        curl_easy_setopt(m_handle, CURLOPT_HEADER, 0);
        curl_easy_setopt(m_handle, CURLOPT_NOBODY, 0);
        return {ResultCode::CONNECTION_FAILED};
    };

    m_code = curl_easy_perform(m_handle);
    curl_easy_setopt(m_handle, CURLOPT_HEADER, 0);
    curl_easy_setopt(m_handle, CURLOPT_NOBODY, 0);
    if (m_code != CURLE_OK) {
        return {ResultCode::CONNECTION_FAILED};
    }
    return {checkHTTPStatusCode(false), headerStream.str()};
}

ResultCode CurlWrapper::stream(
        const string& fullUrl,
        ostream& responseStream,
        const weak_ptr<CurlProgressCallbackInterface>&) {
    ACSDK_INFO(LX("stream").m("Starting stream request"));

    // Curl callback function to write downloaded data chunk
    auto writeFunction = [](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
        auto output = static_cast<ostream*>(userdata);
        output->write(ptr, size * nmemb);
        if (output->good()) {
            return nmemb;
        } else {
            // we could use ftell() to figure out how much is written, but no need since anything < nmemb is a failure
            return 0;
        }
    };
    if ((m_code = curl_easy_setopt(m_handle, CURLOPT_WRITEDATA, &responseStream))) {
        return ResultCode::CONNECTION_FAILED;
    }
    if ((m_code = curl_easy_setopt(m_handle, CURLOPT_WRITEFUNCTION, static_cast<CURL_WRITE_CALLBACK>(writeFunction)))) {
        return ResultCode::CONNECTION_FAILED;
    }

    ACSDK_DEBUG9(LX("stream").m("Getting truncated url").d("url", fullUrl.substr(0, 100).c_str()));
    if ((m_code = curl_easy_setopt(m_handle, CURLOPT_URL, fullUrl.c_str()))) {
        return ResultCode::CONNECTION_FAILED;
    }

    m_code = curl_easy_perform(m_handle);
    if (m_code != CURLE_OK) {
        if (m_code == CURLE_ABORTED_BY_CALLBACK) {
            // cancelled by user, do not retry
            return ResultCode::CATASTROPHIC_FAILURE;
        } else {
            return ResultCode::CONNECTION_FAILED;
        }
    }

    return checkHTTPStatusCode();
}

ResultCode CurlWrapper::streamToFile(
        const std::string& fullUrl,
        const std::string& path,
        size_t size,
        const weak_ptr<CurlProgressCallbackInterface>&) {
    auto downloadStream = DownloadStream::create(path, size);
    if (downloadStream == nullptr) {
        ACSDK_ERROR(LX("streamToFile").m("fileStream is evil").d("path", path.c_str()));
        s_metrics().addCounter("evilFileStream");
        return ResultCode::CATASTROPHIC_FAILURE;
    }
    ACSDK_INFO(LX("streamToFile").m("Downloading to").d("path", path.c_str()));

    // Curl callback function to write downloaded data chunk
    auto writeFunction = [](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
        auto output = static_cast<DownloadStream*>(userdata);
        if (output->write(ptr, size * nmemb)) {
            return nmemb;
        } else {
            return 0;  // anything < nmemb is a failure
        }
    };
    if ((m_code = curl_easy_setopt(m_handle, CURLOPT_WRITEDATA, downloadStream.get()))) {
        return ResultCode::CONNECTION_FAILED;
    }
    if ((m_code = curl_easy_setopt(m_handle, CURLOPT_WRITEFUNCTION, static_cast<CURL_WRITE_CALLBACK>(writeFunction)))) {
        return ResultCode::CONNECTION_FAILED;
    }

    if ((m_code = curl_easy_setopt(m_handle, CURLOPT_URL, fullUrl.c_str()))) {
        return ResultCode::CONNECTION_FAILED;
    }

    m_code = curl_easy_perform(m_handle);
    if (m_code != CURLE_OK) {
        if (m_code == CURLE_ABORTED_BY_CALLBACK) {
            // cancelled by user, do not retry
            return ResultCode::CATASTROPHIC_FAILURE;
        } else {
            return ResultCode::CONNECTION_FAILED;
        }
    }
    if (!downloadStream->downloadSucceeded()) {
        return ResultCode::CHECKSUM_MISMATCH;
    }
    if (!changePermissions(path, DEFAULT_FILE_PERMISSIONS)) {
        ACSDK_ERROR(LX("streamToFile").m("Failed to set DEFAULT_FILE_PERMISSIONS").d("path", path.c_str()));
        return ResultCode::CATASTROPHIC_FAILURE;
    }
    return checkHTTPStatusCode();
}

ResultCode CurlWrapper::streamToQueue(
        const string& fullUrl,
        shared_ptr<DownloadChunkQueue> downloadChunkQueue,
        const weak_ptr<CurlProgressCallbackInterface>&) {
    ACSDK_INFO(LX("streamToQueue").m("Starting streamToQueue request"));
    auto resultCode = ResultCode::SUCCESS;

    FinallyGuard afterRun([&]() { downloadChunkQueue->pushComplete(resultCode == ResultCode::SUCCESS); });

    // Curl callback function to write downloaded data chunk
    auto curlWriteCallback = [](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
        auto queuePtr = static_cast<DownloadChunkQueue*>(userdata);
        if (queuePtr == nullptr) {
            return 0;
        }
        auto numBytes = size * nmemb;
        if (queuePtr->push(ptr, numBytes)) {
            auto queueSize = queuePtr->size();
            if (queueSize > DOWNLOAD_QUEUE_SIZE_THRESHOLD * 2) {
                ACSDK_ERROR(LX("curlWriteCallback").m("QueueSize too big, abort download.").d("QueueSize", queueSize));
                s_metrics().addCounter("UnpackingStalled");
                return 0;
            } else if (queueSize > DOWNLOAD_QUEUE_SIZE_THRESHOLD) {
                // usually download speed is slower than unpack speed, when this is no longer the case,
                // we slow down the download so as not to increase RAM consumption unnecessarily.
                // Each dataChunk is up to 16k.
                ACSDK_INFO(LX("curlWriteCallback").m("Slowing down download").d("queue size", queueSize));
                this_thread::sleep_for(milliseconds(10) * queueSize);
            }
            return nmemb;
        } else {
            return 0;
        }
    };

    if ((m_code = curl_easy_setopt(m_handle, CURLOPT_WRITEDATA, downloadChunkQueue.get()))) {
        resultCode = ResultCode::CONNECTION_FAILED;
        return resultCode;
    }

    if ((m_code = curl_easy_setopt(
                 m_handle, CURLOPT_WRITEFUNCTION, static_cast<CURL_WRITE_CALLBACK>(curlWriteCallback)))) {
        resultCode = ResultCode::CONNECTION_FAILED;
        return resultCode;
    }

    ACSDK_DEBUG9(LX("streamToQueue").m("Getting truncated URL").d("url", fullUrl.substr(0, 100).c_str()));
    if ((m_code = curl_easy_setopt(m_handle, CURLOPT_URL, fullUrl.c_str()))) {
        return ResultCode::CONNECTION_FAILED;
    }

    m_code = curl_easy_perform(m_handle);

    if (m_code != CURLE_OK) {
        if (m_code == CURLE_ABORTED_BY_CALLBACK) {
            // cancelled by user, do not retry
            resultCode = ResultCode::CATASTROPHIC_FAILURE;
            return resultCode;
        } else {
            resultCode = ResultCode::CONNECTION_FAILED;
            return resultCode;
        }
    }

    resultCode = checkHTTPStatusCode();
    return resultCode;
}

bool CurlWrapper::unpack(std::shared_ptr<DownloadChunkQueue> downloadChunkQueue, const std::string& path) {
    ACSDK_INFO(LX("unpack").m("Unpacking from downloadChunkQueue").d("path", path.c_str()));
    auto readArchive = unique_ptr<archive, decltype(&archive_read_free)>(archive_read_new(), archive_read_free);
    auto writeArchive =
            unique_ptr<archive, decltype(&archive_write_free)>(archive_write_disk_new(), archive_write_free);
    vector<string> writtenFileslist;

    // Other archive formats are supported as well: see '*_all' in libarchive
    archive_read_support_format_all(readArchive.get());

    // Other filters are supported as well: see '*_all' in libarchive
    archive_read_support_filter_all(readArchive.get());

    // Archive callback function to read downloaded data chunk
    auto archiveReadCallback = [](struct archive* archive, void* userdata, const void** buffer) -> la_ssize_t {
        auto queuePtr = static_cast<shared_ptr<DownloadChunkQueue>*>(userdata);
        if (queuePtr == nullptr || *queuePtr == nullptr) {
            archive_set_error(archive, ARCHIVE_FAILED, "invalid userdata");
            // ssize_t as “signed size_t ”. ssize_t is able to represent the number -1
            return -1;
        }

        auto dataChunk = (*queuePtr)->waitAndPop();
        if (dataChunk == nullptr) {
            // download terminated, either encountered error or completed
            // archiveCloseCallback will be able to distinguish the two cases
            *buffer = nullptr;
            return 0;
        }
        *buffer = dataChunk->data();
        // This buffer size is by default CURL_MAX_WRITE_SIZE (16kB).
        // The maximum buffer size allowed to be set is CURL_MAX_READ_SIZE (512kB)
        // it should be safe to assign size_t to ssize_t
        return dataChunk->size();
    };

    // Archive callback function to signal unpack completion
    auto archiveCloseCallback = [](struct archive* archive, void* userdata) -> int {
        auto queuePtr = static_cast<shared_ptr<DownloadChunkQueue>*>(userdata);
        if (queuePtr == nullptr || *queuePtr == nullptr) {
            archive_set_error(archive, ARCHIVE_FAILED, "invalid userdata");
            return ARCHIVE_FATAL;
        }

        if (!(*queuePtr)->popComplete(true)) {
            archive_set_error(archive, ARCHIVE_FAILED, "download/format error");
            return ARCHIVE_FATAL;
        }
        return ARCHIVE_OK;
    };

    // Open the archive file with the maximum buffer size
    auto archiveStatus = archive_read_open(
            readArchive.get(), &downloadChunkQueue, nullptr, archiveReadCallback, archiveCloseCallback);
    if (archiveStatus != ARCHIVE_OK) {
        ACSDK_ERROR(LX("unpack").m("Failed to downnload and unpack").d("error", archiveStatus));
        return false;
    }
    return (ArchiveWrapper::getInstance())->unpack(readArchive.get(), writeArchive.get(), path);
}

ResultCode CurlWrapper::getAndDownloadMultipart(
        const std::string& url,
        shared_ptr<ResponseSink> sink,
        const std::weak_ptr<CurlProgressCallbackInterface>& callbackObj) {
    if (setHTTPHEADER() != ResultCode::SUCCESS) {
        ACSDK_ERROR(LX("getAndDownloadMultipart").m("Unable to set HTTPHEADER"));
        return ResultCode::CONNECTION_FAILED;
    }
    auto downloadChunkQueue = make_shared<DownloadChunkQueue>(0);

    auto curlHeadersCallback = [](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
        auto sink = static_cast<ResponseSink*>(userdata);
        if (sink == nullptr) {
            return 0;
        }
        auto length = size * nmemb;
        std::string line(ptr, length);
        sink->onHeader(line);
        return length;
    };

    if ((m_code = curl_easy_setopt(m_handle, CURLOPT_HEADERDATA, sink.get()))) {
        return ResultCode::CONNECTION_FAILED;
    }
    if ((m_code = curl_easy_setopt(
                 m_handle, CURLOPT_HEADERFUNCTION, static_cast<CURL_WRITE_CALLBACK>(curlHeadersCallback)))) {
        ACSDK_ERROR(LX("getAndDownloadMultipart").m("Bad header callback"));
        return ResultCode::CONNECTION_FAILED;
    }

    std::thread downloadThread(&CurlWrapper::streamToQueue, this, url, downloadChunkQueue, callbackObj);
    ResultCode resultCode;
    if (sink->parser(downloadChunkQueue)) {
        resultCode = ResultCode::SUCCESS;
    } else {
        resultCode = ResultCode::CATASTROPHIC_FAILURE;
    }
    downloadThread.join();
    if (resultCode == ResultCode::SUCCESS) {
        auto path = sink->getArtifactPath();
        if (!changePermissions(path, DEFAULT_FILE_PERMISSIONS)) {
            ACSDK_ERROR(LX("getAndDownloadMultipart").m("Failed to set DEFAULT_FILE_PERMISSIONS").d("path", path));
            return ResultCode::CATASTROPHIC_FAILURE;
        }
    }

    return resultCode;
}

ResultCode CurlWrapper::download(
        const std::string& url,
        const std::string& path,
        const weak_ptr<CurlProgressCallbackInterface>& callbackObj,
        bool unpack,
        size_t size) {
    ACSDK_INFO(LX("download")
                       .sensitive("URL for download", url.c_str())
                       .d("Local path to download", path.c_str())
                       .d("unpack", unpack));

    // common download cancellation function for both stream to file and stream to queue
    auto curlProgressCallback =
            [](void* userdata, curl_off_t dlTotal, curl_off_t dlNow, curl_off_t ulTotal, curl_off_t ulNow) -> int {
        auto callbackWeakPtr = static_cast<weak_ptr<CurlProgressCallbackInterface>*>(userdata);
        if (callbackWeakPtr == nullptr) {
            return 0;
        }
        auto callbackPtr = callbackWeakPtr->lock();
        if (callbackPtr == nullptr) {
            // Returning a non-zero value will cause libcurl to abort the transfer and return
            ACSDK_WARN(LX("curlProgressCallback")
                               .m("CurlWrapper: stream: progressFunction: callbackWeakPtr expired"));  // NOLINT
            return 1;
        }
        auto toContinue = callbackPtr->onProgressUpdate(dlTotal, dlNow, ulTotal, ulNow);
        return (toContinue) ? 0 : 1;
    };

    if ((m_code = curl_easy_setopt(
                 m_handle,
                 CURLOPT_XFERINFOFUNCTION,
                 static_cast<CURL_XFERINFOFUNCTION_CALLBACK>(curlProgressCallback)))) {
        return ResultCode::CONNECTION_FAILED;
    }
    if ((m_code = curl_easy_setopt(m_handle, CURLOPT_XFERINFODATA, &callbackObj))) {
        return ResultCode::CONNECTION_FAILED;
    }
    // If onoff is to 1, it tells the library to shut off the progress meter completely
    if ((m_code = curl_easy_setopt(m_handle, CURLOPT_NOPROGRESS, 0))) {
        return ResultCode::CONNECTION_FAILED;
    }

    if (!unpack) {
        return streamToFile(url, path, size, callbackObj);
    }

    // it seems for now that we can only handle one unpack task at a time, so only allow one download+unpack
    static mutex s_downloadUnpackMutex;
    lock_guard<mutex> lock(s_downloadUnpackMutex);

    auto downloadChunkQueue = make_shared<DownloadChunkQueue>(size);

    // Download to queue in separate thread (producer)
    // Before streamToQueue exit, it must call downloadChunkQueue->pushComplete to indicate success or failure
    std::thread downloadThread(&CurlWrapper::streamToQueue, this, url, downloadChunkQueue, callbackObj);

    // Unpack from data in queue
    ResultCode resultCode;
    if (CurlWrapper::unpack(downloadChunkQueue, path)) {
        resultCode = ResultCode::SUCCESS;
    } else {
        resultCode = ResultCode::UNPACK_FAILURE;
    }

    downloadThread.join();
    ACSDK_INFO(LX("download").d("resultCode", resultCode).d("path", path));
    return resultCode;
}
}  // namespace common
}  // namespace acsdkAssets
}  // namespace alexaClientSDK
