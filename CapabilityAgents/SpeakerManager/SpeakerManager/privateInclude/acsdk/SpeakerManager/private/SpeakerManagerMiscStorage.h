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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGER_PRIVATEINCLUDE_ACSDK_SPEAKERMANAGER_PRIVATE_SPEAKERMANAGERMISCSTORAGE_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGER_PRIVATEINCLUDE_ACSDK_SPEAKERMANAGER_PRIVATE_SPEAKERMANAGERMISCSTORAGE_H_

#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>
#include <acsdk/SpeakerManager/SpeakerManagerStorageInterface.h>
#include <acsdk/SpeakerManager/SpeakerManagerStorageState.h>

namespace alexaClientSDK {
namespace speakerManager {

/**
 * @brief Configuration interface implementation for speaker manager.
 *
 * This class adapts @c MiscStorageInterface to @c SpeakerManagerStorageInterface.
 *
 * @ingroup Lib_acsdkSpeakerManager
 */
class SpeakerManagerMiscStorage : public SpeakerManagerStorageInterface {
public:
    /**
     * Creates an instance of the @c SpeakerManagerMiscStorage.
     *
     * @param miscStorage The underlying miscellaneous storage to store @c SpeakerManager data.
     * @return A unique pointer to the instance of the newly created @c SpeakerManagerMiscStorage.
     */
    static std::shared_ptr<SpeakerManagerMiscStorage> create(
        std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> miscStorage) noexcept;

    /// @name SpeakerManagerStorageInterface Functions
    /// @{
    bool loadState(SpeakerManagerStorageState& state) noexcept override;
    bool saveState(const SpeakerManagerStorageState& state) noexcept override;
    /// @}

private:
    /// Helper to convert structure to JSON string.
    static std::string convertToStateString(const SpeakerManagerStorageState& state) noexcept;
    static std::string convertToStateString(const SpeakerManagerStorageState::ChannelState& state) noexcept;

    /**
     * Constructor.
     *
     * @param miscStorage The underlying miscellaneous storage used to store component data.
     */
    SpeakerManagerMiscStorage(
        std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> miscStorage) noexcept;

    /**
     * Method to initialize the object.
     *
     * This method connects to underlying storage and performs necessary actions.
     *
     * @return Boolean status, indicating operation success.
     */
    bool init() noexcept;

    /**
     * Helper to convert JSON string to structure.
     *
     * @param stateString JSON string describing @a SpeakerManagerStorageState data.
     * @param state Pointer to storage for parsed values.
     *
     * @return Boolean status, indicating operation success.
     */
    bool convertFromStateString(const std::string& stateString, SpeakerManagerStorageState& state) noexcept;

    /**
     * Helper to convert JSON string to structure.
     *
     * @param stateString JSON string describing @a SpeakerManagerStorageState::ChannelState data.
     * @param state Pointer to storage for parsed values.
     *
     * @return Boolean status, indicating operation success.
     */
    bool convertFromStateString(
        const std::string& stateString,
        SpeakerManagerStorageState::ChannelState& state) noexcept;

    /// The Misc storage.
    std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> m_miscStorage;
};

}  // namespace speakerManager
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGER_PRIVATEINCLUDE_ACSDK_SPEAKERMANAGER_PRIVATE_SPEAKERMANAGERMISCSTORAGE_H_
