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

#ifndef ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_MEDIAENDPOINT_H_
#define ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_MEDIAENDPOINT_H_

#include <atomic>
#include <condition_variable>
#include <vector>

#include <AVSCommon/AVS/AudioInputStream.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/Services/A2DPSourceInterface.h>
#include <AVSCommon/Utils/Bluetooth/FormattedAudioStreamAdapter.h>

#include "BlueZ/BlueZDeviceManager.h"
#include "BlueZ/BlueZUtils.h"
#include "BlueZ/MediaContext.h"

#include <gio/gio.h>
#include <sbc/sbc.h>

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

/**
 * A DBus object implementing the MediaEndpoint1 interface of BlueZ. Registering it with DBus allows BlueZ to use it
 * for audio streaming using A2DP. This particular endpoint implements the SINK case, where remote bluetooth device is
 * streaming audio to our SDK.
 */
class MediaEndpoint : public DBusObject<MediaEndpoint> {
public:
    /**
     * Constructor inherited from @c DBusObject base class. Prepares a DBus object for registration.
     *
     * @param connection DBus connection to register object with.
     * @param endpointPath Object path to register object to.
     */
    MediaEndpoint(std::shared_ptr<DBusConnection> connection, const std::string& endpointPath);

    /**
     * Destructor
     */
    ~MediaEndpoint() override;

    /**
     * A callback called by BlueZ to notify MediaEndpoint of the stream codec configuration selected by both sides.
     *
     * @param arguments Variant containing the codec configuration bytes (4 bytes for SBC)
     * @param invocation DBus method invocation object used to report execution result.
     */
    void onSetConfiguration(GVariant* arguments, GDBusMethodInvocation* invocation);

    /**
     * A callback called by BlueZ to asking MediaEndpoint to select the audio code configuration to use for the audio
     * streaming. BlueZ expects this method to return a variant containing the selected configuration. This
     * configuration will then be provided to the @c onSetConfiguration callback
     *
     * @param arguments Variant containing the available codec configurations. For SBC this is a 4 byte bitmask.
     * @param invocation DBus method invocation object used to report execution result.
     */
    void onSelectConfiguration(GVariant* arguments, GDBusMethodInvocation* invocation);

    /**
     * A callback called by BlueZ to notify MediaEndpoint that it should reset the codec configuration.
     *
     * @param arguments Null
     * @param invocation DBus method invocation object used to report execution result.
     */
    void onClearConfiguration(GVariant* arguments, GDBusMethodInvocation* invocation);

    /**
     * A callback called by BlueZ to notify MediaEndpoint that it is being release and will not be used anymore.
     *
     * @param arguments Null
     * @param invocation DBus method invocation object used to report execution result.
     */
    void onRelease(GVariant* arguments, GDBusMethodInvocation* invocation);

    /**
     * A callback called by @c DeviceManager to notify MediaEndpoint that BlueZ reported a streaming state change.
     *
     * @param newState A new @c MediaStreamingState reported by DBus.
     * @param devicePath DBus object path of the device reporting the state change.
     */
    void onMediaTransportStateChanged(
        avsCommon::utils::bluetooth::MediaStreamingState newState,
        const std::string& devicePath);

    /**
     * Get DBus object path of the media endpoint
     *
     * @return Object path of the media endpoint
     */
    std::string getEndpointPath() const;

    /**
     * Get DBus object path of the device the BlueZ currently uses this media endpoint with for streaming.
     *
     * @return Device object path.
     */
    std::string getStreamingDevicePath() const;

    /**
     * Get the @c FormattedAudioStreamAdapter for the audio stream being received from the remote bluetooth device over
     * A2DP. It is safe to call this method early.
     *
     * @return An @c FormattedAudioStreamAdapter object used by @c MediaEndpoint.
     */
    std::shared_ptr<avsCommon::utils::bluetooth::FormattedAudioStreamAdapter> getAudioStream();

private:
    /**
     * Operating mode of the @c MediaEndpoint and its media stream
     */
    enum class OperatingMode {
        /**
         * There is no streaming currently active. Media streaming thread is waiting for the new mode.
         */
        INACTIVE,

        /**
         * The @c MediaEnpoint is working in SINK mode, receiving audio stream from the remote bluetooth device. Media
         * streaming thread is reading the data from the file descriptor provided by BlueZ.
         */
        SINK,

        /**
         * Reserved for future use.
         */
        SOURCE,

        /**
         * The @c MediaEndpoint has been released and any operation on it should fail. Media streaming thread will
         * stop processing the data and exit as soon as returns from any active blocking operation such as poll() or
         * read().
         */
        RELEASED
    };

    /**
     * Returns a string representation of @c OperatingMode enum class
     * @param mode
     * @return
     */
    std::string operatingModeToString(OperatingMode mode);

    /**
     * A media streaming thread main function.
     */
    void mediaThread();

    /**
     * Set the current operating mode for the media endpoint.
     *
     * @param mode A new operating mode to set.
     */
    void setOperatingMode(OperatingMode mode);

    /**
     * Disconnects the device and enters INACTIVE state.
     *
     * @remarks This method is used by media streaming thread to abort the streaming when error occurs.
     */
    void abortStreaming();

    /**
     * An object path where media endpoint is/should be registered.
     */
    std::string m_endpointPath;

    /**
     * An object path of the device that is currently being used to stream from using this media endpoint.
     */
    std::string m_streamingDevicePath;

    /**
     * Flag signalling that operating mode has changed
     */
    bool m_operatingModeChanged;

    /**
     * Current @c OperatingMode.
     */
    std::atomic<OperatingMode> m_operatingMode;

    /**
     * Buffer used to decode SBC data to. Contains raw PCM data after the decoding.
     */
    std::vector<uint8_t> m_sbcBuffer;

    /**
     * Mutex synchronizing the state of media thread.
     */
    std::mutex m_mutex;

    /**
     * Mutex synchronizing the creation/querying of the audio @c FormattedAudioStreamAdapter object.
     */
    std::mutex m_streamMutex;

    /**
     * A condition variable to listen for operating state changes.
     */
    std::condition_variable m_modeChangeSignal;

    /**
     * @c FormattedAudioStreamAdapter object exposed to the clients and used to send decoded audio data to.
     */
    std::shared_ptr<avsCommon::utils::bluetooth::FormattedAudioStreamAdapter> m_ioStream;

    /**
     * Buffer for receiving encoded data from BlueZ. This buffer contains RTP packets with SBC packets payload.
     */
    std::vector<uint8_t> m_ioBuffer;

    /**
     * The @c AudioFormat associated with the stream.
     */
    avsCommon::utils::AudioFormat m_audioFormat;

    /**
     * A @c MediaContext instance used to hold the streaming configuration before the actual stream starts. As soon
     * as stream starts this configuration is cleared and will be created again, when streaming configuration changes.
     */
    std::shared_ptr<MediaContext> m_currentMediaContext;

    /*
     * Dedicated thread for I/O.
     */
    std::thread m_thread;
};

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_MEDIAENDPOINT_H_
