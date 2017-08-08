#include "System/NotifyingMessageRequest.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {

using namespace avsCommon::avs;

NotifyingMessageRequest::NotifyingMessageRequest(
        const std::string& jsonContent,
        std::shared_ptr<StateSynchronizer> stateSynchronizer) :
    MessageRequest(jsonContent),
    m_stateSynchronizer{stateSynchronizer}
{
}

void NotifyingMessageRequest::onSendCompleted(MessageRequest::Status status) {
    m_stateSynchronizer->messageSent(status);
}

} // namespace system
} // namespace capabilityAgents
} // namespace alexaClientSDK
