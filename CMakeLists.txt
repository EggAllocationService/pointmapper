cmake_minimum_required(VERSION 4.2)
project(Pointmapper)

set(CMAKE_CXX_STANDARD 20)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(wgpu-native)

option(REALSENSE_SUPPORT "Support for realsense cameras" ON)
option(KINECT2_SUPPORT "Support for Xbox Kinect v2 cameras" ON)
option(OAK_SUPPORT "Support for OAK cameras via depthai" OFF)
option(VISUALIZATION "Build visualization utilities" ON)

add_library(pointmapper STATIC
        include/pointmapper/DepthDevice.h
        lib/background/BackgroundProcessor.cpp
        include/pointmapper/background/BackgroundProcessor.h
        include/pointmapper/common.h
        lib/common.cpp
        lib/pipeline/PointmapperPipeline.cpp
        include/pointmapper/pipeline/PointmapperPipeline.h
        lib/pipeline/ComputePipeline.cpp
        include/pointmapper/pipeline/ComputePipeline.h
        lib/pipeline/GPUTexture.cpp
        include/pointmapper/pipeline/GPUTexture.h
        lib/pipeline/EmbeddedKernels.cpp
        include/pointmapper/pipeline/EmbeddedKernels.h
        include/pointmapper/registration/registration.h
        include/pointmapper/pipeline/nodes/Node.h
        include/pointmapper/pipeline/types/CommonTypes.h
        lib/pipeline/nodes/DepthCameraNode.cpp
        include/pointmapper/pipeline/nodes/DepthCameraNode.h
        lib/pipeline/nodes/Node.cpp
        lib/pipeline/nodes/CreatePointCloudNode.cpp
        include/pointmapper/pipeline/nodes/CreatePointCloudNode.h
        include/pointmapper/pipeline/nodes/RemoveBackgroundNode.h
        lib/pipeline/nodes/RemoveBackgroundNode.cpp
        include/pointmapper/pipeline/nodes/RemoveBlobsNode.h
        lib/pipeline/nodes/RemoveBlobsNode.cpp
        include/pointmapper/pipeline/nodes/GpuToCpuCopyNode.h
        lib/pipeline/nodes/GpuToCpuCopyNode.cpp
        lib/pipeline/nodes/NetworkSendNode.cpp
        include/pointmapper/pipeline/nodes/NetworkSendNode.h
        lib/pipeline/net.cpp
        lib/pipeline/nodes/NetworkReceiveNode.cpp
        include/pointmapper/pipeline/nodes/NetworkReceiveNode.h
        lib/pipeline/nodes/CpuToGpuCopyNode.cpp
        include/pointmapper/pipeline/nodes/CpuToGpuCopyNode.h
        lib/pipeline/enet.cpp
        lib/pipeline/nodes/DepthmapSendNode.cpp
        include/pointmapper/pipeline/nodes/DepthmapSendNode.h
        lib/pipeline/nodes/DepthmapReceiveNode.cpp
        include/pointmapper/pipeline/nodes/DepthmapReceiveNode.h)

target_include_directories(pointmapper PUBLIC include)

if (REALSENSE_SUPPORT)
    find_package(realsense2 REQUIRED)

    target_link_libraries(pointmapper PRIVATE realsense2::realsense2)
    target_sources(pointmapper
            PRIVATE
            lib/realsense/RealsenseFrame.cpp
            include/pointmapper/realsense/RealsenseFrame.h
            lib/realsense/RealsenseDevice.cpp
            include/pointmapper/realsense/RealsenseDevice.h)
endif()

if (KINECT2_SUPPORT)
    find_package(freenect2 REQUIRED)

    target_include_directories(pointmapper PRIVATE ${freenect2_INCLUDE_DIRS})
    target_link_libraries(pointmapper PRIVATE ${freenect2_LIBRARIES})

    target_sources(pointmapper
            PRIVATE
            lib/kinect2/Kinect2Device.cpp
            include/pointmapper/kinect2/Kinect2Device.h
            lib/kinect2/Kinect2Frame.cpp
            include/pointmapper/kinect2/Kinect2Frame.h)
endif()

if (OAK_SUPPORT)
    find_package(depthai REQUIRED COMPONENTS core)
    target_link_libraries(pointmapper depthai::core)
endif()


if (VISUALIZATION)
    include(FetchContent)
    FetchContent_Declare(glengine-src
        GIT_REPOSITORY https://github.com/EggAllocationService/GLEngine.git
        GIT_TAG ca449157342f4cd219bd7286a2f726e67c2b40d7
    )
    FetchContent_MakeAvailable(glengine-src)

    create_resource_library(NAME PointResources SOURCES
        lib/rendering/kernels.wgsl
        lib/rendering/shaders.wgsl
    )

    target_sources(pointmapper PRIVATE
            lib/rendering/PointCloudComponent.cpp
            include/pointmapper/rendering/PointCloudComponent.h
            lib/rendering/pipelines.cpp
            include/pointmapper/rendering/pipelines.h
    )
    target_link_libraries(pointmapper PUBLIC glengine PointResources)
    target_include_directories(pointmapper PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_compile_definitions(pointmapper PUBLIC POINTMAPPER_USE_GLENGINE_MATH)
else()
    provide_wgpu_native("29.0.0.0")
    target_link_libraries(pointmapper PUBLIC webgpu)
endif()

add_executable(testpoints src/main.cpp)


set(OpenMP_C_FLAGS "-Xpreprocessor -fopenmp")
set(OpenMP_C_LIB_NAMES "omp")
set(OpenMP_omp_LIBRARY omp)
set(OpenMP_CXX_FLAGS "-Xpreprocessor -fopenmp")
set(OpenMP_CXX_LIB_NAMES "omp")
set(OpenMP_omp_LIBRARY omp)

find_package(PkgConfig REQUIRED)
target_include_directories(pointmapper PUBLIC "/opt/homebrew/opt/libomp/include" "/usr/local/opt/libomp/include")
target_link_directories(pointmapper PUBLIC "/opt/homebrew/opt/libomp/lib" "/usr/local/opt/libomp/lib")
find_package(ZFP REQUIRED)
find_package(Eigen3 REQUIRED)

target_link_libraries(pointmapper PUBLIC Eigen3::Eigen ${ZFP_LIBRARIES})
target_include_directories(pointmapper PUBLIC ${ZFP_LIBRARIES})
target_link_libraries(testpoints PUBLIC pointmapper)

if(VISUALIZATION)
    add_executable(testvis
            vis/TestActor.cpp
            vis/TestActor.h
            vis/main.cpp
    )
    target_link_libraries(testvis PUBLIC pointmapper)
endif()

