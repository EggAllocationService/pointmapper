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
        lib/rendering/pipelines.h
        lib/registration/registration.cpp
        lib/registration/registration.h)

if (REALSENSE_SUPPORT)
    find_package(realsense2 REQUIRED)

    target_link_libraries(pointmapper PRIVATE realsense2::realsense2)
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
    target_link_libraries(pointmapper PRIVATE ${freenect2_LIBRARIES})

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
        GIT_TAG d587376
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
    target_link_libraries(pointmapper PUBLIC glengine PointResources)
    target_include_directories(pointmapper PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
else()
    provide_wgpu_native("29.0.0.0")
    target_link_libraries(pointmapper webgpu)
endif()

add_executable(testpoints src/main.cpp
        src/TestActor.cpp
        src/TestActor.h)

set(OpenMP_C_FLAGS "-Xpreprocessor -fopenmp")
set(OpenMP_C_LIB_NAMES "omp")
set(OpenMP_omp_LIBRARY omp)
set(OpenMP_CXX_FLAGS "-Xpreprocessor -fopenmp")
set(OpenMP_CXX_LIB_NAMES "omp")
set(OpenMP_omp_LIBRARY omp)

target_include_directories(pointmapper PUBLIC "/opt/homebrew/opt/libomp/include" "/usr/local/opt/libomp/include")
target_link_directories(pointmapper PUBLIC "/opt/homebrew/opt/libomp/lib" "/usr/local/opt/libomp/lib")
find_package(PCL REQUIRED)
find_package(Eigen3 REQUIRED)

target_link_libraries(pointmapper PUBLIC Eigen3::Eigen ${PCL_LIBRARIES})
target_include_directories(pointmapper PUBLIC ${PCL_INCLUDE_DIRS})

target_link_libraries(testpoints PUBLIC pointmapper)

