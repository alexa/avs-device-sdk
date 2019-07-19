/*
 * Copyright 2017-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_PLAYLISTPARSER_INCLUDE_PLAYLISTPARSER_URLCONTENTTOATTACHMENTCONVERTER_H_
#define ALEXA_CLIENT_SDK_PLAYLISTPARSER_INCLUDE_PLAYLISTPARSER_URLCONTENTTOATTACHMENTCONVERTER_H_

#include <atomic>
#include <memory>

#include <AVSCommon/AVS/Attachment/InProcessAttachment.h>
#include <AVSCommon/AVS/Attachment/InProcessAttachmentReader.h>
#include <AVSCommon/AVS/Attachment/InProcessAttachmentWriter.h>

#include <AVSCommon/SDKInterfaces/HTTPContentFetcherInterfaceFactoryInterface.h>
#include <AVSCommon/Utils/PlaylistParser/PlaylistParserObserverInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>

#include "PlaylistParser/ContentDecrypter.h"
#include "PlaylistParser/PlaylistParser.h"

namespace alexaClientSDK {
namespace playlistParser {

/// Class that handles the streaming of urls containing media into @c Attachments.
class UrlContentToAttachmentConverter
        : public avsCommon::utils::playlistParser::PlaylistParserObserverInterface
        , public avsCommon::utils::RequiresShutdown {
public:
    /// Class to observe errors that arise from converting a URL to to an @c Attachment
    class ErrorObserverInterface {
    public:
        /**
         * Destructor.
         */
        virtual ~ErrorObserverInterface() = default;

        /**
         * Notification that an error has occurred in the streaming of content.
         */
        virtual void onError() = 0;
    };

    /**
     * Creates a converter object. Note that calling this function will commence the parsing and streaming of the URL
     * into the internal attachment. If a desired start time is specified, this function will attempt to start streaming
     * at that offset, based on available metadata if the URL points to a playlist file. If no such information is
     * available, streaming will begin from the beginning. It is up to the caller of this function to make a call to
     * @c getStartStreamingPoint() to find out the actual offset from which streaming began.
     *
     * @param contentFetcherFactory Used to create @c HTTPContentFetchers.
     * @param url The URL to stream from.
     * @param observer An observer to be notified of any errors that may happen during streaming.
     * @param startTime The desired time to attempt to start streaming from. Note that this will only succeed
     * in cases where the URL points to a playlist with metadata about individual chunks within it. If none are found,
     * streaming will begin from the beginning.
     * @return A @c std::shared_ptr to the new @c UrlContentToAttachmentConverter object or @c nullptr on failure.
     *
     * @note This object is intended to be used once. Subsequent calls to @c convertPlaylistToAttachment() will fail.
     */
    static std::shared_ptr<UrlContentToAttachmentConverter> create(
        std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory,
        const std::string& url,
        std::shared_ptr<ErrorObserverInterface> observer,
        std::chrono::milliseconds startTime = std::chrono::milliseconds::zero());

    /**
     * Returns the attachment into which the URL content was streamed into.
     *
     * @return A @c std::shared_ptr of an @c InProcessAttachment or @c nullptr on failure.
     */
    std::shared_ptr<avsCommon::avs::attachment::InProcessAttachment> getAttachment();

    /**
     * Gets the actual point from which streaming began.
     *
     * @return The point from which streaming began.
     */
    std::chrono::milliseconds getStartStreamingPoint();

    /**
     * Gets the initial desired point of steaming.
     *
     * @return The point from which streaming was desired to begin at.
     */
    std::chrono::milliseconds getDesiredStreamingPoint();

    void doShutdown() override;

private:
    /**
     * Constructor.
     *
     * @param contentFetcherFactory Used to create @c HTTPContentFetcher's.
     * @param url The URL to stream from.
     * @param observer An observer to be notified of any errors that may happen during streaming.
     * @param desiredStartTime The desired time to attempt to start streaming from. Note that this will only succeed
     * in cases where the URL points to a playlist with metadata about individual chunks within it. If none are found,
     * streaming will begin from the beginning.
     */
    UrlContentToAttachmentConverter(
        std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory,
        const std::string& url,
        std::shared_ptr<ErrorObserverInterface> observer,
        std::chrono::milliseconds startTime);

    void onPlaylistEntryParsed(int requestId, avsCommon::utils::playlistParser::PlaylistEntry playlistEntry) override;

    /**
     * Notify the observer that an error has occured.
     **/
    void notifyError();

    /**
     * @name Executor Thread Functions
     *
     * These functions (and only these functions) are called by @c m_executor on a single worker thread.  All other
     * functions in this class can be called asynchronously, and pass data to the @c Executor thread through parameters
     * to lambda functions.  No additional synchronization is needed.
     */
    /// @{

    /**
     * Downloads the content from the url, decrypts (if required) and writes it into the internal stream.
     *
     * @param url The URL to download.
     * @param headers HTTP headers to pass to server.
     * @param encryptionInfo The Encryption info for the URL to download.
     * @param contentFetcher The content fetcher to use to retrieve content. Can be a null pointer.
     * @return @c true if the content was successfully streamed and written or @c false otherwise.
     */
    bool writeDecryptedUrlContentIntoStream(
        std::string url,
        std::vector<std::string> headers,
        avsCommon::utils::playlistParser::EncryptionInfo encryptionInfo,
        std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterface> contentFetcher);

    /**
     * Downloads the content from the url and writes to the stream.
     *
     * @param url The URL to download.
     * @param headers HTTP headers to pass to server.
     * @param streamWriter The attachment writer to write downloaded content.
     * @param contentFetcher The content fetcher to use to retrieve content. Can be a null pointer.
     * @return @c true if the content was successfully downloaded or @c false otherwise.
     */
    bool download(
        const std::string& url,
        const std::vector<std::string>& headers,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentWriter> streamWriter,
        std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterface> contentFetcher);

    /**
     * Downloads the content from the url to unsigned char vector.
     *
     * @param url The URL to download.
     * @param headers HTTP headers to pass to server.
     * @param[out] content Sets the content of the pointer if download is successful.
     * @param contentFetcher The content fetcher to use to retrieve content. Can be a null pointer.
     * @return @c true if the content was successfully downloaded or @c false otherwise.
     */
    bool download(
        const std::string& url,
        const std::vector<std::string>& headers,
        ByteVector* content,
        std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterface> contentFetcher);

    /**
     * Reads content from the reader to unsigned char vector.
     *
     * @param reader The AttachmentReder to read content from.
     * @param[out] content Sets the content of the pointer if read is successful.
     * @return @c true if the content was successfully read or @c false otherwise.
     */
    bool readContent(std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> reader, ByteVector* content);

    /**
     * Helper to decide if decryption is required.
     *
     * @param encryptionInfo EncryptionInfo of the Media URL.
     * @return @c true if content should be decrypted or @c false otherwise.
     */
    bool shouldDecrypt(const avsCommon::utils::playlistParser::EncryptionInfo& encryptionInfo) const;

    /**
     * Helper method to close writing to stream.
     **/
    void closeStreamWriter();

    /// @}

    /// The initial desired offset from which streaming should begin.
    const std::chrono::milliseconds m_desiredStreamPoint;

    /// Used to retrieve content from URLs
    std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> m_contentFetcherFactory;

    /// Used to parse URLS that point to playlists.
    std::shared_ptr<PlaylistParser> m_playlistParser;

    /// The stream that will hold downloaded data.
    std::shared_ptr<avsCommon::avs::attachment::InProcessAttachment> m_stream;

    /// The writer used to write data into the stream.
    std::shared_ptr<avsCommon::avs::attachment::AttachmentWriter> m_streamWriter;

    /// The observer to be notified of errors.
    std::shared_ptr<ErrorObserverInterface> m_observer;

    /// Used to serialize access to m_observer.
    std::mutex m_mutex;

    /// Flag to indicate if a shutdown is occurring.
    std::atomic<bool> m_shuttingDown;

    /// Helper to decrypt encrypted content.
    std::shared_ptr<ContentDecrypter> m_contentDecrypter;

    /**
     * @name @c onPlaylistEntryParsed Callback Variables
     *
     * These member variables are only accessed by @c onPlaylistEntryParsed callback functions that is called
     * in a single thread in PlaylistParser or after the @c m_playlistParser is shutdown, and do not require any
     * synchronization.
     */
    /// @{
    /// A promise to fulfill once streaming begins.
    std::promise<std::chrono::milliseconds> m_startStreamingPointPromise;

    /// A future which will hold the point at which streaming began.
    std::shared_future<std::chrono::milliseconds> m_startStreamingPointFuture;

    /// A counter to indicate the current running total of durations found in playlist entries.
    std::chrono::milliseconds m_runningTotal;

    /// Indicates whether streaming has begun.
    bool m_startedStreaming;
    /// @}

    /**
     * @name Executor Thread Variables
     *
     * These member variables are only accessed by functions in the @c m_executor worker thread, and do not require any
     * synchronization.
     */
    /// @{
    /// Indicates whether the stream writer has closed.
    bool m_streamWriterClosed;
    /// @}

    /**
     * @c Executor which queues up operations from asynchronous API calls.
     *
     * @note This declaration needs to come *after* the Executor Thread Variables above so that the thread shuts down
     *     before the Executor Thread Variables are destroyed.
     */
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace playlistParser
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_PLAYLISTPARSER_INCLUDE_PLAYLISTPARSER_URLCONTENTTOATTACHMENTCONVERTER_H_
