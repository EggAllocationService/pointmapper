cmake_minimum_required(VERSION 4.2)
project(Pointmapper)

set(CMAKE_CXX_STANDARD 20)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(wgpu-native)

option(REALSENSE_SUPPORT "Support for realsense cameras" ON)
option(KINECT2_SUPPORT "Support for Xbox Kinect v2 cameras" ON)
option(OAK_SUPPORT "Support for OAK cameras via depthai" OFF)

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
        lib/rendering/CloudRenderer.h
        lib/kinect2/Kinect2Device.cpp
        lib/kinect2/Kinect2Device.h
        lib/kinect2/Kinect2Frame.cpp
        lib/kinect2/Kinect2Frame.h)

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
endif()

if (OAK_SUPPORT)
    find_package(depthai REQUIRED COMPONENTS core)
    target_link_libraries(pointmapper depthai::core)
endif()

provide_wgpu_native("29.0.0.0")
target_include_directories(pointmapper PUBLIC ${wgpu-native_INCLUDE_DIRS})
target_link_libraries(pointmapper ${wgpu-native_LIBRARIES})

add_executable(testpoints src/main.cpp)

target_link_libraries(testpoints pointmapper)

