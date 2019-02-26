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

#include <chrono>
#include <string>

#include "BlueZ/PulseAudioBluetoothInitializer.h"

#include <AVSCommon/Utils/Logger/Logger.h>

/// String to identify log entries originating from this file.
static const std::string TAG{"PulseAudioBluetoothInitializer"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

using namespace avsCommon::utils::bluetooth;

/// The PulseAudio module related to device discovery.
static std::string BLUETOOTH_DISCOVER = "module-bluetooth-discover";

/// The PulseAudio module related to stack related policies.
static std::string BLUETOOTH_POLICY = "module-bluetooth-policy";

/// Return indicating an operation was successful.
static const int PA_CONTEXT_CB_SUCCESS{1};

/// Return for a module callback indicating that this is the eol.
static const int PA_MODULE_CB_EOL_EOL{1};

/// Return for a module callback indicating that an error occurred.
static const int PA_MODULE_CB_EOL_ERR{-1};

/// Timeout for blocking operations.
static const std::chrono::seconds TIMEOUT{2};

/**
 * Converts a pa_context_state_t enum to a string.
 */
static std::string stateToString(pa_context_state_t state) {
    switch (state) {
        case PA_CONTEXT_UNCONNECTED:
            return "PA_CONTEXT_UNCONNECTED";
        case PA_CONTEXT_CONNECTING:
            return "PA_CONTEXT_CONNECTING";
        case PA_CONTEXT_AUTHORIZING:
            return "PA_CONTEXT_AUTHORIZING";
        case PA_CONTEXT_SETTING_NAME:
            return "PA_CONTEXT_SETTING_NAME";
        case PA_CONTEXT_READY:
            return "PA_CONTEXT_READY";
        case PA_CONTEXT_FAILED:
            return "PA_CONTEXT_FAILED";
        case PA_CONTEXT_TERMINATED:
            return "PA_CONTEXT_TERMINATED";
    }

    return "UNKNOWN";
}

std::shared_ptr<PulseAudioBluetoothInitializer> PulseAudioBluetoothInitializer::create(
    std::shared_ptr<BluetoothEventBus> eventBus) {
    ACSDK_DEBUG5(LX(__func__));

    if (!eventBus) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullEventBus"));
        return nullptr;
    }

    auto pulseAudio = std::shared_ptr<PulseAudioBluetoothInitializer>(new PulseAudioBluetoothInitializer(eventBus));
    pulseAudio->init();
    return pulseAudio;
}

PulseAudioBluetoothInitializer::PulseAudioBluetoothInitializer(
    std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus> eventBus) :
        m_eventBus{eventBus},
        m_paLoop{nullptr},
        m_paLoopStarted{false},
        m_context{nullptr},
        m_policyState{ModuleState::UNKNOWN},
        m_discoverState{ModuleState::UNKNOWN},
        m_connected{false} {
}

void PulseAudioBluetoothInitializer::init() {
    ACSDK_DEBUG5(LX(__func__));
    m_eventBus->addListener({BluetoothEventType::BLUETOOTH_DEVICE_MANAGER_INITIALIZED}, shared_from_this());
}

void PulseAudioBluetoothInitializer::onLoadDiscoverResult(pa_context* context, uint32_t index, void* userdata) {
    ACSDK_DEBUG5(LX(__func__));

    if (!userdata) {
        ACSDK_ERROR(LX("onLoadDiscoverResultFailed").d("reason", "nullUserData"));
        return;
    }

    PulseAudioBluetoothInitializer* initializer = static_cast<PulseAudioBluetoothInitializer*>(userdata);
    initializer->handleLoadModuleResult(context, index, BLUETOOTH_DISCOVER);
}

void PulseAudioBluetoothInitializer::onLoadPolicyResult(pa_context* context, uint32_t index, void* userdata) {
    ACSDK_DEBUG5(LX(__func__));

    if (!userdata) {
        ACSDK_ERROR(LX("onLoadPolicyResultFailed").d("reason", "nullUserData"));
        return;
    }

    PulseAudioBluetoothInitializer* initializer = static_cast<PulseAudioBluetoothInitializer*>(userdata);
    initializer->handleLoadModuleResult(context, index, BLUETOOTH_POLICY);
}

void PulseAudioBluetoothInitializer::handleLoadModuleResult(
    pa_context* context,
    uint32_t index,
    const std::string& moduleName) {
    ACSDK_DEBUG5(LX(__func__).d("module", moduleName).d("index", index));

    std::unique_lock<std::mutex> lock(m_mutex);

    if (!context) {
        ACSDK_ERROR(LX("handleLoadModuleResultFailed").d("reason", "nullContext"));
        return;
    } else if (PA_INVALID_INDEX == index) {
        ACSDK_ERROR(LX("handleLoadModuleResultFailed").d("reason", "loadFailed"));
        return;
    }

    if (updateStateLocked(ModuleState::LOADED_BY_SDK, moduleName)) {
        m_mainThreadCv.notify_one();
    }
}

void PulseAudioBluetoothInitializer::onUnloadPolicyResult(pa_context* context, int success, void* userdata) {
    ACSDK_DEBUG5(LX(__func__));

    if (!userdata) {
        ACSDK_ERROR(LX("onUnloadPolicyResultFailed").d("reason", "nullUserData"));
        return;
    }

    PulseAudioBluetoothInitializer* initializer = static_cast<PulseAudioBluetoothInitializer*>(userdata);
    initializer->handleUnloadModuleResult(context, success, BLUETOOTH_POLICY);
}

void PulseAudioBluetoothInitializer::onUnloadDiscoverResult(pa_context* context, int success, void* userdata) {
    ACSDK_DEBUG5(LX(__func__));

    if (!userdata) {
        ACSDK_ERROR(LX("onUnloadDiscoverResultFailed").d("reason", "nullUserData"));
        return;
    }

    PulseAudioBluetoothInitializer* initializer = static_cast<PulseAudioBluetoothInitializer*>(userdata);
    initializer->handleUnloadModuleResult(context, success, BLUETOOTH_DISCOVER);
}

void PulseAudioBluetoothInitializer::handleUnloadModuleResult(
    pa_context* context,
    int success,
    const std::string& moduleName) {
    ACSDK_DEBUG5(LX(__func__).d("module", moduleName).d("success", success));

    if (!context) {
        ACSDK_ERROR(LX("handleUnloadModuleResultFailed").d("reason", "nullContext"));
        return;
    } else if (PA_CONTEXT_CB_SUCCESS != success) {
        ACSDK_ERROR(LX("handleUnloadModuleResult").d("reason", "unloadFailed"));
        return;
    }

    if (updateStateLocked(ModuleState::UNLOADED, moduleName)) {
        m_mainThreadCv.notify_one();
    }
}

void PulseAudioBluetoothInitializer::onModuleFound(
    pa_context* context,
    const pa_module_info* info,
    int eol,
    void* userdata) {
    ACSDK_DEBUG9(LX(__func__));

    if (!context) {
        ACSDK_ERROR(LX("moduleFoundFailed").d("reason", "nullContext"));
        return;
    } else if (!userdata) {
        ACSDK_ERROR(LX("moduleFoundFailed").d("reason", "nullUserData"));
        return;
    } else if (PA_MODULE_CB_EOL_ERR == eol) {
        ACSDK_ERROR(LX("moduleFoundFailed").d("reason", "pulseAudioError").d("eol", PA_MODULE_CB_EOL_ERR));
        return;
    }

    PulseAudioBluetoothInitializer* initializer = static_cast<PulseAudioBluetoothInitializer*>(userdata);

    std::unique_lock<std::mutex> lock(initializer->m_mutex);

    // If end of the list info object is not valid.
    if (PA_MODULE_CB_EOL_EOL == eol) {
        ACSDK_DEBUG(LX(__func__).m("endOfList"));
        if (ModuleState::INITIALLY_LOADED != initializer->m_policyState) {
            initializer->updateStateLocked(ModuleState::UNLOADED, BLUETOOTH_POLICY);
        }

        if (ModuleState::INITIALLY_LOADED != initializer->m_discoverState) {
            initializer->updateStateLocked(ModuleState::UNLOADED, BLUETOOTH_DISCOVER);
        }

        initializer->m_mainThreadCv.notify_one();
        return;
    } else if (!info || !info->name) {
        ACSDK_ERROR(LX("moduleFoundFailed").d("reason", "invalidInfo"));
        return;
    }

    ACSDK_DEBUG9(LX(__func__).d("name", info->name));

    if (BLUETOOTH_POLICY == info->name) {
        initializer->updateStateLocked(ModuleState::INITIALLY_LOADED, BLUETOOTH_POLICY);
        pa_context_unload_module(context, info->index, &PulseAudioBluetoothInitializer::onUnloadPolicyResult, userdata);
    } else if (BLUETOOTH_DISCOVER == info->name) {
        initializer->updateStateLocked(ModuleState::INITIALLY_LOADED, BLUETOOTH_DISCOVER);
        pa_context_unload_module(
            context, info->index, &PulseAudioBluetoothInitializer::onUnloadDiscoverResult, userdata);
    }
}

bool PulseAudioBluetoothInitializer::updateStateLocked(const ModuleState& state, const std::string& module) {
    ModuleState currentState = ModuleState::UNKNOWN;

    if (BLUETOOTH_POLICY == module) {
        currentState = m_policyState;
        m_policyState = state;
    } else if (BLUETOOTH_DISCOVER == module) {
        currentState = m_discoverState;
        m_discoverState = state;
    } else {
        ACSDK_ERROR(LX("updateStateLockedFailed").d("reason", "invalidModule"));
        return false;
    }

    ACSDK_DEBUG5(LX(__func__).d("module", module).d("currentState", currentState).d("desiredState", state));
    return true;
}

void PulseAudioBluetoothInitializer::onStateChanged(pa_context* context, void* userdata) {
    ACSDK_DEBUG5(LX(__func__));
    if (!context) {
        ACSDK_ERROR(LX("onStateChangedFailed").d("reason", "nullContext"));
        return;
    } else if (!userdata) {
        ACSDK_ERROR(LX("onStateChangedFailed").d("reason", "nullUserData"));
        return;
    }

    pa_context_state_t state;
    state = pa_context_get_state(context);
    ACSDK_DEBUG5(LX(__func__).d("state", stateToString(state)));

    PulseAudioBluetoothInitializer* initializer = static_cast<PulseAudioBluetoothInitializer*>(userdata);
    initializer->setStateAndNotify(state);
}

void PulseAudioBluetoothInitializer::setStateAndNotify(pa_context_state_t state) {
    ACSDK_DEBUG5(LX(__func__));

    std::unique_lock<std::mutex> lock(m_mutex);

    switch (state) {
        // Connected and ready to receive calls.
        case PA_CONTEXT_READY:
            m_connected = true;
        // These are failed cases.
        case PA_CONTEXT_FAILED:
        case PA_CONTEXT_TERMINATED:
            m_mainThreadCv.notify_one();
            break;
        // Intermediate states that can be ignored.
        case PA_CONTEXT_UNCONNECTED:
        case PA_CONTEXT_CONNECTING:
        case PA_CONTEXT_AUTHORIZING:
        case PA_CONTEXT_SETTING_NAME:
        default:
            break;
    }
}

void PulseAudioBluetoothInitializer::cleanup() {
    ACSDK_DEBUG5(LX(__func__));

    if (m_context) {
        pa_context_disconnect(m_context);
        pa_context_unref(m_context);
        m_context = nullptr;
    }

    if (m_paLoop) {
        pa_threaded_mainloop_stop(m_paLoop);
        pa_threaded_mainloop_free(m_paLoop);
        m_paLoop = nullptr;
    }

    ACSDK_DEBUG(LX("cleanup").m("cleanupCompleted"));
}

PulseAudioBluetoothInitializer::~PulseAudioBluetoothInitializer() {
    ACSDK_DEBUG5(LX(__func__));

    // Ensure there are no references to PA resources being used.
    m_executor.shutdown();

    // cleanup() likely to have been previously called, but call again in case executor is shutdown prematurely.
    cleanup();
}

void PulseAudioBluetoothInitializer::run() {
    ACSDK_DEBUG5(LX(__func__));

    /*
     * pa_threaded_mainloop_new creates a separate thread that PulseAudio uses for callbacks.
     * Do this so we can block and wait on the main thread and terminate early on error conditions.
     */
    m_paLoop = pa_threaded_mainloop_new();
    // Owned by m_paLoop, do not need to free.
    pa_mainloop_api* mainLoopApi = pa_threaded_mainloop_get_api(m_paLoop);
    m_context = pa_context_new(mainLoopApi, "Application to unload and reload Pulse Audio BT modules");

    pa_context_set_state_callback(m_context, &PulseAudioBluetoothInitializer::onStateChanged, this);
    pa_context_connect(m_context, NULL, PA_CONTEXT_NOFLAGS, NULL);

    if (pa_threaded_mainloop_start(m_paLoop) < 0) {
        ACSDK_ERROR(LX("runFailed").d("reason", "runningMainLoopFailed"));
        cleanup();
        return;
    }

    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (std::cv_status::timeout == m_mainThreadCv.wait_for(lock, TIMEOUT) || !m_connected) {
            cleanup();
            return;
        }
    }

    /*
     * Get a list of modules. If we find module-bluetooth-discover and module-bluetooth-policy already loaded, we will
     * unload them.
     */
    pa_context_get_module_info_list(m_context, &PulseAudioBluetoothInitializer::onModuleFound, this);

    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_mainThreadCv.wait_for(lock, TIMEOUT, [this] {
                return ModuleState::UNLOADED == m_policyState && ModuleState::UNLOADED == m_discoverState;
            })) {
            ACSDK_DEBUG(LX(__func__).d("success", "bluetoothModulesUnloaded"));
        } else {
            ACSDK_ERROR(LX("runFailed").d("reason", "unloadModulesFailed"));
            cleanup();
            return;
        }
    }

    // (Re)load the modules.
    pa_context_load_module(
        m_context, BLUETOOTH_POLICY.c_str(), nullptr, &PulseAudioBluetoothInitializer::onLoadPolicyResult, this);
    pa_context_load_module(
        m_context, BLUETOOTH_DISCOVER.c_str(), nullptr, &PulseAudioBluetoothInitializer::onLoadDiscoverResult, this);

    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_mainThreadCv.wait_for(lock, TIMEOUT, [this] {
                return ModuleState::LOADED_BY_SDK == m_policyState && ModuleState::LOADED_BY_SDK == m_discoverState;
            })) {
            ACSDK_DEBUG(LX(__func__).d("reason", "loadModulesSuccesful"));
        } else {
            ACSDK_ERROR(LX("runFailed").d("reason", "loadModulesFailed"));
            cleanup();
            return;
        }
    }

    ACSDK_DEBUG(LX(__func__).m("Reloading PulseAudio Bluetooth Modules Successful"));

    cleanup();
}

void PulseAudioBluetoothInitializer::onEventFired(const BluetoothEvent& event) {
    ACSDK_DEBUG5(LX(__func__));

    if (BluetoothEventType::BLUETOOTH_DEVICE_MANAGER_INITIALIZED != event.getType()) {
        ACSDK_ERROR(LX("onEventFiredFailed").d("reason", "unexpectedEventReceived"));
        return;
    }

    m_executor.submit([this] {
        if (!m_paLoopStarted) {
            m_paLoopStarted = true;
            run();
        } else {
            ACSDK_WARN(LX(__func__).d("reason", "loopAlreadyStarted"));
        }
    });
}

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK
