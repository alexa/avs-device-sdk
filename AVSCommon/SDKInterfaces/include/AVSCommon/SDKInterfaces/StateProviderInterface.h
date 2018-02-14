/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_STATEPROVIDERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_STATEPROVIDERINTERFACE_H_

#include "AVSCommon/AVS/NamespaceAndName.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * A @c StateProvider may be any client component whose state needs to be sent to the AVS.
 * This specifies the interface to a @c StateProvider.
 */
class StateProviderInterface {
public:
    /**
     * Destructor.
     */
    virtual ~StateProviderInterface() = default;

    /**
     * A request to a @c StateProvider to provide the state. The @c StateProvider should perform minimum processing
     * and return quickly, otherwise it will block the processing of updating the states of other @c StateProviders.
     * The @c ContextManager specifies a token which it uses to track the @c getContext request associated with this
     * @c provideState request. The @c stateProviderInterface must use the same token when it updates its state via the
     * @c setState call.
     *
     * @param stateProviderName The name of the state provider.
     * @param stateRequestToken The token to use in the @c setState call.
     */
    virtual void provideState(const avs::NamespaceAndName& stateProviderName, const unsigned int stateRequestToken) = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_STATEPROVIDERINTERFACE_H_
