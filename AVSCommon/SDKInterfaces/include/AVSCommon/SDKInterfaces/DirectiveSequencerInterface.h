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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_DIRECTIVESEQUENCERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_DIRECTIVESEQUENCERINTERFACE_H_

#include <memory>
#include <string>

#include "AVSCommon/AVS/AVSDirective.h"
#include "AVSCommon/SDKInterfaces/DirectiveHandlerInterface.h"
#include "AVSCommon/Utils/RequiresShutdown.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * Interface for sequencing and handling a stream of @c AVSDirective instances.
 *
 * Customers of this interface specify a mapping of @c AVSDirectives specified by (namespace, name) pairs to
 * instances of the @c AVSDirectiveHandlerInterface via calls to @c setDirectiveHandlers(). Changes to this
 * mapping can be made at any time by specifying a new mapping. Customers pass @c AVSDirectives in to this
 * interface for processing via calls to @c onDirective(). @c AVSDirectives are processed in the order that
 * they are received. @c AVSDirectives with a non-empty @c dialogRequestId value are filtered by the
 * @c DirectiveSequencer's current @c dialogRequestId value (specified by calls to @c setDialogRequestId()).
 * Only @c AVSDirectives with a @c dialogRequestId that is empty or which matches the last setting of the
 * @c dialogRequestId are handled. All others are ignored. Specifying a new @c DialogRequestId value while
 * @c AVSDirectives are already being handled will cancel the handling of @c AVSDirectives that have the
 * previous @c DialogRequestId and whose handling has not completed.
 */
class DirectiveSequencerInterface : public utils::RequiresShutdown {
public:
    /**
     * Constructor.
     *
     * @param name The name of the class or object which requires shutdown calls.  Used in log messages when problems
     *    are detected in shutdown or destruction sequences.
     */
    DirectiveSequencerInterface(const std::string& name);

    /**
     * Destructor.
     */
    virtual ~DirectiveSequencerInterface() = default;

    /**
     * Add the specified handler as a handler for its specified namespace, name, and policy. Note that implmentations
     * of this should call the handler's getConfiguration() method to get the namespace(s), name(s), and policy(ies) of
     * the handler. If any of the mappings fail, the entire call is refused.
     *
     * @param handler The handler to add.
     * @return Whether the handler was added.
     */
    virtual bool addDirectiveHandler(std::shared_ptr<DirectiveHandlerInterface> handler) = 0;

    /**
     * Remove the specified handler's mapping of @c NamespaceAndName to @c BlockingPolicy values. Note that
     * implementations of this should call the handler's getConfiguration() method to get the namespace(s), name(s), and
     * policy(ies) of the handler. If the handler's configurations are unable to be removed, the entire operation is
     * refused.
     *
     * @param handler The handler to remove.
     * @return Whether the handler was removed.
     */
    virtual bool removeDirectiveHandler(std::shared_ptr<DirectiveHandlerInterface> handler) = 0;

    /**
     * Set the current @c DialogRequestId. This value can be set at any time. Setting this value causes a
     * @c DirectiveSequencer to drop unhandled @c AVSDirectives with different (and non-empty) DialogRequestId
     * values.  @c AVSDirectives with a differing @c dialogRequestId value and whose pre-handling or handling
     * is already in progress will be cancelled.
     *
     * @param dialogRequestId The new value for the current @c DialogRequestId.
     */
    virtual void setDialogRequestId(const std::string& dialogRequestId) = 0;

    /**
     * Sequence the handling of an @c AVSDirective.  The actual handling is done by whichever @c DirectiveHandler
     * is associated with the @c AVSDirective's (namespace, name) pair.
     *
     * @param directive The @c AVSDirective to handle.
     * @return Whether or not the directive was accepted.
     */
    virtual bool onDirective(std::shared_ptr<avsCommon::avs::AVSDirective> directive) = 0;

    /**
     * Disable the DirectiveSequencer.
     *
     * @note While disabled the DirectiveSequencer should not be able to handle directives.
     */
    virtual void disable() = 0;

    /**
     * Enable the DirectiveSequencer.
     */
    virtual void enable() = 0;
};

inline DirectiveSequencerInterface::DirectiveSequencerInterface(const std::string& name) :
        utils::RequiresShutdown{name} {
}

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_DIRECTIVESEQUENCERINTERFACE_H_
