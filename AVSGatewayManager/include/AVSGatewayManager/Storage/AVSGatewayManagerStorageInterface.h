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

#ifndef ALEXA_CLIENT_SDK_AVSGATEWAYMANAGER_INCLUDE_AVSGATEWAYMANAGER_STORAGE_AVSGATEWAYMANAGERSTORAGEINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSGATEWAYMANAGER_INCLUDE_AVSGATEWAYMANAGER_STORAGE_AVSGATEWAYMANAGERSTORAGEINTERFACE_H_

#include <AVSGatewayManager/GatewayVerifyState.h>

namespace alexaClientSDK {
namespace avsGatewayManager {
namespace storage {

/**
 * The interface class for @c AVSGatewayManager Storage.
 */
class AVSGatewayManagerStorageInterface {
public:
    /**
     * Destructor.
     */
    virtual ~AVSGatewayManagerStorageInterface() = default;

    /**
     * Initializes the underlying database.
     *
     * @return True if successful, else false.
     */
    virtual bool init() = 0;

    /**
     * Loads the @c GatewayVerifyState from the database.
     *
     * @param state The pointer to the @c GatewayVerifyState that will populated. If the default state is passed and
     * there is no data to be loaded, the state remains unchanged.
     * @return True if load is successful, else false.
     */
    virtual bool loadState(GatewayVerifyState* state) = 0;

    /**
     * Saves the given state to the database.
     *
     * @param state The @c GatewayVerifyState to be saved.
     * @return True if successful, else false.
     */
    virtual bool saveState(const GatewayVerifyState& state) = 0;

    /**
     * Clears the stored data from the database.
     */
    virtual void clear() = 0;
};

}  // namespace storage
}  // namespace avsGatewayManager
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSGATEWAYMANAGER_INCLUDE_AVSGATEWAYMANAGER_STORAGE_AVSGATEWAYMANAGERSTORAGEINTERFACE_H_
