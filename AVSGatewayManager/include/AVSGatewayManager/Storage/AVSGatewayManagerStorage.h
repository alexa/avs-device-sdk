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

#ifndef ALEXA_CLIENT_SDK_AVSGATEWAYMANAGER_INCLUDE_AVSGATEWAYMANAGER_STORAGE_AVSGATEWAYMANAGERSTORAGE_H_
#define ALEXA_CLIENT_SDK_AVSGATEWAYMANAGER_INCLUDE_AVSGATEWAYMANAGER_STORAGE_AVSGATEWAYMANAGERSTORAGE_H_

#include <memory>

#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSGatewayManager/Storage/AVSGatewayManagerStorageInterface.h>

namespace alexaClientSDK {
namespace avsGatewayManager {
namespace storage {

/**
 * Wrapper to the MiscStorageInterface used by the @c AVSGatewayManager to store gateway verification state information.
 */
class AVSGatewayManagerStorage : public AVSGatewayManagerStorageInterface {
public:
    /**
     * Creates an instance of the @c AVSGatewayManagerStorage.
     *
     * @param miscStorage The underlying miscellaneous storage to store gateway verification data.
     * @return A unique pointer to the instance of the newly created @c AVSGatewayManagerStorage.
     */
    static std::unique_ptr<AVSGatewayManagerStorage> create(
        std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> miscStorage);

    /// @name AVSGatewayManagerStorageInterface Functions
    /// @{
    bool init() override;
    bool loadState(GatewayVerifyState* state) override;
    bool saveState(const GatewayVerifyState& state) override;
    void clear() override;
    /// @}
private:
    /**
     * Constructor.
     *
     * @param miscStorage The underlying miscellaneous storage used to store gateway verification data.
     */
    AVSGatewayManagerStorage(std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> miscStorage);

    /// The Misc storage.
    std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> m_miscStorage;
};

}  // namespace storage
}  // namespace avsGatewayManager
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSGATEWAYMANAGER_INCLUDE_AVSGATEWAYMANAGER_STORAGE_AVSGATEWAYMANAGERSTORAGE_H_
