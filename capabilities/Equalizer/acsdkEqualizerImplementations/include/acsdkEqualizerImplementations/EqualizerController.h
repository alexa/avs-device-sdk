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

#ifndef ACSDKEQUALIZERIMPLEMENTATIONS_EQUALIZERCONTROLLER_H_
#define ACSDKEQUALIZERIMPLEMENTATIONS_EQUALIZERCONTROLLER_H_

#include <list>
#include <memory>
#include <mutex>
#include <set>

#include <acsdkEqualizerInterfaces/EqualizerConfigurationInterface.h>
#include <acsdkEqualizerInterfaces/EqualizerControllerListenerInterface.h>
#include <acsdkEqualizerInterfaces/EqualizerInterface.h>
#include <acsdkEqualizerInterfaces/EqualizerModeControllerInterface.h>
#include <acsdkEqualizerInterfaces/EqualizerStorageInterface.h>
#include <AVSCommon/Utils/Error/SuccessResult.h>

namespace alexaClientSDK {
namespace acsdkEqualizer {

/**
 * Class to control equalizer operations in the SDK. All the equalizer state manipulations and corresponding
 * notifications are performed within this class.
 */
class EqualizerController {
public:
    /**
     * Destructor.
     */
    ~EqualizerController() = default;

    /**
     * Factory method to create a new instance of @c EqualizerController.
     *
     * @param modeController Interface to handle mode changes. This parameter may be null if no modes are supported in
     * @c configuration.
     * @param configuration Interface to provide equalizer capabilities and configuration.
     * @param storage Interface to provide persistent storage for equalizer state.
     * @return A new instance of @c EqualizerController if all parameters are valid, @c nullptr otherwise.
     */
    static std::shared_ptr<EqualizerController> create(
        std::shared_ptr<acsdkEqualizerInterfaces::EqualizerModeControllerInterface> modeController,
        std::shared_ptr<acsdkEqualizerInterfaces::EqualizerConfigurationInterface> configuration,
        std::shared_ptr<acsdkEqualizerInterfaces::EqualizerStorageInterface> storage);

    /**
     * Returns current level of an equalizer band. Levels are in dB.
     *
     * @param band Equalizer band to get level of.
     * @return @c SuccessResult containing current level of the equalizer band in dB on success and undefined value
     * on failure.
     */
    avsCommon::utils::error::SuccessResult<int> getBandLevel(acsdkEqualizerInterfaces::EqualizerBand band);

    /**
     * Get levels for multiple bands. Levels are in dB.
     *
     * @param bands A set of bands to get levels for.
     * @return @c SuccessResult containing a map requested bands and their current levels on success and undefined
     * value on failure. Levels are in dB.
     */
    avsCommon::utils::error::SuccessResult<acsdkEqualizerInterfaces::EqualizerBandLevelMap> getBandLevels(
        std::set<acsdkEqualizerInterfaces::EqualizerBand> bands);

    /**
     * Set new level for an equalizer band. Level is in dB.
     * This method is not reenterable, calling it from the thread of listener or equalizer callback will cause deadlock.
     *
     * @param band Equalizer band to set level for.
     * @param level Level to set equalizer band to. Level is in dB.
     */
    void setBandLevel(acsdkEqualizerInterfaces::EqualizerBand band, int level);

    /**
     * Set levels for multiple equalizer bands. Levels are in dB.
     * This method is not reenterable, calling it from the thread of listener or equalizer callback will cause deadlock.
     * If unsupported bands/levels are provided, method will try to use supported ones only and will truncate levels
     * if needed.
     *
     * @param bandLevelMap A map of equalizer bands to be changed and their new levels. Levels are in dB.
     */
    void setBandLevels(const acsdkEqualizerInterfaces::EqualizerBandLevelMap& bandLevelMap);

    /**
     * Adjust levels of multiple equalizer bands. Levels are in dB.
     * This method is not reenterable, calling it from the thread of listener or equalizer callback will cause deadlock.
     * If unsupported bands are provided, method will try to use supported ones only. Adjustments leading to the levels
     * beyond the supported range will be truncated.
     *
     * @param bandAdjustmentMap A map of equalizer bands to be adjusted and relative change to be applied. Negative
     * values represent level decreasing. Levels adjustment values are in dB.
     */
    void adjustBandLevels(const acsdkEqualizerInterfaces::EqualizerBandLevelMap& bandAdjustmentMap);

    /**
     * Resets multiple equalizer bands to their default levels.
     * This method is not reenterable, calling it from the thread of listener or equalizer callback will cause deadlock.
     *
     * @param bands A set of bands whose level should be reset to default value.
     */
    void resetBands(const std::set<acsdkEqualizerInterfaces::EqualizerBand>& bands);

    /**
     * Returns equalizer mode currently applied to the device.
     *
     * @return Equalizer mode currently applied to the device.
     */
    acsdkEqualizerInterfaces::EqualizerMode getCurrentMode();

    /**
     * Sets a new equalizer mode.
     * This method is not reenterable, calling it from the thread of listener or equalizer callback will cause deadlock.
     *
     * @param mode A new equalizer mode to be applied.
     */
    void setCurrentMode(acsdkEqualizerInterfaces::EqualizerMode mode);

    /**
     * Returns current equalizer state.
     *
     * @return Current equalizer state.
     */
    acsdkEqualizerInterfaces::EqualizerState getCurrentState();

    /**
     * Returns the configuration equalizer initialized with.
     *
     * @return The configuration equalizer initialized with.
     */
    std::shared_ptr<acsdkEqualizerInterfaces::EqualizerConfigurationInterface> getConfiguration();

    /**
     * Registers an implementation of equalizer that modifies audio stream.
     *
     * @param equalizer An implementation of equalizer that modifies audio stream.
     */
    void registerEqualizer(std::shared_ptr<acsdkEqualizerInterfaces::EqualizerInterface> equalizer);

    /**
     * Unregisters an equalizer implementation.
     *
     * @param equalizer Equalizer implementation to remove.
     */
    void unregisterEqualizer(std::shared_ptr<acsdkEqualizerInterfaces::EqualizerInterface> equalizer);

    /**
     * Adds a listener for equalizer state changes.
     *
     * @param listener An implementation of equalizer state listener to add.
     */
    void addListener(std::shared_ptr<acsdkEqualizerInterfaces::EqualizerControllerListenerInterface> listener);

    /**
     * Removes an equalizer state changes listener from the list of listeners.
     *
     * @param listener An implementation of equalizer state listener to remove.
     */
    void removeListener(std::shared_ptr<acsdkEqualizerInterfaces::EqualizerControllerListenerInterface> listener);

private:
    /**
     * Constructor.
     *
     * @param modeController Interface to handle mode changes. This parameter may be null if no modes are supported in
     * @c configuration.
     * @param configuration Interface to provide equalizer capabilities and configuration.
     * @param storage Interface to provide persistent storage for equalizer state.
     */
    EqualizerController(
        std::shared_ptr<acsdkEqualizerInterfaces::EqualizerModeControllerInterface> modeController,
        std::shared_ptr<acsdkEqualizerInterfaces::EqualizerConfigurationInterface> configuration,
        std::shared_ptr<acsdkEqualizerInterfaces::EqualizerStorageInterface> storage);

    /**
     * Performs the actual equalizer state change. Applies changes to all equalizer implementations, then notifies
     * all listeners of the changes applied if any.
     */
    void updateState();

    /**
     * Applies transformation operation over the current equalizer state's band levels.
     *
     * @param changesDataMap @c EqualizerBandLevelMap containing changes data for required bands.
     * @param operation Modification operation to perform on current state's band value and a change. Operation accepts
     * original band level and data for the change to be applied and returns modified value.
     */

    void applyChangesToCurrentState(
        const acsdkEqualizerInterfaces::EqualizerBandLevelMap& changesDataMap,
        std::function<int(int, int)> operation);

    /**
     * Loads equalizer state from persistent storage.
     *
     * @return State retrieved from the persistent storage or default state if no or invalid state was restored.
     */
    acsdkEqualizerInterfaces::EqualizerState loadState();

    /**
     * Truncate band level to fit the supported range according to the current configuration.
     *
     * @param level level to truncate.
     * @return Truncated level.
     */
    int truncateBandLevel(int level);

    /**
     * Notify all listeners for equalizer state changes.
     *
     * @param changed band level changed or not.
     */
    void notifyListenersOnStateChanged(bool changed);

    /// Interface to handle equalizer mode changes.
    std::shared_ptr<acsdkEqualizerInterfaces::EqualizerModeControllerInterface> m_modeController;

    /// Configuration associated with the equalizer.
    std::shared_ptr<acsdkEqualizerInterfaces::EqualizerConfigurationInterface> m_configuration;

    /// Persistent storage to keep equalizer state in.
    std::shared_ptr<acsdkEqualizerInterfaces::EqualizerStorageInterface> m_storage;

    /// Current equalizer state.
    acsdkEqualizerInterfaces::EqualizerState m_currentState;

    /// Mutex to synchronize equalizer state access.
    std::mutex m_stateMutex;

    /// Mutex to synchronize equalizer mode changes.
    std::mutex m_modeMutex;

    /// A list of equalizer state change listeners.
    std::list<std::shared_ptr<acsdkEqualizerInterfaces::EqualizerControllerListenerInterface>> m_listeners;

    /// A list of equalizer implementations that apply equalization to audio stream.
    std::list<std::shared_ptr<acsdkEqualizerInterfaces::EqualizerInterface>> m_equalizers;
};

}  // namespace acsdkEqualizer
}  // namespace alexaClientSDK

#endif  // ACSDKEQUALIZERIMPLEMENTATIONS_EQUALIZERCONTROLLER_H_
