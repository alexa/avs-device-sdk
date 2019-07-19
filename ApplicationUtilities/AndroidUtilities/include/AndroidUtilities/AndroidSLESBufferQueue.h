/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#ifndef ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_ANDROIDUTILITIES_INCLUDE_ANDROIDUTILITIES_ANDROIDSLESBUFFERQUEUE_H_
#define ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_ANDROIDUTILITIES_INCLUDE_ANDROIDUTILITIES_ANDROIDSLESBUFFERQUEUE_H_

#include <array>
#include <vector>

#include <SLES/OpenSLES_Android.h>

#include <AVSCommon/AVS/AudioInputStream.h>
#include <AndroidUtilities/AndroidSLESObject.h>

namespace alexaClientSDK {
namespace applicationUtilities {
namespace androidUtilities {

/**
 * Class responsible for consuming data produced from an AndroidSLESObject.
 *
 * For that, it keeps a circular buffer queue that it feeds to the OpenSL ES microphone. When the microphone
 * is recording it should call @c enqueueBuffers to enqueue all free buffers to the OpenSL ES object.
 *
 * Whenever the OpenSL ES microphone fills up a buffer, it calls the @c onBufferCompleted method, which will copy
 * the recorded data to the @c AudioInputStream and enqueue the same buffer back.
 *
 * When recording is stopped, clearBuffers can be used to clear all buffers with any previously recorded data.
 */
class AndroidSLESBufferQueue {
public:
    /**
     * Creates a new @c AndroidSLESBufferQueue object.
     *
     * @param queueObject The AndroidSLESObject that represent the OpenSL ES microphone and queue.
     * @param writer The buffer writer to copy the recorded audio to.
     * @return Pointer to the new object if object could be successfully created; otherwise, return a @c nullptr.
     */
    static std::unique_ptr<AndroidSLESBufferQueue> create(
        std::shared_ptr<AndroidSLESObject> queueObject,
        std::unique_ptr<avsCommon::avs::AudioInputStream::Writer> writer);

    /**
     * The callback function to be called when a buffer has been filled with recorded data. This function will copy the
     * data out of the filled buffer into its @c AudioInputStream.
     */
    void onBufferCompleted();

    /**
     * Add all free buffers into the queue.
     *
     * @return @c true if succeeds and @c false otherwise.
     */
    bool enqueueBuffers();

    /**
     * Clear all the buffers from the queue.
     *
     * @return @c true if succeeds and @c false otherwise.
     */
    bool clearBuffers();

    /**
     * Destructor.
     */
    ~AndroidSLESBufferQueue();

    /// The number of buffers to use.
    static constexpr uint32_t NUMBER_OF_BUFFERS{2u};

private:
    /**
     * Constructor.
     *
     * @param slObject The @c AndroidSLESObject representation which implements the OpenSL ES microphone.
     * @param bufferQueue The buffer queue interface representation used to access the OpenSL ES microphone queue.
     * @param writer The buffer writer to copy the recorded audio to.
     */
    AndroidSLESBufferQueue(
        std::shared_ptr<AndroidSLESObject> slObject,
        SLAndroidSimpleBufferQueueItf bufferQueue,
        std::unique_ptr<avsCommon::avs::AudioInputStream::Writer> writer);

    /// Enqueue buffer and increment m_index. This function should only be called with a lock.
    bool enqueueBufferLocked();

    /// Size of each buffer. It should be greater than 0.
    static constexpr size_t BUFFER_SIZE{8192u};

    /// Mutex used to guard queue operations.
    std::mutex m_mutex;

    /// Internal AndroidSLES object which represents the microphone and its queue. Keep a reference to it to ensure that
    /// the object will stay valid.
    std::shared_ptr<AndroidSLESObject> m_slObject;

    /// Internal AndroidSLES recorder interface which is used to access the OpenSL ES Object.
    SLAndroidSimpleBufferQueueItf m_queueInterface;

    /// Internal buffers used to record data.
    /// @warning We assume that m_buffers cannot be empty.
    std::array<std::array<int16_t, BUFFER_SIZE>, NUMBER_OF_BUFFERS> m_buffers;

    /// The writer that will be used to write audio data into the sds.
    std::unique_ptr<avsCommon::avs::AudioInputStream::Writer> m_writer;

    /// Index of the next available buffer.
    std::size_t m_index;
};

}  // namespace androidUtilities
}  // namespace applicationUtilities
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_ANDROIDUTILITIES_INCLUDE_ANDROIDUTILITIES_ANDROIDSLESBUFFERQUEUE_H_
