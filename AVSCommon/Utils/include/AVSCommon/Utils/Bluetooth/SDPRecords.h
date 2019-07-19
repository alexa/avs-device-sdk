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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_BLUETOOTH_SDPRECORDS_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_BLUETOOTH_SDPRECORDS_H_

#include <string>

#include "AVSCommon/SDKInterfaces/Bluetooth/Services/SDPRecordInterface.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace bluetooth {

/// Base class for an SDPRecord object used.
class SDPRecord : public sdkInterfaces::bluetooth::services::SDPRecordInterface {
public:
    /**
     * Constructor.
     *
     * @param name The Bluetooth Service name.
     * @param uuid The service UUID.
     * @param version The service version.
     */
    SDPRecord(const std::string& name, const std::string& uuid, const std::string& version);

    /**
     * Gets the service name.
     *
     * @return The service name.
     */
    std::string getName() const override;

    /**
     * Gets the full 128-bit service UUID.
     */
    std::string getUuid() const override;

    /**
     * Gets the version of the service.
     */
    std::string getVersion() const override;

protected:
    /// The service name.
    const std::string m_name;

    /// The 128-bit UUID.
    const std::string m_uuid;

    /// The version.
    const std::string m_version;
};

/// A SDP record representing A2DPSource.
class A2DPSourceRecord : public SDPRecord {
public:
    /**
     * Constructor
     *
     * @param version The version of the service.
     */
    A2DPSourceRecord(const std::string& version);
};

/// A SDP record representing A2DPSink.
class A2DPSinkRecord : public SDPRecord {
public:
    /**
     * Constructor
     *
     * @param version The version of the service.
     */
    A2DPSinkRecord(const std::string& version);
};

/// A SDP record representing AVRCPTarget.
class AVRCPTargetRecord : public SDPRecord {
public:
    /**
     * Constructor
     *
     * @param version The version of the service.
     */
    AVRCPTargetRecord(const std::string& version);
};

/// A SDP record representing AVRCPController.
class AVRCPControllerRecord : public SDPRecord {
public:
    /**
     * Constructor
     *
     * @param version The version of the service.
     */
    AVRCPControllerRecord(const std::string& version);
};

}  // namespace bluetooth
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_BLUETOOTH_SDPRECORDS_H_
