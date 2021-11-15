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

#ifndef ACSDKASSETSINTERFACES_DAVSREQUEST_H_
#define ACSDKASSETSINTERFACES_DAVSREQUEST_H_

#include <map>
#include <set>

#include "ArtifactRequest.h"
#include "Region.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace commonInterfaces {

class DavsRequest : public ArtifactRequest {
public:
    using FilterMap = std::map<std::string, std::set<std::string>>;

    /**
     * Creates an Artifact Request that will contain all the necessary information to identify an artifact with DAVS.
     * API information: https://wiki.labcollab.net/confluence/display/Doppler/DAVS+2.0+GET+ARTIFACT+API
     *
     * @param type REQUIRED, used to identify a family of artifact (WW, earcon, alarms...).
     * @param key REQUIRED, used to narrow down the scope per type, for WW (alexa, amazon), for earcons (tones,
     * local-dependant), etc...
     * @param filters REQUIRED, extra filters that are flexible in number (Locale, compatibility versions, etc...).
     * @param endpoint OPTIONAL, specifies the endpoint for the request to download from, defaults to NA.
     * @param unpack OPTIONAL, if true, then artifact will be unpacked and the directory will be provided.
     * @return NULLABLE, a smart pointer to a request if all params are valid.
     */
    static std::shared_ptr<DavsRequest> create(
            std::string type,
            std::string key,
            FilterMap filters,
            Region endpoint = Region::NA,
            bool unpack = false);

    /**
     * @return the Type which is used to identify the main component of this DAVS request.
     */
    const std::string& getType() const;

    /**
     * @return the Key which is used to identify the subcomponent of this DAVS request.
     */
    const std::string& getKey() const;

    /**
     * @return the map of filter sets used to distinguish this DAVS request from similar components.
     */
    const FilterMap& getFilters() const;

    /**
     * @return the DAVS Region which this request is targeting.
     */
    Region getRegion() const;

    /// @name { ArtifactRequest methods.
    /// @{
    Type getRequestType() const override;
    bool needsUnpacking() const override;
    std::string getSummary() const override;
    std::string toJsonString() const override;
    /// @}

private:
    DavsRequest(std::string type, std::string key, FilterMap filters, Region endpoint, bool unpack);

private:
    const std::string m_type;
    const std::string m_key;
    const FilterMap m_filters;
    const Region m_region;
    const bool m_unpack;
    std::string m_summary;
};

inline const std::string& DavsRequest::getType() const {
    return m_type;
}

inline const std::string& DavsRequest::getKey() const {
    return m_key;
}

inline const DavsRequest::FilterMap& DavsRequest::getFilters() const {
    return m_filters;
}

inline Region DavsRequest::getRegion() const {
    return m_region;
}

inline Type DavsRequest::getRequestType() const {
    return Type::DAVS;
}

inline bool DavsRequest::needsUnpacking() const {
    return m_unpack;
}

inline std::string DavsRequest::getSummary() const {
    return m_summary;
}

}  // namespace commonInterfaces
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKASSETSINTERFACES_DAVSREQUEST_H_
