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

#ifndef ALEXA_CLIENT_SDK_MEDIAPLAYER_GSTREAMERMEDIAPLAYER_INCLUDE_MEDIAPLAYER_PIPELINEINTERFACE_H_
#define ALEXA_CLIENT_SDK_MEDIAPLAYER_GSTREAMERMEDIAPLAYER_INCLUDE_MEDIAPLAYER_PIPELINEINTERFACE_H_

#include <cstdint>
#include <memory>
#include <functional>

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

namespace alexaClientSDK {
namespace mediaPlayer {

/**
 * An interface that allows accessing some members of an @c AudioPipeline instantiated by the @c MediaPlayer. The
 * interface also allows queuing callbacks on the worker thread of the @c MediaPlayer.
 */
class PipelineInterface {
public:
    /**
     * Sets the appSrc element in the @c AudioPipeline.
     *
     * @param appSrc The element the appSrc of @c AudioPipeline should be set to.
     *
     */
    virtual void setAppSrc(GstAppSrc* appSrc) = 0;

    /**
     * Gets the appSrc element of the @c AudioPipeline.
     *
     * @return The appSrc element.
     */
    virtual GstAppSrc* getAppSrc() const = 0;

    /**
     * Sets the decoder element in the @c AudioPipeline
     *
     * @param decoder The element the decoder of @c AudioPipeline should be set to.
     *
     */
    virtual void setDecoder(GstElement* decoder) = 0;

    /**
     * Gets the decoder element of the @c AudioPipeline.
     *
     * @return The decoder element.
     */
    virtual GstElement* getDecoder() const = 0;

    /**
     * Gets the pipeline of the @c AudioPipeline.
     *
     * @return The pipeline.
     */
    virtual GstElement* getPipeline() const = 0;

    /**
     * Queue the specified callback for execution on the worker thread.
     *
     * @param callback The callback to queue.
     * @return The ID of the queued callback (for calling @c g_source_remove).
     */
    virtual guint queueCallback(const std::function<gboolean()>* callback) = 0;

    /**
     * Attach the source to the worker thread.
     *
     * @param source The source to be executed on the worker thread.
     * @return The ID (greater than 0) of the source.  0 if there is an error.
     */
    virtual guint attachSource(GSource* source) = 0;

    /**
     * Remove the callback from the worker thread.
     *
     * @param The ID of the queued callback.
     * @return Whether the removal is successful.
     */
    virtual gboolean removeSource(guint tag) = 0;

protected:
    /**
     * Destructor.
     */
    virtual ~PipelineInterface() = default;
};

}  // namespace mediaPlayer
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_MEDIAPLAYER_GSTREAMERMEDIAPLAYER_INCLUDE_MEDIAPLAYER_PIPELINEINTERFACE_H_
