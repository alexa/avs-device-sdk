#
# Custom Rapidjson usage.
#
# To build without the rapidjson provided by the SDK
#     cmake <path-to-source>
#           -DUSE_DEFAULT_RAPIDJSON=OFF
#
# If using the rapidjson provided by SDK the below options are available.
#
# To build without the memory optimization using the CrtAllocator run with the option,
#     cmake <path-to-source>
#           -DRAPIDJSON_MEM_OPTIMIZATION=OFF
#
# To build with custom memory settings run with the option,
#     cmake <path-to-source>
#           -DRAPIDJSON_MEM_OPTIMIZATION=CUSTOM
#           -DRAPIDJSON_DEFAULT_ALLOCATOR=<Default Allocator>
#           -DRAPIDJSON_VALUE_DEFAULT_OBJECT_CAPACITY=<Default Object Capacity>
#           -DRAPIDJSON_VALUE_DEFAULT_ARRAY_CAPACITY=<Defatul Array Capacity>
#           -DRAPIDJSON_DEFAULT_STACK_ALLOCATOR=<Default Stack Allocator>

set(USE_DEFAULT_RAPIDJSON ON CACHE BOOL "Use rapidjson packaged within the SDK")

if(USE_DEFAULT_RAPIDJSON)
    if(RAPIDJSON_MEM_OPTIMIZATION STREQUAL "OFF")
        # Do Nothing and let defaults take over
        message(STATUS "rapidjson upstream defaults used")
    elseif(RAPIDJSON_MEM_OPTIMIZATION STREQUAL "CUSTOM")
        # Use Custom values if set to custom
        message(STATUS "rapidjson custom values used")
        if(RAPIDJSON_DEFAULT_ALLOCATOR)
            add_definitions(-DRAPIDJSON_DEFAULT_ALLOCATOR=${RAPIDJSON_DEFAULT_ALLOCATOR})
        endif()

        if(RAPIDJSON_VALUE_DEFAULT_OBJECT_CAPACITY)
            add_definitions(-DRAPIDJSON_VALUE_DEFAULT_OBJECT_CAPACITY=${RAPIDJSON_VALUE_DEFAULT_OBJECT_CAPACITY})
        endif()

        if(RAPIDJSON_VALUE_DEFAULT_ARRAY_CAPACITY)
            add_definitions(-DRAPIDJSON_VALUE_DEFAULT_ARRAY_CAPACITY=${RAPIDJSON_VALUE_DEFAULT_ARRAY_CAPACITY})
        endif()

        if(RAPIDJSON_DEFAULT_STACK_ALLOCATOR)
            add_definitions(-DRAPIDJSON_DEFAULT_STACK_ALLOCATOR=${RAPIDJSON_DEFAULT_STACK_ALLOCATOR})
        endif()
    else()
        # Use Memory Optimization
        message(STATUS "rapidjson memory optimization used")
        add_definitions(-DRAPIDJSON_DEFAULT_ALLOCATOR=CrtAllocator)
        add_definitions(-DRAPIDJSON_VALUE_DEFAULT_OBJECT_CAPACITY=1)
        add_definitions(-DRAPIDJSON_VALUE_DEFAULT_ARRAY_CAPACITY=1)
        add_definitions(-DRAPIDJSON_DEFAULT_STACK_ALLOCATOR=CrtAllocator)
    endif()
endif()
