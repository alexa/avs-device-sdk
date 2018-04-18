/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CALLMANAGERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CALLMANAGERINTERFACE_H_

#include <memory>
#include <string>

#include <AVSCommon/SDKInterfaces/CallStateObserverInterface.h>
#include <AVSCommon/SDKInterfaces/ConnectionStatusObserverInterface.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include "AVSCommon/Utils/RequiresShutdown.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * This class provides an interface to the @c CallManager.
 */
class CallManagerInterface
        : public utils::RequiresShutdown
        , public avsCommon::avs::CapabilityAgent
        , public avsCommon::sdkInterfaces::ConnectionStatusObserverInterface {
public:
    /**
     * Constructor.
     *
     * @param objectName The name of the class or object which requires shutdown calls.  Used in log messages when
     * problems are detected in shutdown or destruction sequences.
     * @param avsNamespace The namespace of the CapabilityAgent.
     * @param exceptionEncounteredSender Object to use to send Exceptions to AVS.
     */
    CallManagerInterface(
        const std::string& objectName,
        const std::string& avsNamespace,
        std::shared_ptr<sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender);

    /**
     * Destructor
     */
    virtual ~CallManagerInterface() = default;

    /**
     * Adds a CallStateObserverInterface to the group of observers.
     *
     * @param observer The observer to add.
     */
    virtual void addObserver(std::shared_ptr<avsCommon::sdkInterfaces::CallStateObserverInterface> observer) = 0;

    /**
     * Removes a CallStateObserverInterface from the group of observers.
     *
     * @param observer The observer to remove.
     */
    virtual void removeObserver(std::shared_ptr<avsCommon::sdkInterfaces::CallStateObserverInterface> observer) = 0;

    /**
     * Accepts an incoming call.
     */
    virtual void acceptCall() = 0;

    /**
     * Stops the call.
     */
    virtual void stopCall() = 0;
};

inline CallManagerInterface::CallManagerInterface(
    const std::string& objectName,
    const std::string& avsNamespace,
    std::shared_ptr<sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender) :
        utils::RequiresShutdown{objectName},
        avsCommon::avs::CapabilityAgent{avsNamespace, exceptionEncounteredSender} {
}

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CALLMANAGERINTERFACE_H_
