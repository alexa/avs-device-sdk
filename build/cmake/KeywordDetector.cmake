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
option(AMAZONLITE_KEY_WORD_DETECTOR "Enable AmazonLite keyword detector." OFF)
option(AMAZONLITE_KEY_WORD_DETECTOR_DYNAMIC_MODEL_LOADING "Enable AmazonLite keyword detector dynamic model loading." OFF)
option(KITTAI_KEY_WORD_DETECTOR "Enable KittAi keyword detector." OFF)
option(SENSORY_KEY_WORD_DETECTOR "Enable Sensory keyword detector." OFF)

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
endif()
