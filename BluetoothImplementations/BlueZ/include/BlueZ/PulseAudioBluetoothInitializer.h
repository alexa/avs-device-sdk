/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_PULSEAUDIOBLUETOOTHINITIALIZER_H_
#define ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_PULSEAUDIOBLUETOOTHINITIALIZER_H_

#include <condition_variable>
#include <memory>
#include <mutex>

#include <AVSCommon/Utils/Bluetooth/BluetoothEventBus.h>
#include <AVSCommon/Utils/Threading/Executor.h>

#include "pulse/pulseaudio.h"

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

/**
 * Applications can receive/send A2DP audio data from/to BlueZ by registering local endpoints. BlueZ
 * only supports sending audio to a single endpoint, and will choose the one that was registered first
 * with the supported capabilities (essentially, a FIFO queue).
 *
 * By default, when PulseAudio starts, it registers two local endpoints for itself: one for A2DP
 * Source and another for A2DP Sink. These will be prioritized by BlueZ and will be sent
 * audio stream data when it is available. By the time the SDK creates its endpoints, they will
 * be of a lower priority.
 *
 * For A2DP Sink, the SDK needs to obtain the audio stream and control playback. This is so the SDK
 * can correctly manage audio focus. For A2DP Source, we continue to let PulseAudio handle the audio
 * routing via their own endpoint.
 *
 * This class unregisters both PulseAudio endpoints so that the SDK's endpoint has priority for Sink,
 * and then immediately re-registers them so that PulseAudio can handle the Source case with its
 * own endpoint.
 *
 * This is behavior is optional and can be enabled at compile time with the appropriate CMake flag (refer to
 * Bluetooth.cmake).
 */

class PulseAudioBluetoothInitializer
        : public avsCommon::utils::bluetooth::BluetoothEventListenerInterface
        , public std::enable_shared_from_this<PulseAudioBluetoothInitializer> {
public:
    /// The state of each module.
    enum class ModuleState {
        /// Before we have queried the state of the module.
        UNKNOWN,
        /// The module was initially loaded.
        INITIALLY_LOADED,
        /// The module is unloaded.
        UNLOADED,
        /// The module has been loaded by the SDK.
        LOADED_BY_SDK,
    };

    /**
     * Constructor.
     *
     * @param eventBus The eventBus which we will receive events upon BluetoothDeviceManager initialization.
     *
     * @returns An instance if successful, else a nullptr.
     */
    static std::shared_ptr<PulseAudioBluetoothInitializer> create(
        std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus> eventBus);

    /**
     * Destructor.
     */
    ~PulseAudioBluetoothInitializer();

protected:
    /// @name BluetoothEventBusListenerInterface Functions
    /// @{
    void onEventFired(const avsCommon::utils::bluetooth::BluetoothEvent& event) override;
    /// @}

private:
    /**
     * Callback for PulseAudio Context state changes.
     *
     * @param context The PulseAudio context.
     * @param userdata The initializer object.
     */
    static void onStateChanged(pa_context* context, void* userdata);

    /**
     * Callback for PulseAudio Context state changes.
     *
     * @param context The PulseAudio context.
     * @param info A struct containing module information.
     * @param eol Signifies this callback is the end of the list.
     * @param userdata The initializer object.
     */
    static void onModuleFound(pa_context* context, const pa_module_info* info, int eol, void* userData);

    /**
     * A callback which indicates the result of module-bluetooth-policy.
     *
     * @param context The PulseAudio context.
     * @param index The index of the new module.
     * @param userdata The initializer object.
     */
    static void onLoadPolicyResult(pa_context* context, uint32_t index, void* userdata);

    /**
     * A callback which indicates the result of loading module-bluetooth-discover.
     *
     * @param context The PulseAudio context.
     * @param index The index of the new module.
     * @param userdata The initializer object.
     */
    static void onLoadDiscoverResult(pa_context* context, uint32_t index, void* userdata);

    /**
     * A callback which indicates the result of loading module-bluetooth-policy.
     *
     * @param context The PulseAudio context.
     * @param success Whether the module was successfully unloaded.
     * @param userdata The initializer object.
     */
    static void onUnloadPolicyResult(pa_context* context, int success, void* userdata);

    /**
     * A callback which indicates the result of loading module-bluetooth-discover.
     *
     * @param context The PulseAudio context.
     * @param success Whether the module was successfully unloaded.
     * @param userdata The initializer object.
     */
    static void onUnloadDiscoverResult(pa_context* context, int success, void* userdata);

    /**
     * A helper function to handle the callback of loading a module.
     *
     * @param context The PulseAudio context.
     * @param index Whether the module was successfully loaded.
     * @param moduleName The module that we attempted to load.
     */
    void handleLoadModuleResult(pa_context* context, uint32_t index, const std::string& moduleName);

    /**
     * A helper function to handle the callback of loading a module.
     *
     * @param context The PulseAudio context.
     * @param success Whether the module was successfully unloaded.
     * @param moduleName The module that we attempted to load.
     */
    void handleUnloadModuleResult(pa_context* context, int success, const std::string& moduleName);

    /**
     * Updates the variables tracking module state. The mutex must be locked before calling this function.
     * The function does not verify state transitions.
     *
     * @param state The desiredState.
     * @param module The module name.
     * @return Whether the state was updated.
     */
    bool updateStateLocked(const ModuleState& state, const std::string& module);

    /**
     * Sets the state and notifies the condition variable. This function does not perform state transition validation,
     * that is done by PulseAudio.
     *
     * @param state The state.
     */
    void setStateAndNotify(pa_context_state_t state);

    /**
     * Performs internal initialization of the object.
     */
    void init();

    /**
     * Entry point to the class.
     */
    void run();

    /**
     * Releases any PulseAudio resources, stops the main thread, and other cleanup.
     */
    void cleanup();

    /**
     * Constructor.
     *
     * @param eventBus The eventBus which we will receive events upon BluetoothDeviceManager initialization.
     */
    PulseAudioBluetoothInitializer(std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus> eventBus);

    /// Condition variable for @c m_mainThread to wait for @c m_paLoop.
    std::condition_variable m_mainThreadCv;

    /// Mutex protecting variables.
    std::mutex m_mutex;

    /// The eventBus which will we will receive events upon BluetoothDeviceManager initialization.
    std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus> m_eventBus;

    /// The main loop that PulseAudio callbacks occur on.
    pa_threaded_mainloop* m_paLoop;

    /// Indicates whether we have started a PulseAudio instance. Do not start multiple instances if we get multiple
    /// messages on m_eventBus.
    bool m_paLoopStarted;

    /// The PulseAudio context.
    pa_context* m_context;

    /// The state of module-bluetooth-policy.
    ModuleState m_policyState;

    /// The state of module-bluetooth-discover.
    ModuleState m_discoverState;

    /// Whether a connection to PulseAudio was succesful.
    bool m_connected;

    /// An executor to serialize calls.
    avsCommon::utils::threading::Executor m_executor;
};

/**
 * Converts the @c ModuleState enum to a string.
 *
 * @param The @c ModuleState to convert.
 * @return A string representation of the @c ModuleState.
 */
inline std::string moduleStateToString(const PulseAudioBluetoothInitializer::ModuleState& state) {
    switch (state) {
        case PulseAudioBluetoothInitializer::ModuleState::UNKNOWN:
            return "UNKNOWN";
        case PulseAudioBluetoothInitializer::ModuleState::INITIALLY_LOADED:
            return "INITIALLY_LOADED";
        case PulseAudioBluetoothInitializer::ModuleState::UNLOADED:
            return "UNLOADED";
        case PulseAudioBluetoothInitializer::ModuleState::LOADED_BY_SDK:
            return "LOADED_BY_SDK";
    }

    return "INVALID";
}

/**
 * Overload for the @c ModuleState enum. This will write the @c ModuleState as a string to the provided
 * stream.
 *
 * @param An ostream to send the @c ModuleState as a string.
 * @param The @c ModuleState.
 * @return The stream.
 */
inline std::ostream& operator<<(std::ostream& stream, const PulseAudioBluetoothInitializer::ModuleState& state) {
    return stream << moduleStateToString(state);
}

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_PULSEAUDIOBLUETOOTHINITIALIZER_H_
