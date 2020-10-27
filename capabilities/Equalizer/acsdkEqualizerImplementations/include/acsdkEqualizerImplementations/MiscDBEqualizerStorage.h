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

#ifndef ACSDKEQUALIZERIMPLEMENTATIONS_MISCDBEQUALIZERSTORAGE_H_
#define ACSDKEQUALIZERIMPLEMENTATIONS_MISCDBEQUALIZERSTORAGE_H_

#include <memory>
#include <mutex>

#include <acsdkEqualizerInterfaces/EqualizerStorageInterface.h>
#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>
#include <AVSCommon/Utils/Error/SuccessResult.h>

namespace alexaClientSDK {
namespace acsdkEqualizer {

/**
 * An implementation of @c EqualizerStorageInterface that uses @c MiscStorageInterface as an underlying storage.
 */
class MiscDBEqualizerStorage : public acsdkEqualizerInterfaces::EqualizerStorageInterface {
public:
    /**
     * Factory method to create an instance of @c EqualizerStorageInterface given the instance @c MiscStorageInterface.
     *
     * @param storage An instance of @c MiscStorageInterface to use as an underlying storage.
     * @return An instance of @c EqualizerStorageInterface on success, @c nullptr otherwise.
     */
    static std::shared_ptr<EqualizerStorageInterface> createEqualizerStorageInterface(
        const std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface>& storage);

    /**
     * Factory method to create an instance of @c MiscDBEqualizerStorage given the instance @c MiscStorageInterface.
     *
     * @deprecated Use createEqualizerStorageInterface.
     * @param storage An instance of @c MiscStorageInterface to use as an underlying storage.
     * @return An instance of @c MiscDBEqualizerStorage on success, @c nullptr otherwise.
     */
    static std::shared_ptr<MiscDBEqualizerStorage> create(
        std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> storage);

    /// @name EqualizerStorageInterface functions.
    /// @{
    void saveState(const acsdkEqualizerInterfaces::EqualizerState& state) override;

    avsCommon::utils::error::SuccessResult<acsdkEqualizerInterfaces::EqualizerState> loadState() override;

    void clear() override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param storage An instance of @c MiscStorageInterface to use as an underlying storage.
     */
    MiscDBEqualizerStorage(std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> storage);

    /**
     * Initializes the underlying storage and prepares instance for usage.
     *
     * @return True if initalization succeeded, false otherwise.
     */
    bool initialize();

    /**
     * An instance of @c MiscStorageInterface to use as an underlying storage.
     */
    std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> m_miscStorage;
};

}  // namespace acsdkEqualizer
}  // namespace alexaClientSDK

#endif  // ACSDKEQUALIZERIMPLEMENTATIONS_MISCDBEQUALIZERSTORAGE_H_
