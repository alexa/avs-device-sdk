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

#include "acsdkAssetsCommon/DataChunk.h"

#include <cstring>
#include <memory>

namespace alexaClientSDK {
namespace acsdkAssets {
namespace common {

DataChunk::DataChunk(char* data, size_t size) {
    if (data != nullptr && size > 0) {
        m_data = static_cast<char*>(operator new(size));
        memcpy(m_data, data, size);
        m_size = size;
    } else {
        m_data = nullptr;
        m_size = 0;
    }
}

DataChunk::~DataChunk() {
    if (m_data != nullptr) {
        operator delete(m_data);
        m_data = nullptr;
        m_size = 0;
    }
}

size_t DataChunk::size() const {
    return m_size;
}

char* DataChunk::data() const {
    return m_data;
}

}  // namespace common
}  // namespace acsdkAssets
}  // namespace alexaClientSDK
