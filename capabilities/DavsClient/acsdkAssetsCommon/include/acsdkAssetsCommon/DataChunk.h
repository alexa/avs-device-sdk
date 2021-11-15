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

#ifndef ACSDKASSETSCOMMON_DATACHUNK_H_
#define ACSDKASSETSCOMMON_DATACHUNK_H_

#include <cstdio>

namespace alexaClientSDK {
namespace acsdkAssets {
namespace common {

/**
 * Represent a binary data chunk
 */
class DataChunk {
public:
    /**
     * Constructor to DataChunk object
     * @param data binary data to copy from
     * @param size number of bytes
     */
    DataChunk(char* data, size_t size);

    ~DataChunk();

    /// number of bytes in the data chunk
    size_t size() const;

    char* data() const;

private:
    // number of bytes in the data chunk
    size_t m_size;
    /// pointer to the binary data
    char* m_data;
};

}  // namespace common
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKASSETSCOMMON_DATACHUNK_H_
