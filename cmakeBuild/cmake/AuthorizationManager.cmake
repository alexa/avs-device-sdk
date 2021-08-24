#
# Set up to use AuthorizationManager. This will also use the LWAAuthorizationAdapter over the CBLAuthDelegate.
#
# AuthorizationManager is currently not in manufactory, and will replace any provided
# AuthDelegateInterface implementation.
#
# To build with AuthorizationManager, run the following command,
#     cmake <path-to-source> -DAUTH_MANAGER=ON

option(AUTH_MANAGER "Enable AuthorizationManager." OFF)

if (AUTH_MANAGER)
    message("Enabling AuthorizationManager")
    add_definitions(-DAUTH_MANAGER)
endif()
