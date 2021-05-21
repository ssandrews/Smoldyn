# Use it on Github Action.
# Thanks https://github.com/rpavlik/cmake-modules/blob/main/FindWindowsSDK.cmake

if (WIN32)
    #find_package(WindowsSDK)
    message ("CMAKE_VS_PLATFORM_NAME: ${CMAKE_VS_PLATFORM_NAME}")
    message ("CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION: ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")

    #set(WinSDK C:/Program Files (x86)/Windows Kits/10)

    get_filename_component(WindowsSDK_ROOT "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots;KitsRoot10]" ABSOLUTE)
    message(STATUS "WindowsSDK_ROOT: ${WindowsSDK_ROOT}")

    list(APPEND WindowsSDK_INCLUDE_DIR ${WindowsSDK_ROOT}/Include/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/shared)
    list(APPEND WindowsSDK_INCLUDE_DIR ${WindowsSDK_ROOT}/Include/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/um)
    list(APPEND WindowsSDK_INCLUDE_DIR ${WindowsSDK_ROOT}/Include/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/winrt)
    list(APPEND WindowsSDK_INCLUDE_DIR ${WindowsSDK_ROOT}/Include/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/ucrt)

    list(APPEND WindowsSDK_LIB_DIR ${WindowsSDK_ROOT}/Lib/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/shared/${CMAKE_VS_PLATFORM_NAME})
    list(APPEND WindowsSDK_LIB_DIR ${WindowsSDK_ROOT}/Lib/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/um/${CMAKE_VS_PLATFORM_NAME})
    list(APPEND WindowsSDK_LIB_DIR ${WindowsSDK_ROOT}/Lib/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/winrt/${CMAKE_VS_PLATFORM_NAME})
    list(APPEND WindowsSDK_LIB_DIR ${WindowsSDK_ROOT}/Lib/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/ucrt/${CMAKE_VS_PLATFORM_NAME})

    message (STATUS "WindowsSDK include dir: ${WindowsSDK_INCLUDE_DIR}")
    message (STATUS "WindowsSDK library dir: ${WindowsSDK_LIB_DIR}")
    include_directories(${WindowsSDK_INCLUDE_DIR})
    link_directories(${WindowsSDK_LIB_DIR})

    add_definitions(-D_WIN32 -D_MBCS)
endif ()

