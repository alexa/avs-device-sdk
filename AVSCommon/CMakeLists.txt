project(AVSCommon)

add_definitions("-DACSDK_LOG_MODULE=avsCommon")

add_subdirectory("AVS")
add_subdirectory("SDKInterfaces")
add_subdirectory("Utils")

add_library(AVSCommon
    AVS/src/AVSContext.cpp
    AVS/src/AVSDirective.cpp
    AVS/src/AVSMessage.cpp
    AVS/src/AVSMessageHeader.cpp
    AVS/src/AbstractAVSConnectionManager.cpp
    AVS/src/BlockingPolicy.cpp
    AVS/src/AlexaClientSDKInit.cpp
    AVS/src/Attachment/Attachment.cpp
    AVS/src/Attachment/AttachmentManager.cpp
    AVS/src/Attachment/AttachmentUtils.cpp
    AVS/src/Attachment/InProcessAttachment.cpp
    AVS/src/Attachment/InProcessAttachmentReader.cpp
    AVS/src/Attachment/InProcessAttachmentWriter.cpp
    AVS/src/CapabilityAgent.cpp
    AVS/src/CapabilityConfiguration.cpp
    AVS/src/CapabilityTag.cpp
    AVS/src/ComponentConfiguration.cpp
    AVS/src/DialogUXStateAggregator.cpp
    AVS/src/DirectiveRoutingRule.cpp
    AVS/src/EventBuilder.cpp
    AVS/src/ExceptionEncounteredSender.cpp
    AVS/src/CapabilityResources.cpp
    AVS/src/CapabilitySemantics/ActionsToDirectiveMapping.cpp
    AVS/src/CapabilitySemantics/CapabilitySemantics.cpp
    AVS/src/CapabilitySemantics/StatesToRangeMapping.cpp
    AVS/src/CapabilitySemantics/StatesToValueMapping.cpp
    AVS/src/Initialization/InitializationParametersBuilder.cpp
    AVS/src/Initialization/SDKPrimitivesProvider.cpp
    AVS/src/HandlerAndPolicy.cpp
    AVS/src/MessageRequest.cpp
    AVS/src/NamespaceAndName.cpp
    AVS/src/EditableMessageRequest.cpp
    AVS/src/WaitableMessageRequest.cpp
    Utils/src/Bluetooth/SDPRecords.cpp
    Utils/src/BluetoothEventBus.cpp
    Utils/src/Configuration/ConfigurationNode.cpp
    Utils/src/DeviceInfo.cpp
    Utils/src/FileUtils.cpp
    Utils/src/FormattedAudioStreamAdapter.cpp
    Utils/src/JSON/JSONGenerator.cpp
    Utils/src/JSON/JSONUtils.cpp
    Utils/src/HTTP2/HTTP2GetMimeHeadersResult.cpp
    Utils/src/HTTP2/HTTP2MimeRequestEncoder.cpp
    Utils/src/HTTP2/HTTP2MimeResponseDecoder.cpp
    Utils/src/HTTP2/HTTP2SendDataResult.cpp
    Utils/src/ID3Tags/ID3v2Tags.cpp
    Utils/src/LibcurlUtils/CallbackData.cpp
    Utils/src/LibcurlUtils/CurlEasyHandleWrapper.cpp
    Utils/src/LibcurlUtils/CurlMultiHandleWrapper.cpp
    Utils/src/LibcurlUtils/HTTPContentFetcherFactory.cpp
    Utils/src/LibcurlUtils/HttpPost.cpp
    Utils/src/LibcurlUtils/HttpPut.cpp
    Utils/src/LibcurlUtils/HTTPResponse.cpp
    Utils/src/LibcurlUtils/LibCurlHttpContentFetcher.cpp
    Utils/src/LibcurlUtils/LibcurlHTTP2Connection.cpp
    Utils/src/LibcurlUtils/LibcurlHTTP2ConnectionFactory.cpp
    Utils/src/LibcurlUtils/LibcurlHTTP2Request.cpp
    Utils/src/LibcurlUtils/LibcurlUtils.cpp
    Utils/src/LibcurlUtils/DefaultSetCurlOptionsCallbackFactory.cpp
    Utils/src/Logger/ConsoleLogger.cpp
    Utils/src/Logger/Level.cpp
    Utils/src/Logger/LogEntry.cpp
    Utils/src/Logger/LogEntryBuffer.cpp
    Utils/src/Logger/LogEntryStream.cpp
    Utils/src/Logger/Logger.cpp
    Utils/src/Logger/LoggerSinkManager.cpp
    Utils/src/Logger/LoggerUtils.cpp
    Utils/src/Logger/LogStringFormatter.cpp
    Utils/src/Logger/ModuleLogger.cpp
    Utils/src/Logger/ThreadMoniker.cpp
    Utils/src/MacAddressString.cpp
    Utils/src/MediaPlayer/PooledMediaPlayerFactory.cpp
    Utils/src/MediaPlayer/PooledMediaResourceProvider.cpp
    Utils/src/MediaPlayer/PlaybackContext.cpp
    Utils/src/Metrics.cpp
    Utils/src/Metrics/DataPoint.cpp
    Utils/src/Metrics/DataPointCounterBuilder.cpp
    Utils/src/Metrics/DataPointDurationBuilder.cpp
    Utils/src/Metrics/DataPointStringBuilder.cpp
    Utils/src/Metrics/MetricEvent.cpp
    Utils/src/Metrics/MetricEventBuilder.cpp
    Utils/src/Metrics/UplData.cpp
    Utils/src/MultiTimer.cpp
    Utils/src/Network/InternetConnectionMonitor.cpp
    Utils/src/Power/AggregatedPowerResourceManager.cpp
    Utils/src/Power/PowerMonitor.cpp
    Utils/src/Power/PowerResource.cpp
    Utils/src/Power/WakeGuard.cpp
    Utils/src/RequiresShutdown.cpp
    Utils/src/RetryTimer.cpp
    Utils/src/SafeCTimeAccess.cpp
    Utils/src/Stopwatch.cpp
    Utils/src/Stream/StreamFunctions.cpp
    Utils/src/Stream/Streambuf.cpp
    Utils/src/StringUtils.cpp
    Utils/src/TaskThread.cpp
    Utils/src/ThreadPool.cpp
    Utils/src/Threading/ConditionVariableWrapper.cpp
    Utils/src/Threading/ExecutorFactory.cpp
    Utils/src/Threading/Executor.cpp
    Utils/src/Threading/SharedExecutor.cpp
    Utils/src/TimePoint.cpp
    Utils/src/TimeUtils.cpp
    Utils/src/Timer.cpp
    Utils/src/Timing/TimerDelegate.cpp
    Utils/src/Timing/TimerDelegateFactory.cpp
    Utils/src/UUIDGeneration.cpp
    Utils/src/WaitEvent.cpp
    Utils/src/WavUtils.cpp
    Utils/src/WorkerThread.cpp
    ${FileSystemUtils_SOURCE})

target_include_directories(AVSCommon PUBLIC
    "${AVSCommon_SOURCE_DIR}/AVS/include"
    "${AVSCommon_SOURCE_DIR}/SDKInterfaces/include"
    "${AVSCommon_SOURCE_DIR}/Utils/include"
    "${AVSCommon_BINARY_DIR}/Utils/include"
    "${AVSCommon_SOURCE_DIR}/../Captions/Interface/include"
    "${RAPIDJSON_INCLUDE_DIR}"
    "${MultipartParser_SOURCE_DIR}"
    ${CURL_INCLUDE_DIRS})

target_include_directories(AVSCommon PRIVATE "${AVSCommon_SOURCE_DIR}/Utils/privateInclude")

if (MSVC)
    target_include_directories(AVSCommon PUBLIC
            "${CMAKE_CURRENT_BINARY_DIR}/exports")

    add_definitions("-DIN_AVSCOMMON")
endif()

if (CUSTOM_AUDIO_INPUT_STREAM_TRAITS)
target_include_directories(AVSCommon PUBLIC
    ${CUSTOMSDSTRAITS_LIB_DIR}/include)

target_link_libraries(AVSCommon ${CUSTOMSDSTRAITS_LIB_NAME})
endif ()

target_link_libraries(AVSCommon
    ${CURL_LIBRARIES}
    acsdkApplicationAudioPipelineFactoryInterfaces
    acsdkEqualizerInterfaces
    acsdkInteractionModelInterfaces
    acsdkNotifierInterfaces
    )

# install target
LIST(APPEND PATHS "${PROJECT_SOURCE_DIR}/AVS/include")
LIST(APPEND PATHS "${PROJECT_SOURCE_DIR}/SDKInterfaces/include")
LIST(APPEND PATHS "${PROJECT_SOURCE_DIR}/Utils/include")
LIST(APPEND PATHS "${PROJECT_BINARY_DIR}/Utils/include")
asdk_install_multiple("${PATHS}")
