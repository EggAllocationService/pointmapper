function(provide_wgpu_native WGPU_VERSION)
    # Determine platform
    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set(WGPU_PLATFORM "macos")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(WGPU_PLATFORM "linux")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(WGPU_PLATFORM "windows")
    else()
        message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
    endif()

    # Determine architecture
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        set(WGPU_ARCH "aarch64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64|x86_64")
        set(WGPU_ARCH "x86_64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i[3-6]86|x86")
        set(WGPU_ARCH "i386")
    else()
        message(FATAL_ERROR "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()

    # Debug/Release suffix
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(WGPU_BUILD_TYPE "debug")
    else()
        set(WGPU_BUILD_TYPE "release")
    endif()

    # Windows ABI suffix
    set(WGPU_ABI "")
    if(WGPU_PLATFORM STREQUAL "windows")
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            set(WGPU_ABI "-gnu")
        else()
            set(WGPU_ABI "-msvc")
        endif()
    endif()

    set(WGPU_ARCHIVE_NAME "wgpu-${WGPU_PLATFORM}-${WGPU_ARCH}${WGPU_ABI}-${WGPU_BUILD_TYPE}.zip")
    set(WGPU_URL "https://github.com/gfx-rs/wgpu-native/releases/download/v${WGPU_VERSION}/${WGPU_ARCHIVE_NAME}")

    include(FetchContent)
    FetchContent_Declare(
        wgpu-native
        URL ${WGPU_URL}
    )
    FetchContent_MakeAvailable(wgpu-native)

    # Provide include and library to parent scope
    set(wgpu-native_INCLUDE_DIRS "${wgpu-native_SOURCE_DIR}/include" PARENT_SCOPE)

    # Find the static library (might be .a or .lib)
    find_library(WGPU_NATIVE_LIB
        NAMES libwgpu_native.a wgpu_native.lib
        PATHS "${wgpu-native_SOURCE_DIR}/lib"
        NO_DEFAULT_PATH
        REQUIRED
    )

    if (WIN32)
        set(OS_LIBRARIES d3dcompiler ws2_32 userenv bcrypt ntdll opengl32 Propsys RuntimeObject)
    elseif(UNIX AND NOT APPLE)
        set(OS_LIBRARIES "-lm -ldl")
    elseif(APPLE)
        set(OS_LIBRARIES "-framework Foundation -framework CoreFoundation -framework QuartzCore -framework Metal")
    endif()

    set(wgpu-native_LIBRARIES "${WGPU_NATIVE_LIB}" ${OS_LIBRARIES} PARENT_SCOPE)

    add_library(webgpu STATIC IMPORTED)
    set_property(TARGET webgpu PROPERTY IMPORTED_LOCATION ${WGPU_NATIVE_LIB})
    target_include_directories(webgpu INTERFACE "${wgpu-native_SOURCE_DIR}/include")
    target_link_libraries(webgpu INTERFACE ${OS_LIBRARIES})
endfunction()
