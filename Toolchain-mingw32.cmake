# the name of the target operating system
SET(CMAKE_SYSTEM_NAME Windows)

# which compilers to use for C and C++
SET(CMAKE_C_COMPILER /opt/local/bin/x86_64-w64-mingw32-gcc)
SET(CMAKE_CXX_COMPILER /opt/local/bin/x86_64-w64-mingw32-g++)
SET(CMAKE_RC_COMPILER /opt/local/bin/x86_64-w64-mingw32-windres)

# here is the target environment located
SET(CMAKE_FIND_ROOT_PATH /opt/local/bin/x86_64-w64-mingw32)

# directory for library files
set (MINGWDIR /opt/local/x86_64-w64-mingw32)


# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search 
# programs in the host environment
#set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
#set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
#set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)