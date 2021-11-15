add_definitions("-DACSDK_LOG_MODULE=acsdkKWD")

# If KWD Component file is not defined by an adapter, set to null kwd.
if (NOT DEFINED KWD_COMPONENT_FILE)
    message("Creating NullKWDComponent")
    set(KWD_COMPONENT_FILE KWDComponent.cpp)
endif()

# Add KWDComponent file from Adapter Repository
add_library(acsdkKWD ${KWD_COMPONENT_FILE})

target_include_directories(acsdkKWD PUBLIC
    "${acsdkKWD_SOURCE_DIR}/include"
)

target_link_libraries(acsdkKWD
    acsdkKWDInterfaces
    acsdkKWDImplementations
    acsdkManufactory
    AVSCommon
    ${TARGET_KWD_LIB}
)

# install target
asdk_install()
