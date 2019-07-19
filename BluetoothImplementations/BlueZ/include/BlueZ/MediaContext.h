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

#ifndef ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_MEDIACONTEXT_H_
#define ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_MEDIACONTEXT_H_

#include <sbc/sbc.h>

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

/**
 * Media streaming context. Contains the data configured during the arbitrary invocation of BlueZ
 * MediaEndpoint1::SetConfiguration callback.
 */
class MediaContext {
public:
    /// Invalid file descriptor
    static constexpr int INVALID_FD = -1;

    /**
     * Destructor
     */
    ~MediaContext();

    /**
     * Constructor
     */
    MediaContext();

    /**
     * Sets a linux file descriptor that should be used for read/write operations.
     * @param streamFD linux file descriptor that should be used for read/write operations.
     */
    void setStreamFD(int streamFD);

    /**
     * Get linux file descriptor that should be used for read/write operations.
     * @return linux file descriptor that should be used for read/write operations.
     */
    int getStreamFD();

    /**
     * Set Maximum Transfer Unit for read operations.
     * @param readMTU Maximum Transfer Unit for read operations.
     */
    void setReadMTU(int readMTU);

    /**
     * Get Maximum Transfer Unit for read operations
     * @return Maximum Transfer Unit for read operations
     */
    int getReadMTU();

    /**
     * Set Maximum Transfer Unit for write operations.
     * @param writeMTU Maximum Transfer Unit for write operations.
     */
    void setWriteMTU(int writeMTU);

    /**
     * Get Maximum Transfer Unit for write operations.
     * @return Maximum Transfer Unit for write operations.
     */
    int getWriteMTU();

    /**
     * Get pointer to SBC decoder context object
     * @return The pointer to SBC decoder context object
     */
    sbc_t* getSBCContextPtr();

    /**
     * Method to check if SBC decoder context has been successfully initialized.
     * @return true if if SBC decoder context has been successfully initialized, false otherwise.
     */
    bool isSBCInitialized();

    /**
     * Marks the SBC decoder context as successfully initialized.
     * @param initialized true if SBC decoder context has been successfully initialized, false otherwise.
     */
    void setSBCInitialized(bool initialized);

private:
    /**
     * Linux file descriptor used to read audio stream data from. The descriptor is provided by BlueZ.
     */
    int m_mediaStreamFD;

    /**
     * Maximum bytes expected to be in one packet for inbound stream.
     */
    int m_readMTU;

    /**
     * Maximum bytes to be sent in one packet for outbound stream. Reserved for future use.
     */
    int m_writeMTU;

    /**
     * libsbc structure containing the context for SBC decoder.
     */
    sbc_t m_sbcContext;

    /**
     * Flag indicating if SBC decoding has been initialized.
     */
    bool m_isSBCInitialized;
};

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_MEDIACONTEXT_H_
