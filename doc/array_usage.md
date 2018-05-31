# Array usage
At the moment objects of vuh::Array family only provide abstraction over Vulkan storage buffers.
Each vuh::Array object is basically a vkBuffer together with owned memory chunk and some auxiliary data.
Array interface for data exchange with a host system depends on a kind of allocator used.
If requested allocation strategy fails the fall-back strategy will be used if such exists.
Naturally the data exchange interface would not change when a fall-back allocator kicks in.
When fall-back options are exhausted and memory is not allocated exception will be thrown.
Some (but not all) operations will be optimized still under the cover in that case for the
actual memory allocated.
Thus to get maximum performance it is better to match the memory at hand.
For example on integrated GPUs use of ```vuh::mem::Host``` would be optimal,
while device-local would still work.
In this particular example the same memory would be allocated but result in Array with
restricted data exchange interface and potential use of stage buffers for some operations.
Allocation strategy does not make any difference for the purpose of passing Array-s to kernels.
All data transfers operations are blocking (again, just for now :)).
Below there is a more detailed description of Allocator options and corresponding Array usage.

## Device-Only (vuh::mem::DeviceOnly)
```cpp
// create device-only array of 1024 floats
auto array = vuh::Array<float, vuh::mem::DeviceOnly>(device, 1024);
```
This type of arrays is supposed to be only used in kernels.
No data transfer to/from device expected.
It is normally allocated in device-local memory.
The fall-back strategy is uncached  host-visible memory.
Apart from missing data transfer interface it only differs from a normal Device array by couple of usage flags.
So it may show a bit better performance but most probably wouldn't.
In any case it is not worse then that and indicates intended usage so is a useful creature.

## Device (vuh::mem::Device)
Array memory will be allocated in device-local memory (preferably).
If that fails host-visible is the fall-back.
This type of arrays is mostly for use in kernels, but data transfer to/from host is expected.
This is the default allocation strategy, so you can skip typing ```vuh::mem::Device```.

### Construction and data transfer from host
```cpp
const auto ha = std::vector<float>(1024, 3.14f);                              // host array to initialize from
auto array_0 = vuh::Array<float, vuh::mem::Device>(device, 1024);             // create array of 1024 float in device local memory
auto array_1 = vuh::Array<float>(device, 1024);                               // same. vuh::mem::Device is the default allocator
array_1.fromHost(begin(ha), end(ha));                                         // copy data from host range
auto array_2 = vuh::Array<float>(device, ha);                                 // create array of floats and copy data from host iterable
auto array_3 = vuh::Array<float>(device, begin(ha), end(ha));                 // same in stl range style
auto array_4 = vuh::Array<float>(device, 1024, [&](size_t i){return ha[i];}); // create + index-based transform
```

### Transfer data to host
```cpp
auto array = vuh::Array<float>(device, 1024);        // device array to copy data from
auto ha = std::vector<float>(1024, 3.14f);           // host iterable to copy data to
array.toHost(begin(ha));                             // copy the whole device array to iterable defined by its begin location
array.toHost(begin(ha), [](auto x){return x;});      // copy-transform the whole device array to an iterable
array.toHost(begin(ha), 512, [](auto x){return x;}); // copy-transforn part the device array to an iterable
auto ha_2 = array.toHost<std::vector<float>>();      // copy the whole device array into a newly created host itreable
```


## Unified (vuh::mem::Unified)
Allocation for these arrays is requested in a device local and host visible memory.
Although such labeled is all memory in integrated GPUs, that is not the target use case.
It is rather for the devices such as some Radeon cards that have some (relatively small)
amount of an actual on-chip memory that can me mapped to host visible address.
Typical use case for such arrays is dynamic data frequently (but maybe sparsely) updated
from the host side.
There is no fall-back allocation strategy, if allocation in device-local & host-visible
memory fails exception is thrown.

### Construction and data exchange interface
```cpp
```

## Host-Visible (vuh::mem::Host)
### Construction and data exchange interface
