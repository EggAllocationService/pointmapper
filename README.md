# Pointmapper

Pointmapper is a utility library for processing and transmitting depth camera data in a flexable and performant way.

It includes support for Xbox Kinect 2 and Intel Realsense cameras. Also provided is a 3D visualization toolkit for rendering point clouds and other 3D objects.

## Requirements
- CMake 4.2+
- Modern C++ compiler (C++20 minimum)
- pkg-config
- (Runtime) Vulkan 1.2+ OR DirectX 11+ OR Metal 2+ 

For Kinect 2 support:
- Enable with `-DKINECT2_SUPPORT=ON`
- `libfreenect2`
- (Runtime) OpenCL

For Realsense support:
- Enable with `-DREALSENSE_SUPPORT=ON`
- `librealsense2`

For the visualization toolkit:
- Enable with `-DVISUALIZATION=ON`
- AVX2 (on x86 platforms)

## Integration

This project is built via CMake, you should add it as a subproject to your project. You can use either `FetchContent` or Git submodules for this.

```cmake
include(FetchContent)
    FetchContent_Declare(pointmapper
        GIT_REPOSITORY https://github.com/EggAllocationService/Pointmapper.git
        GIT_TAG main
    )
    FetchContent_MakeAvailable(pointmapper)
    target_link_libraries(MyProject PUBLIC pointmapper)
```

## Usage (Headless)

Pointmapper is based around a pipeline, where nodes represent sources, sinks, and transformations of data. Pointmapper provides a number of nodes:
- CpuToGpuCopyNode/GpuToCpuCopyNode: Moves point cloud data between CPU and GPU buffers, to allow direct access to the data
- DepthCameraNode: An input node which provides its data from a connected depth camera
- RemoveBackgroundNode: Attempts to remove the background from a depth map by tracking maximum per-pixel depth (only valid for stationary cameras)
- RemoveBlobsNode: Removes isolated blobs of data in a depth map, useful as a post-processing step after RemoveBackgroundNode
- CreatePointCloudNode: Takes a depth/color map and combines it with camera data to produce a set of XYZRGB structs representing 3D points
- NetworkSendNode/NetworkReceiveNode: Provides a server/client implementation for transmitting compressed point cloud data over a network

To begin, create a pipeline object:
```c++
auto pipeline = new pointmapper::pipeline::PointmapperPipeline();
```

You can then create _Nodes_ and _Roots_ using the pipeline. _Roots_ are nodes which take no input and produce output. When transforming the node graph into an execution sequence
Pointmapper will begin at the roots. Any node that cannot be reached from a root will not be evaluated.

```c++
auto cam = pipeline->CreateRoot<pointmapper::pipeline::DepthCameraNode>(new Kinect2Device());
auto mask = pipeline->CreateNode<pointmapper::pipeline::RemoveBackgroundNode>();
```

Nodes expose inputs and outputs, where an output can be connected to an input on another node. For example, we can connect the depth and camera parameters from the camera to the background removal node:
```c++
mask->inputDepthMap->Connect(cam->depth);
mask->camera_params->Connect(cam->params);
mask->frameData->Connect(cam->frameData);
```

The node graph _must_ be acyclic; any cycles will cause the build process to fail. Additionally, all node inputs must be connected to an output unless marked as optional. Once you are finished describing the pipeline, you can build it:
```c++
pipeline->Build();
```

`Build()` will find a linear traversal of the graph that satisfies dependencies, and caches it. You can then call `Process()` in a loop to evaluate the graph:
```c++
while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    pipeline->Process();
}
```

## Usage (Visualization)
If the `VISUALIZATION` option is enabled when building, Pointmapper includes a copy of [GLEngine](https://github.com/EggAllocationService/GLEngine), a C++ graphics engine. 

GLEngine provides rendering of Wavefront .obj models with custom materials and animation, as well as text and other primitives. To get started with GLEngine and Pointmapper you must create an Engine object _before_ the pipeline.
This lets Pointmapper re-use the `wgpu` context created by GLEngine
```c++
auto engine = new glengine::Engine("Visualizer", int2(1280, 720)); // second parameter is the initial window width/height
engine->SetAllowNonFocusedPawnInput(true); // Lets us pilot the free-flying camera without locking the mouse to the window
auto pipeline = new pointmapper::pipeline::PointmapperPipeline(renderer->GetDevice(), wgpuDeviceGetQueue(renderer->GetDevice()));
```
We'll also establish a simple pipeline that converts the color/depth captured by a Kinect 2 camera to a point cloud without any further processing:
```c++

auto cam = pipeline->CreateRoot<pointmapper::pipeline::DepthCameraNode>(new Kinect2Device());
auto cloud = pipeline->CreateNode<pointmapper::pipeline::CreatePointCloudNode>();
cloud->camera_params->Connect(cam->params);
cloud->color->Connect(cam->color);
cloud->frameData->Connect(cam->frameData);
cloud->depth_map->Connect(cam->depth);

pipeline->Build();
printf("Pipeline built!\n");
```

Pointmapper includes the shaders needed to render point clouds efficiently. You can load these shaders into the GLEngine instance by calling `addPointmapperPipelines`:
```c++
addPointmapperPipelines(engine->GetRenderer());
```

Now that the pipeline is built and the rendering environment is set up, we can create an Actor to draw our point cloud. 
In GLEngine, the world is made up of Actors, which themselves are made up of Components. Only Components can submit rendering commands; Actors only manage the components they own.

Pointmapper provides `PointCloudComponent`, which connects to a point cloud output from a pipeline node and renders it at the component's origin. We'll define a `PointActor` that contains one `PointCloudComponent`:

```c++
class PointActor : public glengine::world::Actor {
public:
    PointActor(std::shared_ptr<pointmapper::pipeline::Output<pointmapper::pipeline::GPUPointCloud>> cloud) {
        auto component = CreateComponent<PointCloudComponent>();
        component->SetCloudNode(cloud);
    }
    
    void Update(double deltaTime) override {
        // animation or other per-frame calculations happen here
    }
};
```

Finally, we can spawn our `PointActor` in the world and begin the main loop:
```c++
engine->SpawnActor<PointActor>(cloud->cloud);
while (true) {
    glfwPollEvents();
    pipeline->Process();
    engine->Update();
    engine->Render();
}
```

The engine will provide a default free-flying camera to let you observe the point cloud from any angle. You can view the complete source code for this example in `examples/simpleviz.cpp`




