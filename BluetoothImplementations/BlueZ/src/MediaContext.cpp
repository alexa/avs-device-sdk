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

#include <gio/gio.h>

#include "BlueZ/MediaContext.h"

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

MediaContext::~MediaContext() {
    if (m_mediaStreamFD != INVALID_FD) {
        close(m_mediaStreamFD);
    }
    if (m_isSBCInitialized) {
        sbc_finish(&m_sbcContext);
    }
}

MediaContext::MediaContext() : m_mediaStreamFD{INVALID_FD}, m_readMTU{0}, m_writeMTU{0}, m_isSBCInitialized{false} {
}

void MediaContext::setStreamFD(int streamFD) {
    m_mediaStreamFD = streamFD;
}

int MediaContext::getStreamFD() {
    return m_mediaStreamFD;
}

void MediaContext::setReadMTU(int readMTU) {
    m_readMTU = readMTU;
}

int MediaContext::getReadMTU() {
    return m_readMTU;
}

void MediaContext::setWriteMTU(int writeMTU) {
    m_writeMTU = writeMTU;
}

int MediaContext::getWriteMTU() {
    return m_writeMTU;
}

sbc_t* MediaContext::getSBCContextPtr() {
    return &m_sbcContext;
}

bool MediaContext::isSBCInitialized() {
    return m_isSBCInitialized;
}

void MediaContext::setSBCInitialized(bool initialized) {
    m_isSBCInitialized = initialized;
}

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK
