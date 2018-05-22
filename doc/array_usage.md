# vuh::Array usage notes
## Allocation and usage flags
- mem::Device. Array used on device, DEVICE_LOCAL memory, copy to/from host expected.
   + default
   + usage flags include transfer bits to and from
   + fallback allocation strategy mem::Host
   + provide view with mutable forward iterators (mem::Host(-double)-buffered)
   + provide fromHost() factory functions (incl. transform)
   + provide toHost() (incl. transform) function to limit potential performance hit on systems with integrated GPUs.
- mem::DeviceOnly. Array used on device only, no copy to/from host
   + some intermediate arrays used in a pipeline not visible by host
   + no usage flags
   + fallback allocation mem::Host
   + provides no views or iterables interface
- mem::DeviceMapped. Host-mappable device arrays, DEVICE_LOCAL | HOST_VISIBLE memory
   + small arrays used mostly by GPU, occasionally updated from host
   + fallback allocation mem::Host
   + provides view with mutable random access iterator
- mem::Host. Arrays in HOST_VISIBLE memory (preferrably uncached)
   + arrays used mostly by CPU, and only occasionally by GPU
   + each value accessed 1-2 times by GPU
   + no transfer bits in usage (unless brought down by fallback)
   + iterable with mutable random access iterators
   + no fallback. allocation failure is end of the story.
- Staging buffers. Not much use for these apart of DEVICE_LOCAL array internals.
   + mem::HostUncached. use to copy data to device, fallback mem::Host
   + mem::HostCached. use to copy data from device, fallback mem::Host

## Notes
vuh::mem::Device arrays are safe to use on systems without dedicated GPU memory,
however this may have an associated performance hit in case of host-side iterable interfaces.
To avoid it use hasDeviceMemory(), hasDeviceMemoryMapped() functions to branch early.


When fallback allocation strategy is used the underlying usage flags are transferred down the stack.

## Examples
