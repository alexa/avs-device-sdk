#
# Configure sample applications and libraries.
#
# By default, common libraries for sample applications are not installed. To enable installation of common libraries,
# add INSTALL_COMMON_SAMPLE_LIBS to cmake:
#     cmake <path-to-source> -DINSTALL_COMMON_SAMPLE_LIBS=ON
#

option(INSTALL_COMMON_SAMPLE_LIBS "Install common libraries for sample applications" OFF)
