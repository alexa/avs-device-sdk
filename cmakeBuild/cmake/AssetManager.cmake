#
# Set up and enable Asset Manager and DAVS Client functionalities.
#
# Asset Manager is disabed by default, to build with it, run the following command,
#     cmake <path-to-source>
#       -DASSET_MANAGER=ON
#
# Asset Manager will only be enabled if both FileSystemUtils and LibArchive are found and enabled.
#

option(ASSET_MANAGER "Enable Asset Manager & DAVS Client functionality." OFF)

if(ASSET_MANAGER)
    if (NOT FILE_SYSTEM_UTILS)
        message("FileSystemUtils is not enabled, cannot enable Asset Manager functionalities")
        set(ASSET_MANAGER OFF)
    elseif (NOT LibArchive_FOUND)
        message("LibArchive is not found, cannot enable Asset Manager functionalities")
        set(ASSET_MANAGER OFF)
    elseif (NOT CRYPTO_FOUND)
        message("Crypto is not found, cannot enable Asset Manager functionalities")
        set(ASSET_MANAGER OFF)
    else ()
        message("Enabling Asset Manager functionalities")
        add_definitions("-DASSET_MANAGER")
    endif()
endif()
