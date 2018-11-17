# Array usage
The objects of vuh::Array family only provide abstraction over uniform storage buffers.
Each vuh::Array object is basically a vkBuffer together with its own memory chunk and some auxiliary data.
Array interface for data exchange with a host system depends on a kind of allocator used.
If requested allocation strategy fails the fall-back strategy will be used if such exists.
If a fall-back allocator kicks in the data exchange interface would not change .
In that case some (but not all) operations will still be optimized for the actual type memory in use.
When all fall-back options are exhausted and memory is not allocated exception is be thrown.
Exceptions thrown from ```vuh::Array``` are all members of ```vk::Error``` family.
To get the maximum performance it is better to match the exact type memory at hand.
For example on integrated GPUs using ```vuh::mem::Host``` would be optimal while device-local still works.
In this particular example the same memory would be allocated but result in Array with
restricted data exchange interface and potential use of stage buffers for some operations.
Allocation strategy does not make any difference for the purpose of passing Arrays to computation kernels.
Below there is a more detailed description of most useful Allocator options and corresponding Array usage.

## Allocations
### Device (```vuh::mem::Device```)
Array memory will be requested in device-local memory.
The fall-back allocation strategy is ```vuh::mem::Host```.
This type of arrays is to be used in kernels, but data transfer to/from host is expected.
This is the default allocation strategy, so you can skip typing ```vuh::mem::Device```.
Its construction and data transfer interface enables efficient data handling with a potential
to avoid extra (staging) copy, handle big transfers in smaller chunks and partial latency hiding.
#### Construction and data transfer from host
```cpp
const auto ha = std::vector<float>(1024, 3.14f);     // host array to initialize from
using Array = vuh::Array<float>;                     // = vuh::Array<float, vuh::mem::Device>;

auto array_0 = Array(device, 1024);                  // create array of 1024 float in device local memory
array_0.fromHost(begin(ha), end(ha));                // copy data from host range
auto array_1 = Array(device, ha);                    // create array of floats and copy data from host iterable
auto array_2 = Array(device, begin(ha), end(ha));    // same in stl range style
auto array_3 = Array(device, 1024, [&](size_t i){return ha[i];}); // create + index-based transform
```
#### Transfer data to host
```cpp
auto ha = std::vector<float>(1024, 3.14f);           // host iterable to copy data to
auto array = vuh::Array<float>(device, 1024);        // device array to copy data from

array.toHost(begin(ha));                             // copy the whole device array to iterable defined by its begin location
array.toHost(begin(ha), [](auto x){return x;});      // copy-transform the whole device array to an iterable
array.toHost(begin(ha), 512, [](auto x){return x;}); // copy-transforn part the device array to an iterable
ha = array.toHost<std::vector<float>>();             // copy the whole device array to host
```

### Device-Only (```vuh::mem::DeviceOnly```)
```cpp
// create device-only array of 1024 floats
auto array = vuh::Array<float, vuh::mem::DeviceOnly>(device, 1024);
```
This type of arrays is supposed to be only inside the kernels.
No data transfer to/from device expected.
It is normally allocated in device-local memory.
The fall-back allocation strategy is ```vuh::mem::Host```.
Apart from missing data transfer interface it only differs from a normal Device array by couple of usage flags.
So it may show a bit better performance but most probably wouldn't.
In any case it is not worse then that and indicates intended usage so is a useful creature.

### Host (```vuh::mem::Host```)
For these memory is allocated on a host in a 'pinned' area, so that it is visible to GPU.
This is the only kind of memory you can get with integrated GPUs
(althogh there it is flagged as device-local, so technically it would be the same as ```vuh::mem::Unified```).
With descrete GPUs it is normally used as a fall-back memory when all device-local memory is drained
and for stage buffers (with ```vuh``` you do not normally need to think about those).
Another use case would be when each value of array is used on GPU only a few times so a separate
explicit copy of the whole array does not make sense.
Being a fall-back choice for other allocators this one is a last resort.
If it fails exception is thrown and no resources get allocated.
Its construction and data transfer interface follows that for a standard containers.
With an important difference that while it provides random access with operator [],
the iterators fall into 'mutable forward' category.
#### Construction and data exchange interface
```cpp
auto ha = std::vector<float>(1024, 3.14f);      // host array to initialize from
using Array = vuh::Array<float, vuh::mem::Host>;

auto array = Array(device, 1024);               // construct array of given size, memory uninitialized
auto array = Array(device, 1024, 3.14f);        // construct array of given size, initialize memory to a value
auto array = Array(device, begin(ha), end(ha)); // construct array from host range

array[42] = 6.28f;                              // random access with []
std::copy(begin(ha), end(ha), begin(array));    // forward-iterable
```

### Unified (```vuh::mem::Unified```)
Allocation for these arrays takes place in a device local and host visible memory.
Although such labeled is all memory in integrated GPUs, that is not the target use case.
It is rather for the devices such as some Radeon cards that have some (relatively small)
amount of an actual on-chip memory that can me mapped to a host visible address.
Typical use case for such arrays is dynamic data frequently (but maybe sparsely) updated
from the host side.
There is no fall-back allocation strategy, if allocation in device-local & host-visible
memory fails exception is thrown.
Construction and data exchange interface mirrors that of ```vuh::mem::Host``` allocated arrays.

## Iterators
Iterators provide means to copy around parts of ```vuh::Array``` data and constitute the interface of the ```copy_async``` family of functions.
Iterators to device data are created with ```device_begin()```, ```device_end()``` helper functions.
Offsetting those can be done with just a + operator (so those are kind of random access iterators).
```cpp
copy_async(device_begin(array_src), device_begin(array_src) + chunk_size
           , device_begin(array_dst));
```
Iterators to host-accesible data (if such exists for the allocator used with particular array) are obtained with the usual ```begin()``` and ```end()```. These are true random access iterators and are  implemented just as pointers at the moment.

## Array views
ArrayView is the non-owning read-write range of continuous data of some ```Array``` object.
It serves mainly as a tool to pass partial arrays to computational kernels.
ArrayView can be used interchangeably with Array for that purpose.
Copy operations at the moment do not support views and rely fully on iterators for similar tasks.
The convenience way to create the ArrayView is the ```array_view``` factory function.
