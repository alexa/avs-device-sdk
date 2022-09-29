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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGER_INCLUDE_ACSDK_SPEAKERMANAGER_SPEAKERMANAGERSTORAGEINTERFACE_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGER_INCLUDE_ACSDK_SPEAKERMANAGER_SPEAKERMANAGERSTORAGEINTERFACE_H_

namespace alexaClientSDK {
namespace speakerManager {

struct SpeakerManagerStorageState;

/**
 * @brief Speaker manager storage interface.
 *
 * @see SpeakerManagerStorageState
 *
 * @ingroup Lib_acsdkSpeakerManager
 */
struct SpeakerManagerStorageInterface {
    /**
     * Virtual destructor to assure proper cleanup of derived types.
     */
    virtual ~SpeakerManagerStorageInterface() noexcept = default;

    /**
     * Loads state from underlying storage.
     * @param[out] state Pointer to state structure for loaded values.
     * @return Boolean flag if the operation is successful.
     */
    virtual bool loadState(SpeakerManagerStorageState& state) noexcept = 0;

    /**
     * Stores state to underlying storage.
     * @param[in] state Reference of state structure for values to store.
     * @return Boolean flag if the operation is successful.
     */
    virtual bool saveState(const SpeakerManagerStorageState& state) noexcept = 0;
};

}  // namespace speakerManager
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGER_INCLUDE_ACSDK_SPEAKERMANAGER_SPEAKERMANAGERSTORAGEINTERFACE_H_
