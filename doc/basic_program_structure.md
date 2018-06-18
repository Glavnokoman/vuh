# Basic Workflow
This document is to give an idea of how the ```c++``` side of a program can be organized
and connects to associated structures in the compute shader.
For more info about shader compilation see the [project setup](project_setup.md) part of
tutorial.
For illustration purposes the [saxpy](examples/saxpy) is roughly followed and explained.
For a complete working example please see the original code in the example directory.

## Create Instance
```cpp
auto instance = vuh::Instance(); // create default Instance object
```
Working with ```vuh``` starts from creating an object of ```vuh::Instance``` class.
There may be several such objects at the same time in one application.
This object is used to list and choose ```Vulkan``` capable devices
and also defines set of layers and extensions to use.
In debug builds debug reporting extension and validation layers are enabled by default.
It also provides simple logging interface that is used to log messages from debug report
extension, ```vuh``` itself and can be be called explicitly by an application using it.
The default reporter sends messages to ```std::cerr```.
For an example of using custom logger with ```vuh::Instance``` see
[spdlog example](examples/spdlog).
```Vulkan``` application info object can be passed as a parameter to ```Instance```
constructor for self-documenting purposes.
```Instance``` object should remain in scope as long as any devices created from it
are in use.

## Choose GPU device(s)
The next thing to do after an object of Instance is created is to choose the device.
Devices are used to execute kernels and allocate memory resources
(again mostly for the purpose of feeding it to the kernel).
```Vulkan```-enabled devices present in the system can be listed using
```Instance::devices()``` call.
```cpp
auto device = instance.devices().at(0); // just use the first available device
```
Most systems have only a single device however some have none and some have several.
For example to prefer using the discrete GPU when available and integrated otherwise one
could write
```cpp
auto devices = instance.devices();
auto it_discrete = std::find_if(begin(devices), end(devices), [&](const auto& d){ return
                      d.properties().deviceType == vk::PhysicalDeviceType::eIntegratedGpu;
                   });
auto device = it_discrete != end(devices) ? *it_discrete : devices.at(0);
```
As seen from this example device objects are copyable. When device object is copied a new
logical ```Vulkan``` device interfacing the same physical one is created.
When the device goes out of scope all resources associated with it are released.

## Copy input data to a GPU device
GPU kernels normally operate on a data present in the GPU memory.
So that to set up the computation some initial data must be copied to a GPU.
The ```vuh``` interface to GPU memory is ```vuh::Array<T>```.
Here only the simple illustration of basic idea is provided using the array allocated
(by default) in device-local memory.
For the details of arrays creation and usage see the corresponding
[tutorial section](array_usage.md).
```cpp
const auto ha = std::vector<float>(N, 3.14f); // host array as a data source
auto array = vuh::Array<float>(device, N);    // create device array with N elements
array.fromHost(begin(ha), end(ha));           // copy data from host to GPU
```
For this case there is also a convenience constructor combining the two latter lines into
```cpp
auto array = vuh::Array<float>(device, ha);   // create device array and initialize from host iterable
```

Now the data is on the GPU and ready to be used.

An exception to this model are the integrated cards found on laptops and most phones and
embedded devices. In that case one still have to use ```vuh::Array``` to provide data to
a kernel but now it can be allocated in the host-visible memory
(vuh::Array<T, vuh::mem::Host>) with the data being directly accessible by both GPU and CPU.
The example above would still work but may incur some performance penalties.
For more details see [array usage](array_usage.md) section.

## Execute Kernels
The GPU-side part of the program (kernel) is presented in ```vuh``` by a
callable ```Program``` class.
Again here only the basic usage is demonstrated, for more details see the corresponding
tutorial [section](kernels.md) and documentation.

Kernels are executed on 3-dimensional grids each grid-cell constituting a 3-dimensional workgroup.
The size of a workgroup is set up in the shader code, but can be set up from the host by
means of specification constants.
Size of the grid is then chosen such that the whole array (in all dimensions) is covered
possibly overshooting the boundaries of data (which should be handled in the shader code).
To pass small data to the kernel the so-called push constants are used.
To summarize ```vuh::Program``` interface mirroring that of a shader is combined by the
set of specification constants, push constants and array parameters.
The former two being the template parameters of a ```vuh::Program``` class.
To execute the object of a ```Program``` one should provide the values for grid size(s),
specification constants (if available) and push constants together with array parameters in a call ()
operator.
In the code all this looks like the following
```cpp
   using Specs = vuh::typelist<uint32_t>;     // shader specialization constants interface
   struct Params{uint32_t size; float a;};    // shader push-constants interface
   auto program = vuh::Program<Specs, Params>(device, "saxpy.spv"); // load SPIR-V binary code
   program.grid(128/64).spec(64)({128, 0.1}, array_out, array_in); // run, wait for completion
```
For corresponding constructs in the shader code see the [Kernels Usage](kernels.md).
Physical device on which the kernel is executed should be the same used to allocate arrays
passed to the Program call. Usually this means the same ```vuh::device``` but that is not
necessary.
Call to the object of ```Program``` is fully blocking, meaning that it returns only
when the underlying ```SPIR-V``` code finishes its execution on a GPU.
Now the data is processed and the result is written to the output array
(```d_y``` in this case)

## Copy data from GPU to host
Just like the input data needs to be copied to a GPU device before computation the result
should be copied back to host to make it available to the rest of the program.
That is simple
```cpp
d_y.toHost(begin(array_out));   // copy data back to host
```
Again, when the Array is allocated in host-visible memory one can skip this step and
access its data directly.
See the [Mandelbrot example](examples/mandelbrot) for how this may look in the code.

## Error handling
At the moment errors are signaled by throwing exceptions (Other options may be provided
in the future).
Exception are derived from ```vk::Error``` class, and most of them are native ```vk::```
exceptions.
All ```vuh``` classes are built to behave and release associated resources
when stack is unwound.
So one possible way to structure a program would be
```cpp
try{
   // do the vuh stuff
} catch (vk::Error& err){
   // handle vuh exceptions
} catch (...){
   // non-vuh exception here, f.e. std::out_of_range from instance.devices().at(0)
}
```
Certainly more granular approach is possible and welcome.
When exception is thrown there may also be a corresponding error message reported
to the log associated with underlying Instance object.
