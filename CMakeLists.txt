cmake_minimum_required(VERSION 4.2)
project(Pointmapper)

set(CMAKE_CXX_STANDARD 20)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(wgpu-native)

option(REALSENSE_SUPPORT "Support for realsense cameras" ON)
option(KINECT2_SUPPORT "Support for Xbox Kinect v2 cameras" ON)
option(OAK_SUPPORT "Support for OAK cameras via depthai" OFF)
option(VISUALIZATION "Build visualization utilities" ON)

add_library(pointmapper SHARED lib/library.cpp
        lib/DepthDevice.h
        lib/background/BackgroundProcessor.cpp
        lib/background/BackgroundProcessor.h
        lib/common.h
        lib/common.cpp
        src/main.cpp
        lib/PointmapperPipeline.cpp
        lib/PointmapperPipeline.h
        lib/rendering/CloudRenderer.cpp
        lib/rendering/CloudRenderer.h)

if (REALSENSE_SUPPORT)
    find_package(realsense2 REQUIRED)

    target_link_libraries(pointmapper realsense2::realsense2)
    target_sources(pointmapper
            PRIVATE
            lib/realsense/RealsenseFrame.cpp
            lib/realsense/RealsenseFrame.h
            lib/realsense/RealsenseDevice.cpp
            lib/realsense/RealsenseDevice.h)
endif()

if (KINECT2_SUPPORT)
    find_package(freenect2 REQUIRED)

    target_include_directories(pointmapper PRIVATE ${freenect2_INCLUDE_DIRS})
    target_link_libraries(pointmapper ${freenect2_LIBRARIES})

    target_sources(pointmapper
            PRIVATE
            lib/kinect2/Kinect2Device.cpp
            lib/kinect2/Kinect2Device.h
            lib/kinect2/Kinect2Frame.cpp
            lib/kinect2/Kinect2Frame.h)
endif()

if (OAK_SUPPORT)
    find_package(depthai REQUIRED COMPONENTS core)
    target_link_libraries(pointmapper depthai::core)
endif()

provide_wgpu_native("29.0.0.0")

if (VISUALIZATION)
    include(FetchContent)
    set(GLFW_LIBRARY_TYPE "STATIC")
    FetchContent_Declare(glfw3-source
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
    )
    FetchContent_MakeAvailable(glfw3-source)

    FetchContent_Declare(glfw3-webgpu-source
        GIT_REPOSITORY https://github.com/eliemichel/glfw3webgpu.git
        GIT_TAG main
    )
    FetchContent_MakeAvailable(glfw3-webgpu-source)
    target_link_libraries(pointmapper glfw glfw3webgpu)
endif()
target_link_libraries(pointmapper webgpu)

add_executable(testpoints src/main.cpp)

target_link_libraries(testpoints pointmapper)

