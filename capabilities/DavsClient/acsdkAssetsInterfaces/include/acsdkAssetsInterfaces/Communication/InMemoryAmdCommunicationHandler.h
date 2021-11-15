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

#ifndef ACSDKASSETSINTERFACES_COMMUNICATION_INMEMORYAMDCOMMUNICATIONHANDLER_H_
#define ACSDKASSETSINTERFACES_COMMUNICATION_INMEMORYAMDCOMMUNICATIONHANDLER_H_
#include <acsdkCommunication/InMemoryCommunicationInvokeHandler.h>
#include <acsdkCommunication/InMemoryCommunicationPropertiesHandler.h>
#include "AmdCommunicationInterface.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace commonInterfaces {

class InMemoryAmdCommunicationHandler
        : public virtual AmdCommunicationInterface
        , public acsdkCommunication::InMemoryCommunicationPropertiesHandler<std::string>
        , public acsdkCommunication::InMemoryCommunicationPropertiesHandler<int>
        , public acsdkCommunication::InMemoryCommunicationInvokeHandler<std::string>
        , public acsdkCommunication::InMemoryCommunicationInvokeHandler<bool, std::string> {
public:
    ~InMemoryAmdCommunicationHandler() override = default;

    static std::shared_ptr<AmdCommunicationInterface> create() {
        return std::shared_ptr<AmdCommunicationInterface>(new InMemoryAmdCommunicationHandler());
    }

private:
    InMemoryAmdCommunicationHandler() = default;
};

}  // namespace commonInterfaces
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKASSETSINTERFACES_COMMUNICATION_INMEMORYAMDCOMMUNICATIONHANDLER_H_
