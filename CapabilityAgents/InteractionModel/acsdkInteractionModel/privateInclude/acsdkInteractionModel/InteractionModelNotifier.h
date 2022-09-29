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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_INTERACTIONMODEL_ACSDKINTERACTIONMODEL_PRIVATEINCLUDE_ACSDKINTERACTIONMODEL_INTERACTIONMODELNOTIFIER_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_INTERACTIONMODEL_ACSDKINTERACTIONMODEL_PRIVATEINCLUDE_ACSDKINTERACTIONMODEL_INTERACTIONMODELNOTIFIER_H_

#include <memory>

#include <acsdkInteractionModelInterfaces/InteractionModelNotifierInterface.h>
#include <acsdkInteractionModelInterfaces/InteractionModelRequestProcessingObserverInterface.h>
#include <acsdk/Notifier/Notifier.h>

namespace alexaClientSDK {
namespace acsdkInteractionModel {

/**
 * Relays notifications related to acsdkInteractionModel.
 */
class InteractionModelNotifier
        : public notifier::Notifier<
              acsdkInteractionModelInterfaces::InteractionModelRequestProcessingObserverInterface> {
public:
    /*
     * Factory method.
     * @return A new instance of @c InteractionModelNotifierInterface.
     */
    static std::shared_ptr<acsdkInteractionModelInterfaces::InteractionModelNotifierInterface>
    createInteractionModelNotifierInterface();
};

}  // namespace acsdkInteractionModel
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_INTERACTIONMODEL_ACSDKINTERACTIONMODEL_PRIVATEINCLUDE_ACSDKINTERACTIONMODEL_INTERACTIONMODELNOTIFIER_H_
