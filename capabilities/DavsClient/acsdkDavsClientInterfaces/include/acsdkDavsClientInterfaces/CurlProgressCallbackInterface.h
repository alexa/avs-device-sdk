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

#ifndef ACSDKDAVSCLIENTINTERFACES_CURLPROGRESSCALLBACKINTERFACE_H_
#define ACSDKDAVSCLIENTINTERFACES_CURLPROGRESSCALLBACKINTERFACE_H_

namespace amazon {
namespace davs {

class CurlProgressCallbackInterface {
public:
    virtual ~CurlProgressCallbackInterface() = default;

    /**
     * An event that is called with a frequent interval. While data is being transferred it will be
     * called very frequently, and during slow periods like when nothing is being transferred it can
     * slow down to about one call per second.
     *
     * @param dlTotal, total bytes need to be downloaded, 0 if Unknown/unused
     * @param dlNow, number of bytes downloaded so far, 0 if Unknown/unused
     * @param ulTotal, total bytes need to be uploaded, 0 if Unknown/unused
     * @param ulNow, number of bytes uploaded so far, 0 if Unknown/unused
     * @return Returning false will cause libcurl to abort the transfer and return
     */
    virtual bool onProgressUpdate(long dlTotal, long dlNow, long ulTotal, long ulNow) = 0;
};

}  // namespace davs
}  // namespace amazon

#endif  // ACSDKDAVSCLIENTINTERFACES_CURLPROGRESSCALLBACKINTERFACE_H_
