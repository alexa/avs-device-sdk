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
#ifndef ACSDK_APLCAPABILITYCOMMONINTERFACES_APLEVENTPAYLOAD_H_
#define ACSDK_APLCAPABILITYCOMMONINTERFACES_APLEVENTPAYLOAD_H_

#include <string>

#include "PresentationToken.h"

namespace alexaClientSDK {
namespace aplCapabilityCommonInterfaces {
namespace aplEventPayload {

/**
 * Represents a request for the clients to publish a user event to AVS. See
 * https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/presentation-apl.html#userevent
 * for description of the properties
 */
struct UserEvent {
    /// The presentation token for the document requesting the event
    PresentationToken token;
    /// Array of argument data to pass to Alexa
    std::string arguments;
    /// meta-information about the event trigger
    std::string source;
    /// The values of the components for all component IDs.
    std::string components;

/// GCC 4.x and 5.x reached end of support before some of c++11 amendments were made. In particular, these compilers
/// do not have default constructors for list initialization from aggregates. This issue has been addressed in GCC 6.x.
/// http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_defects.html#1467
#if !defined(__clang__) && defined(__GNUC__) && (__GNUC__ < 6)
    /**
     * Explicit constructor with parameters.
     *
     * This constructor is required only for GCC before version 6.x.
     *
     * @param[in] token      The presentation token for the document requesting the event
     * @param[in] arguments  Array of argument data to be passed to Alexa
     * @param[in] source     Meta-information about the event trigger
     * @param[in] components The values of the components for all component IDs.
     *
     * @see http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_defects.html#1467
     */
    UserEvent(PresentationToken token, std::string arguments, std::string source, std::string components) :
            token{token},
            arguments{arguments},
            source{source},
            components{components} {
    }
    /**
     * Explicit copy constructor
     *
     * This constructor is required only for GCC before version 6.x.
     *
     * @param[in] params Source parameters.
     *
     * @see http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_defects.html#1467
     */
    UserEvent(const UserEvent& params) :
            token{params.token},
            arguments{params.arguments},
            source{params.source},
            components{params.components} {
    }
#endif
};

/**
 * Represents a request for the clients to publish DataSource fetch request to AVS. Refer to
 * https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/presentation-apl.html for more
 * information on the payload
 */
struct DataSourceFetch {
    /// The presentation token for the document requesting the event
    PresentationToken token;
    /// Type of datasource that requests a fetch
    std::string dataSourceType;
    /// implementation specific (SendIndexListData, UpdateIndexListData, SendTokenListData) fetch request from APL Core
    std::string fetchPayload;

/// GCC 4.x and 5.x reached end of support before some of c++11 amendments were made. In particular, these compilers
/// do not have default constructors for list initialization from aggregates. This issue has been addressed in GCC 6.x.
/// http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_defects.html#1467
#if !defined(__clang__) && defined(__GNUC__) && (__GNUC__ < 6)
    /**
     * Explicit constructor with parameters.
     *
     * This constructor is required only for GCC before version 6.x.
     *
     * @param[in] token           The presentation token for the document requesting the event
     * @param[in] dataSourceType  Type of datasource that requests a fetch
     * @param[in] fetchPayload    Implementation specific (SendIndexListData, UpdateIndexListData, SendTokenListData)
     * fetch request from APL Core
     *
     * @see http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_defects.html#1467
     */
    DataSourceFetch(PresentationToken token, std::string dataSourceType, std::string fetchPayload) :
            token{token},
            dataSourceType{dataSourceType},
            fetchPayload{fetchPayload} {
    }
    /**
     * Explicit copy constructor
     *
     * This constructor is required only for GCC before version 6.x.
     *
     * @param[in] params Source parameters.
     *
     * @see http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_defects.html#1467
     */
    DataSourceFetch(const DataSourceFetch& params) :
            token{params.token},
            dataSourceType{params.dataSourceType},
            fetchPayload{params.fetchPayload} {
    }
#endif
};

/**
 * Represents a request for the clients to publish RuntimeError event to AVS.
 * See https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/presentation-apl.html#runtimeerror
 * for the description the payload
 */
struct RuntimeError {
    /// The presentation token for the document requesting the event
    PresentationToken token;
    /// error payload when APL processing encounters an error
    std::string errors;

/// GCC 4.x and 5.x reached end of support before some of c++11 amendments were made. In particular, these compilers
/// do not have default constructors for list initialization from aggregates. This issue has been addressed in GCC 6.x.
/// http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_defects.html#1467
#if !defined(__clang__) && defined(__GNUC__) && (__GNUC__ < 6)
    /**
     * Explicit constructor with parameters.
     *
     * This constructor is required only for GCC before version 6.x.
     *
     * @param[in] token   The presentation token for the document requesting the event
     * @param[in] errors  Error payload when APL processing encounters an error
     *
     * @see http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_defects.html#1467
     */
    RuntimeError(PresentationToken token, std::string errors) : token{token}, errors{errors} {
    }
    /**
     * Explicit copy constructor
     *
     * This constructor is required only for GCC before version 6.x.
     *
     * @param[in] params Source parameters.
     *
     * @see http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_defects.html#1467
     */
    RuntimeError(const RuntimeError& params) : token{params.token}, errors{params.errors} {
    }
#endif
};

/**
 * Represents a request for the clients to publish RenderDocumentState event to AVS.
 * See https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/presentation-apl.html#rendereddocumentstate
 * for the description the payload
 */
struct VisualContext {
    /// The presentation token for the document requesting visual context
    PresentationToken token;
    // the version of the UI component on the device
    std::string version;
    /// serialized visual context obtained from APL Core
    std::string visualContext;
    /// serialized datasource context obtained from APL Core
    std::string datasourceContext;

/// GCC 4.x and 5.x reached end of support before some of c++11 amendments were made. In particular, these compilers
/// do not have default constructors for list initialization from aggregates. This issue has been addressed in GCC 6.x.
/// http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_defects.html#1467
#if !defined(__clang__) && defined(__GNUC__) && (__GNUC__ < 6)
    /**
     * Explicit constructor with parameters.
     *
     * This constructor is required only for GCC before version 6.x.
     *
     * @param[in] token             The presentation token for the document requesting the event
     * @param[in] version           The version of the UI component on the device
     * @param[in] visualContext     Serialized visual context obtained from APL Core
     * @param[in] datasourceContext Serialized datasource context obtained from APL Core
     *
     * @see http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_defects.html#1467
     */
    VisualContext(
        PresentationToken token,
        std::string version,
        std::string visualContext,
        std::string datasourceContext) :
            token{token},
            version{version},
            visualContext{visualContext},
            datasourceContext{datasourceContext} {
    }
    /**
     * Explicit copy constructor
     *
     * This constructor is required only for GCC before version 6.x.
     *
     * @param[in] params Source parameters.
     *
     * @see http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_defects.html#1467
     */
    VisualContext(const VisualContext& params) :
            token{params.token},
            version{params.version},
            visualContext{params.visualContext},
            datasourceContext{params.datasourceContext} {
    }
#endif
};

}  // namespace aplEventPayload
}  // namespace aplCapabilityCommonInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_APLCAPABILITYCOMMONINTERFACES_APLEVENTPAYLOAD_H_