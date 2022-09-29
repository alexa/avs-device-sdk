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

#ifndef ACSDK_ALEXAREMOTEVIDEOPLAYERINTERFACES_REMOTEVIDEOPLAYERCONFIGURATION_H_
#define ACSDK_ALEXAREMOTEVIDEOPLAYERINTERFACES_REMOTEVIDEOPLAYERCONFIGURATION_H_

#include <list>
#include <set>
#include <string>

#include <AVSCommon/Utils/Optional.h>

#include <acsdk/VideoContent/VideoEntityTypes.h>

namespace alexaClientSDK {
namespace alexaRemoteVideoPlayerInterfaces {

/**
 * Configuration object used in the Discovery Response
 */
struct Configuration {
    /**
     * Enumeration of supported operations.
     */
    enum class SupportedOperations {
        /// Play video content.
        PLAY_VIDEO,
        /// Display results for a user query.
        DISPLAY_SEARCH_RESULTS
    };

    /**
     * A Catalog object instance.
     */
    struct Catalog {
        /**
         * Enumeration of different supported Catalog types.
         */
        enum class Type {
            /// Private catalog type.
            PRIVATE_CATALOG,
            /// Publicly accessible catalog type.
            PUBLIC_CATALOG
        };

        /**
         * Constructor.
         *
         * @param sourceId The identifier for the catalog, defaults to "imdb'.
         * @param type The type of catalog, defaults to PUBLIC_CATALOG.
         */
        Catalog(std::string sourceId = "imdb", Type type = Type::PUBLIC_CATALOG) :
                sourceId{std::move(sourceId)},
                type(type) {
        }

        /// Unique identifier for the catalog. For private catalogs, this is set to the partner id used for catalog
        /// ingestion.
        std::string sourceId;
        /// Type of Catalog
        Type type;
    };

    /// All directives supported by the skill. If it is not set, all directives will be assumed supported.
    avsCommon::utils::Optional<std::set<SupportedOperations>> operations;
    /// All entity types supported by the skill
    avsCommon::utils::Optional<std::set<acsdkAlexaVideoCommon::VideoEntity::EntityType>> entityTypes;
    /// Public and private catalogs supported by the skill.
    std::list<Catalog> catalogs;

    /**
     * Default constructor. Add a catalog entry with sourceId set to 'imdb' and type set to PUBLIC_CATALOG.
     */
    Configuration() : catalogs{Catalog()} {
    }

    /**
     * Constructor.
     *
     * @param operations The set of supported operations.
     * @param entityTypes The set of supported entity types.
     * @param catalogs The list of supported private and public catalogs.
     */
    Configuration(
        std::set<SupportedOperations> operations,
        std::set<acsdkAlexaVideoCommon::VideoEntity::EntityType> entityTypes,
        std::list<Catalog> catalogs) :
            operations{std::move(operations)},
            entityTypes{std::move(entityTypes)},
            catalogs{std::move(catalogs)} {
    }
};

}  // namespace alexaRemoteVideoPlayerInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_ALEXAREMOTEVIDEOPLAYERINTERFACES_REMOTEVIDEOPLAYERCONFIGURATION_H_