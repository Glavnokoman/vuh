# Asynchronous operations
Thus section describes asynchronous data transfer and kernel execution modes with the host-side synchronization.
It can be used to perform some useful work on the host while waiting for GPU to complete its job.
Another application is overlapping data transfers and computation on GPU (see [doc/examples/compute_transfer_overlap/](examples/compute_transfer_overlap) example ).
For the latter the interface presented here is not optimal since it involves synchronization with the host.

## Synchronization tokens
Asynchronous data transfer operation (```vuh::copy_async()```) and kernel execution (```Program::run_async()```) return the objects of type ```vuh::Delayed<>``` that can be used to synchronize with the host.
```cpp
auto token_copy = vuh::copy_async(begin(y), end(y), device_begin(d_y));
auto token_comp = program.run_async(d_y);
```
These tokens are basically the Vulkan fence bundled together with the action that needs to be performed at the synchronization point.
The action can be just a release of some resources that have to be kept alive for the duration of async operation.
Or anything else representable with a movable class implementing the ```operator()() const-> void```.

Synchronization with the host can be achieved by explicit call to ```Delayed<>::wait()``` function (possibly with a max wait duration specified in ns).
This would block the execution of the current thread till the underlying Vulkan fence object is signaled, or in case of the timed wait - till the time runs out.
When the token goes out of scope the corresponding destructor waits for the fence to be in a signaled state.
Thus scoping can be used to set up synchronization points.
Another consequence of this is that ignoring return value from asynchronous operations makes them effectively blocking.
The subtle difference is that asynchronous calls may be more expensive resource-wise (in particular all of them create a temporary Vulkan command buffer for the duration of their run).
On the other hand blocking copy() calls may split its work in chunks and run them asynchronously - something ```copy_async()``` would never do.
Timed out ```wait()``` can be safely called multiple times, or ```wait()``` may not be called at all -
the underlying action will be executed once and only once.
Move assignment is also a synchronization point for the ```Delayed<>``` object being assigned to.

## Async data transfer
Asynchronous copy can be initiated between the two ```vuh``` arrays, or between the host iterable and device-local ```vuh``` array (both ways).
```cpp
auto tkn_1 = vuh::copy_async(begin(host), end(host), device_begin(d_y)); // transfer data to device
auto tkn_2 = vuh::copy_async(device_begin(d_x), device_end(d_x), device_begin(d_y)); // data transfer between device buffers
auto tkn_3 = vuh::copy_async(device_begin(d_y), device_end(d_y), begin(host)); // data transfer from device to host
```
The synchronization token returned by copy operations is of the type  ```Delayed<Copy>```.
In particular the token carries the temporary Vulkan command buffer created for every async copy operation.
The details of synchronization behavior depends on arrays involved in an operation, especially on whether the copy involves a hidden staging buffer.

- Copying between the two ```vuh::Array``` objects initiates the copy and return immediately. At the sync point it blocks till the underlying fence is signaled (copy is complete) and then returns.
- Copying between the host and host-visible array (either direction) fully blocks for the duration of copy. At sync point just returns immediately.
- Copying from host to device-local array blocks initially for the duration of hidden copy to staging buffer, then returns. At sync point waits till the fence is signaled (copy to device is complete) and returns.
- Copying from device-local array to host returns immediately. At sync point blocks till the fence is signaled (copy to staging buffer is complete) and then starts the blocking copy from staging buffer to the host target.

So that when there are several device-to-host async copies in the scope
care must be taken to sync them in the same order they were initiated
```cpp
{
   auto t_1 = vuh::copy_async(device_begin(d_y), device_begin(d_y) + tile_size, begin(y));
   auto t_2 = vuh::copy_async(device_begin(d_y) + tile_size, device_end(d_y), begin(y) + tile_size);
   t_1.wait(); // explicitly wait for the first chunk
}
{
   auto t_1 = vuh::copy_async(device_begin(d_y), device_begin(d_y) + tile_size, begin(y));
   auto t_2 = vuh::copy_async(device_begin(d_y) + tile_size, device_end(d_y), begin(y) + tile_size);
}
```
In the example above operations (assuming two device-to-host copies in same direction may not overlap) may be depicted as
```
  block 1      block 2
▧▧▧▧▒▒▒    │▧▧▧▧       ▒▒▒│
    ▧▧▧▧▒▒▒│    ▧▧▧▧▒▒▒   │
```
with
```
▧- device-to-stage copy
▒- stage-to-host copy
```
In block 2 where the tokens are deleted in reverse creation order as they go out scope the staging copy of the first buffer is only initiated after the second one is complete which is suboptimal.

## Async kernel execution
Asynchronous kernel execution can be initialized by a call to ```Program::run_async()```.
It is interchangeable with the blocking calls to ```Program::operator()(...)``` and ```Program::run()``` and just like those expect that specialization constants and grid dimensions are set for the object they are called from.
The synchronization token returned by ```run_async()``` is of the type ```Delayed<Compute>```.
No delayed actions are scheduled with the token.
It only carries the temporary Vulkan command buffer associated with the call.
```cpp
auto t_p = program.grid(tile_size/grid_x).spec(grid_x)
                  .run_async({tile_size, a}, vuh::array_view(d_y, 0, tile_size)
                                           , vuh::array_view(d_x, 0, tile_size));
```
Async kernels are often executed on parts a problem.
In those cases ```array_view``` come in handy to replace array references in ```Program::bind()``` and ```Program::run_async()```.

## Example
[doc/examples/compute_transfer_overlap](examples/compute_transfer_overlap)
