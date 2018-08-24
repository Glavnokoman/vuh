# Asynchronous operations
Thus section describes asynchronous data transfer and kernel execution modes with the host-side synchronization.
It can be used to perform some useful work on the host while waiting for GPU to complete its job.
Another application is overlapping data transfers and computation on GPU.
For the latter interface presented here not optimal since it involves synchronization with the host.

# Synchronization tokens
- returned by async operations
- used to sync with host
- are some actions delayed till the underlying fence is signalled
- sync points are Delayed<>::wait() and Delayed<>::~Delayed<>()
- time-out wait(t) can be called multiple time
- action will take place once and only once
- tokens are movable
- assignment is a sync point for the token being assigned to
- ignoring returned value (a token) from async function effectively makes a blocking call
   + but may still be slightly different from corresponding blocking operation
# Async data transfer
- Delayed<Copy> carries the temporary command buffer used for async operation and whatever data required to complete the operation
- blocks for the duration of host-staging-buffer-exchange (if any)
   + host-to-device blocks for the staging copy at the call size
   + device-to-host blocks for the staging copy at the sync point and after the fence is triggered
# Async kernel execution
- Delayed<Compute> carries temporary command buffer used for async operatrion
- May overlap with the other kernel calls and data transfers depending on device capabilities
# Overlapping transfer with execution
- dissect the async saxpy example
