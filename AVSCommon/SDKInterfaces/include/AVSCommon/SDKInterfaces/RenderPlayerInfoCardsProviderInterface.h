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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_RENDERPLAYERINFOCARDSPROVIDERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_RENDERPLAYERINFOCARDSPROVIDERINTERFACE_H_

#include <memory>

#include <AVSCommon/SDKInterfaces/RenderPlayerInfoCardsObserverInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * This class provides an interface to set the provider of the @c RenderPlayerInfoCardsObserverInterface.
 */
class RenderPlayerInfoCardsProviderInterface {
public:
    /**
     * Destructor
     */
    virtual ~RenderPlayerInfoCardsProviderInterface() = default;

    /**
     * This function sets an @c RenderPlayerInfoCardsObserverInterface so that it will get notified for
     * RenderPlayerInfoCards state changes.  This implies that there can be one or no observer at a given time.
     *
     * @param observer The @c RenderPlayerInfoCardsObserverInterface
     */
    virtual void setObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsObserverInterface> observer) = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_RENDERPLAYERINFOCARDSPROVIDERINTERFACE_H_
