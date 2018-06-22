add_definitions("-DACSDK_LOG_MODULE=gadgetManager")

add_library(GadgetManager SHARED
	BinaryRepresentation.cpp
	FinalizeGadgetEvent.cpp
	FrameParser.cpp
	GadgetEventConstants.cpp
	GadgetEventHeader.cpp
	GadgetEventProcessor.cpp
	GadgetInfoEvent.cpp
	GadgetManager.cpp
	GadgetProtocol.cpp
	GadgetProtocolConstants.cpp)

target_include_directories(GadgetManager PUBLIC
        "${ContextManager_INCLUDE_DIRS}"
        "${GadgetManager_SOURCE_DIR}/include")

if (WIN32)
	target_link_libraries(GadgetManager AVSCommon ws2_32)
else()
	target_link_libraries(GadgetManager AVSCommon)
endif()

# install target
asdk_install()
