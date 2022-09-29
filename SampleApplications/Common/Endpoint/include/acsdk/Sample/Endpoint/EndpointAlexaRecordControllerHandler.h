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

#ifndef ACSDK_SAMPLE_ENDPOINT_ENDPOINTALEXARECORDCONTROLLERHANDLER_H_
#define ACSDK_SAMPLE_ENDPOINT_ENDPOINTALEXARECORDCONTROLLERHANDLER_H_

#include <memory>
#include <mutex>
#include <string>

#include <acsdk/AlexaRecordControllerInterfaces/RecordControllerInterface.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace common {

/**
 * Sample implementation of a @c RecordControllerInterface.
 */

class EndpointAlexaRecordControllerHandler : public alexaRecordControllerInterfaces::RecordControllerInterface {
public:
    /**
     * Create a @c EndpointAlexaRecordController object.
     *
     * @param endpointName The name of the endpoint.
     * @return A pointer to a new @c EndpointAlexaRecordController object if it succeeds; otherwise, @c nullptr.
     */
    static std::shared_ptr<EndpointAlexaRecordControllerHandler> create(std::string endpointName);

    /// @name RecordControllerInterface methods
    /// @{
    virtual alexaRecordControllerInterfaces::RecordControllerInterface::Response startRecording() override;
    virtual alexaRecordControllerInterfaces::RecordControllerInterface::Response stopRecording() override;
    virtual bool isRecording() override;
    /// @}

private:
    /**
     * Constructor.
     * @param endpointName The name of the endpoint the capability is associated.
     */
    EndpointAlexaRecordControllerHandler(std::string endpointName);

    /// Mutex to serialize access to variables.
    std::mutex m_mutex;

    /// The name of the endpoint that this controller is associated.
    std::string m_endpointName;

    /// The current recording state
    bool m_isRecording;
};

}  // namespace common
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ACSDK_SAMPLE_ENDPOINT_ENDPOINTALEXARECORDCONTROLLERHANDLER_H_
