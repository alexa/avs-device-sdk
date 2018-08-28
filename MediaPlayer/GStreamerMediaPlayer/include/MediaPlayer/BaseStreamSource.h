/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_MEDIAPLAYER_GSTREAMERMEDIAPLAYER_INCLUDE_MEDIAPLAYER_BASESTREAMSOURCE_H_
#define ALEXA_CLIENT_SDK_MEDIAPLAYER_GSTREAMERMEDIAPLAYER_INCLUDE_MEDIAPLAYER_BASESTREAMSOURCE_H_

#include <memory>

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>

#include "MediaPlayer/SourceInterface.h"

namespace alexaClientSDK {
namespace mediaPlayer {

class BaseStreamSource : public SourceInterface {
public:
    /**
     * Constructor.
     *
     * @param pipeline The @c PipelineInterface through which the source of the @c AudioPipeline may be set.
     * @param className The name of the class to be passed to @c RequiresShutdown.
     */
    BaseStreamSource(PipelineInterface* pipeline, const std::string& className);

    ~BaseStreamSource() override;

    bool hasAdditionalData() override;
    bool handleEndOfStream() override;
    void preprocess() override;

protected:
    /**
     * Initializes a source. Creates all the necessary pipeline elements such that audio output from the final
     * element should be decoded output that can be input to the @c converter of the @c AudioPipeline. Adding
     * the elements to the @c pipeline of the @c AudioPipeline, linking the elements and setting up the
     * callbacks for signals should be handled.
     *
     * @param audioFormat The audioFormat to be used when playing raw PCM data.
     * @return @c true if the initialization was successful else @c false.
     */
    bool init(const avsCommon::utils::AudioFormat* audioFormat = nullptr);

    /**
     * Return whether the audio source is still open.
     *
     * @return whether the audio source is still open.
     */
    virtual bool isOpen() = 0;

    /**
     * Close the audio source.
     */
    virtual void close() = 0;

    /**
     * Reads data from this instance and pushes it into appsrc element.
     *
     * @return @c false if there is an error or end of data from this source, else @c true.
     */
    virtual gboolean handleReadData() = 0;

    /**
     * Seeks to the appropriate offset. Any data pushed after this should come from the new offset.
     *
     * @return @c false if the seek failed, or @c true otherwise.
     */
    virtual gboolean handleSeekData(guint64 offset) = 0;

    /**
     * Get the AppSrc to which this instance should feed audio data.
     *
     * @return The AppSrc to which this instance should feed audio data.
     */
    GstAppSrc* getAppSrc() const;

    /**
     * Signal gstreamer about the end of data from this instance.
     */
    void signalEndOfData();

    /**
     * Install the @c onReadData() handler.  If it is already installed, reset the retry count.
     */
    void installOnReadDataHandler();

    /**
     * Update when to call @c onReadData() handler based upon the number of retries since data was last read.
     */
    void updateOnReadDataHandler();

    /**
     * Uninstall the @c onReadData() handler.
     */
    void uninstallOnReadDataHandler();

    /**
     * Clear out the tracking of the @c onReadData() handler callback.  This is used when gstreamer is
     * known to have uninstalled the handler on its own.
     */
    void clearOnReadDataHandler();

private:
    /**
     * The callback for pushing data into the appsrc element.
     *
     * @param pipeline The pipeline element.
     * @param size The size of the data to push to the pipeline.
     * @param source The instance from which data is needed.
     */
    static void onNeedData(GstElement* pipeline, guint size, gpointer source);

    /**
     * Pushes data into the appsrc element by starting a read data loop to run.
     *
     * @return Whether this callback should be called again on the worker thread.
     */
    gboolean handleNeedData();

    /**
     * The callback to stop pushing data into the appsrc element.
     *
     * @param pipeline The pipeline element.
     * @param source The instance from which enough data has been received.
     */
    static void onEnoughData(GstElement* pipeline, gpointer source);

    /**
     * Stops pushing data to the @c appSrc element.
     *
     * @return @c false always.
     */
    gboolean handleEnoughData();

    static gboolean onSeekData(GstElement* pipeline, guint64 offset, gpointer source);

    /**
     * The callback for reading data from this instance.
     *
     * @param source The instance to read data from
     * @return @c false if there is an error or end of data from this source, else @c true.
     */
    static gboolean onReadData(gpointer source);

    /// The @c PipelineInterface through which the source of the @c AudioPipeline may be set.
    PipelineInterface* m_pipeline;

    /// The sourceId used to identify the installation of the @c onReadData() handler.
    guint m_sourceId;

    /// Number of times reading data has been attempted since data was last successfully read.
    guint m_sourceRetryCount;

    /// Function to invoke on the worker thread thread when more data is needed.
    const std::function<gboolean()> m_handleNeedDataFunction;

    /// Function to invoke on the worker thread thread when there is enough data.
    const std::function<gboolean()> m_handleEnoughDataFunction;

    /// ID of the handler installed to receive need data signals.
    guint m_needDataHandlerId;

    /// ID of the handler installed to receive enough data signals.
    guint m_enoughDataHandlerId;

    /// ID of the handler installed to receive seek data signals.
    guint m_seekDataHandlerId;

    /// Mutex to serialize access to idle callback IDs.
    std::mutex m_callbackIdMutex;

    /// ID of idle callback to handle need data.
    guint m_needDataCallbackId;

    /// ID of idle callback to handle enough data.
    guint m_enoughDataCallbackId;
};

}  // namespace mediaPlayer
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_MEDIAPLAYER_GSTREAMERMEDIAPLAYER_INCLUDE_MEDIAPLAYER_BASESTREAMSOURCE_H_
