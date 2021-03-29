#
# Setup the Keyword Detector type and compiler options.
#
# To build with a Keyword Detector, run the following command with a keyword detector type of AMAZON_KEY_WORD_DETECTOR,
# AMAZONLITE_KEY_WORD_DETECTOR, KITTAI_KEY_WORD_DETECTOR, or SENSORY_KEY_WORD_DETECTOR:
#     cmake <path-to-source>
#       -DAMAZON_KEY_WORD_DETECTOR=ON
#           -DAMAZON_KEY_WORD_DETECTOR_LIB_PATH=<path-to-amazon-lib>
#           -DAMAZON_KEY_WORD_DETECTOR_INCLUDE_DIR=<path-to-amazon-include-dir>
#       -DAMAZONLITE_KEY_WORD_DETECTOR=ON
#           -DAMAZONLITE_KEY_WORD_DETECTOR_LIB_PATH=<path-to-amazon-lib>
#           -DAMAZONLITE_KEY_WORD_DETECTOR_INCLUDE_DIR=<path-to-amazon-include-dir>
#           -DAMAZONLITE_KEY_WORD_DETECTOR_DYNAMIC_MODEL_LOADING=<ON_OR_OFF>
#           -DAMAZONLITE_KEY_WORD_DETECTOR_MODEL_CPP_PATH=<path-to-model-cpp-file-if-dynamic-model-loading-disabled>
#       -DKITTAI_KEY_WORD_DETECTOR=ON
#           -DKITTAI_KEY_WORD_DETECTOR_LIB_PATH=<path-to-kittai-lib>
#           -DKITTAI_KEY_WORD_DETECTOR_INCLUDE_DIR=<path-to-kittai-include-dir>
#       -DSENSORY_KEY_WORD_DETECTOR=ON 
#           -DSENSORY_KEY_WORD_DETECTOR_LIB_PATH=<path-to-sensory-lib>
#           -DSENSORY_KEY_WORD_DETECTOR_INCLUDE_DIR=<path-to-sensory-include-dir>
#

option(AMAZON_KEY_WORD_DETECTOR "Enable Amazon keyword detector." OFF)
set(AMAZON_KEY_WORD_DETECTOR_LIB_PATH "" CACHE FILEPATH "Amazon keyword detector library path.")
set(AMAZON_KEY_WORD_DETECTOR_INCLUDE_DIR "" CACHE PATH "Amazon keyword detector include dir.")
mark_as_dependent(AMAZON_KEY_WORD_DETECTOR_LIB_PATH AMAZON_KEY_WORD_DETECTOR)
mark_as_dependent(AMAZON_KEY_WORD_DETECTOR_INCLUDE_DIR AMAZON_KEY_WORD_DETECTOR)

option(AMAZONLITE_KEY_WORD_DETECTOR "Enable AmazonLite keyword detector." OFF)
option(AMAZONLITE_KEY_WORD_DETECTOR_DYNAMIC_MODEL_LOADING "Enable AmazonLite keyword detector dynamic model loading." OFF)
set(AMAZONLITE_KEY_WORD_DETECTOR_LIB_PATH "" CACHE FILEPATH "AmazonLite keyword detector library path")
set(AMAZONLITE_KEY_WORD_DETECTOR_INCLUDE_DIR  "" CACHE PATH "AmazonLite keyword detector include dir.")
set(AMAZONLITE_KEY_WORD_DETECTOR_MODEL_CPP_PATH "" CACHE FILEPATH "AmazonLite keyword detector dynamic model loading cpp file path")
mark_as_dependent(AMAZONLITE_KEY_WORD_DETECTOR_DYNAMIC_MODEL_LOADING AMAZONLITE_KEY_WORD_DETECTOR)
mark_as_dependent(AMAZONLITE_KEY_WORD_DETECTOR_MODEL_CPP_PATH AMAZONLITE_KEY_WORD_DETECTOR_DYNAMIC_MODEL_LOADING)
mark_as_dependent(AMAZONLITE_KEY_WORD_DETECTOR_LIB_PATH AMAZONLITE_KEY_WORD_DETECTOR)
mark_as_dependent(AMAZONLITE_KEY_WORD_DETECTOR_INCLUDE_DIR AMAZONLITE_KEY_WORD_DETECTOR)

option(KITTAI_KEY_WORD_DETECTOR "Enable KittAi keyword detector." OFF)
option(KITTAI_KEY_WORD_DETECTOR_LEGACY_CPP_ABI "Use legacy ABI version of KittAi binary release." ON)
set(KITTAI_KEY_WORD_DETECTOR_INCLUDE_DIR "" CACHE PATH "KittAi keyword detector include dir")
set(KITTAI_KEY_WORD_DETECTOR_LIB_PATH "" CACHE FILEPATH "KittAi keyword detector library path")
mark_as_dependent(KITTAI_KEY_WORD_DETECTOR_LEGACY_CPP_ABI KITTAI_KEY_WORD_DETECTOR)
mark_as_dependent(KITTAI_KEY_WORD_DETECTOR_INCLUDE_DIR KITTAI_KEY_WORD_DETECTOR)
mark_as_dependent(KITTAI_KEY_WORD_DETECTOR_LIB_PATH KITTAI_KEY_WORD_DETECTOR)

option(SENSORY_KEY_WORD_DETECTOR "Enable Sensory keyword detector." OFF)
set(SENSORY_KEY_WORD_DETECTOR_LIB_PATH "" CACHE FILEPATH "Sensory keyword detector library path.")
set(SENSORY_KEY_WORD_DETECTOR_INCLUDE_DIR "" CACHE PATH "Sensory keyword detector include dir.")
mark_as_dependent(SENSORY_KEY_WORD_DETECTOR_LIB_PATH SENSORY_KEY_WORD_DETECTOR)
mark_as_dependent(SENSORY_KEY_WORD_DETECTOR_INCLUDE_DIR SENSORY_KEY_WORD_DETECTOR)

if(NOT AMAZON_KEY_WORD_DETECTOR AND NOT AMAZONLITE_KEY_WORD_DETECTOR AND NOT KITTAI_KEY_WORD_DETECTOR AND NOT SENSORY_KEY_WORD_DETECTOR)
    message("No keyword detector type specified, skipping build of keyword detector.")
    return()
endif()

if(AMAZON_KEY_WORD_DETECTOR)
    message("Creating ${PROJECT_NAME} with keyword detector type: Amazon")
    if(NOT AMAZON_KEY_WORD_DETECTOR_LIB_PATH)
        message(FATAL_ERROR "Must pass library path of Amazon KeywordDetector!")
    endif()
    if(NOT AMAZON_KEY_WORD_DETECTOR_INCLUDE_DIR)
        message(FATAL_ERROR "Must pass include dir path of Amazon KeywordDetector!")
    endif()
    add_definitions(-DKWD)
    add_definitions(-DKWD_AMAZON)
    set(KWD ON)
endif()

if(AMAZONLITE_KEY_WORD_DETECTOR)
    message("Creating ${PROJECT_NAME} with keyword detector type: AmazonLite")
    if(NOT AMAZONLITE_KEY_WORD_DETECTOR_LIB_PATH)
        message(FATAL_ERROR "Must pass library path of AmazonLite KeywordDetector!")
    endif()
    if(NOT AMAZONLITE_KEY_WORD_DETECTOR_INCLUDE_DIR)
        message(FATAL_ERROR "Must pass include dir path of AmazonLite KeywordDetector!")
    endif()
    if(NOT AMAZONLITE_KEY_WORD_DETECTOR_DYNAMIC_MODEL_LOADING)
        if(NOT AMAZONLITE_KEY_WORD_DETECTOR_MODEL_CPP_PATH)
            message(FATAL_ERROR "Must pass the path of the desired model .cpp file for the AmazonLite Keyword Detector if dynamic loading of model is disabled!")
        endif()
    else()
        add_definitions(-DKWD_AMAZONLITE_DYNAMIC_MODEL_LOADING)
    endif()
    add_definitions(-DKWD)
    add_definitions(-DKWD_AMAZONLITE)
    set(KWD ON)
endif()

if(KITTAI_KEY_WORD_DETECTOR)
    message("Creating ${PROJECT_NAME} with keyword detector type: KittAi")
    if(NOT KITTAI_KEY_WORD_DETECTOR_LIB_PATH)
        message(FATAL_ERROR "Must pass library path of Kitt.ai KeywordDetector!")
    endif()
    if(NOT KITTAI_KEY_WORD_DETECTOR_INCLUDE_DIR)
        message(FATAL_ERROR "Must pass include dir path of Kitt.ai KeywordDetector!")
    endif()
    add_definitions(-DKWD)
    add_definitions(-DKWD_KITTAI)
    if(KITTAI_KEY_WORD_DETECTOR_LEGACY_CPP_ABI)
        add_definitions(-DKWD_KITTAI_LEGACY)
    endif()
    set(KWD ON)
endif()

if(SENSORY_KEY_WORD_DETECTOR)
    message("Creating ${PROJECT_NAME} with keyword detector type: Sensory")
    if(NOT SENSORY_KEY_WORD_DETECTOR_LIB_PATH)
        message(FATAL_ERROR "Must pass library path of Sensory KeywordDetector!")
    endif()
    if(NOT SENSORY_KEY_WORD_DETECTOR_INCLUDE_DIR)
        message(FATAL_ERROR "Must pass include dir path of Sensory KeywordDetector!")
    endif()
    add_definitions(-DKWD)
    add_definitions(-DKWD_SENSORY)
    set(KWD ON)
endif()
