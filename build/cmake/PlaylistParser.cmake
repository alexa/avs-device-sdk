#
# Setup the PlaylistParser build.
#
# To build the Totem-PlParser based PlaylistParser, run the following command,
#     cmake <path-to-source> -DTOTEM_PLPARSER=ON.
#
#

option(TOTEM_PLPARSER "Enable Totem-Pl-Parser based playlist parser." OFF)

set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH ON)
if(TOTEM_PLPARSER)
    find_package(PkgConfig)
    pkg_check_modules(TOTEM REQUIRED totem-plparser>=3.10)
    add_definitions(-DTOTEM_PLPARSER)
endif()