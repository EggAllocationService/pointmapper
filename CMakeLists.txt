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
        lib/PointmapperPipeline.cpp
        lib/PointmapperPipeline.h
        lib/rendering/PointCloudComponent.cpp
        lib/rendering/PointCloudComponent.h
        lib/rendering/pipelines.cpp
        lib/rendering/pipelines.h)

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


if (VISUALIZATION)
    include(FetchContent)
    FetchContent_Declare(glengine-src
        GIT_REPOSITORY https://github.com/EggAllocationService/GLEngine.git
        GIT_TAG d5e9bd4bf00ab243df26c057598351abe72fec41
    )
    FetchContent_MakeAvailable(glengine-src)


    create_resource_library(NAME PointResources SOURCES
        lib/rendering/kernels.wgsl
        lib/rendering/shaders.wgsl
    )

    target_sources(pointmapper PRIVATE
            lib/rendering/PointCloudComponent.cpp
            lib/rendering/PointCloudComponent.h
            lib/rendering/pipelines.cpp
            lib/rendering/pipelines.h
    )
    target_link_libraries(pointmapper glengine PointResources)
    target_include_directories(pointmapper PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
else()
    provide_wgpu_native("29.0.0.0")
    target_link_libraries(pointmapper webgpu)
endif()

add_executable(testpoints src/main.cpp)

find_package(PCL REQUIRED)

target_link_libraries(testpoints pointmapper ${PCL_LIBRARIES})

