# vuh examples
## Mandelbrot ([doc/examples/mandelbrot/](mandelbrot))
Render the Mandelbrot set and write it to an (ppm) image.
This is the literal translation of [Vulkan Minimal Compute](https://github.com/Erkaman/vulkan_minimal_compute) example.
With the original source spanning over ~800 lines...
```cpp
auto main()-> int {
   const auto width = 320;  // image dimensions
   const auto height = 240;

   auto instance = vuh::Instance();
   auto device = instance.devices().at(0); // just get the first compute-capable device

   auto mandel = vuh::Array<uint32_t, vuh::mem::Host>(device, 4*width*height); // allocate memory to render Mandelbrot set to

   using Specs = vuh::typelist<uint32_t, uint32_t>;  // width, height of a workgroup
   struct Params{uint32_t width; uint32_t height;};  // width, height of an image
   auto program = vuh::Program<Specs, Params>(device, "mandelbrot.spv");
   program.grid(vuh::div_up(width, 32), vuh::div_up(height, 32))
          .spec(32, 32)({width, height}, mandel);    // do the render

   write_ppm("mandelebrot.ppm", begin(mandel), width, height); // dump image data to a ppm file

   return 0;
}
```
Each pixel value here is only accessed once by the kernel so it makes sense to allocate output array directly in host-visible memory.
Set is rendered in 2D workgroups of size 32x32 and the corresponding 2D grid dimensioned to cover the complete image.
In the end the data is dumped to disk as a ppm file.
Note that ```write_ppm``` accepts just normal ```uint32_t*``` pointer.

## saxpy ([doc/examples/saxpy/](saxpy))
Calculate [generalized vector addition](https://en.wikipedia.org/wiki/Basic_Linear_Algebra_Subprograms#Level_1).
Classical example to demonstrate computational frameworks/libraries.
Using arrays in device-local memory (needless in this particular example) and performs calculation on a 1D grid with 1D workgroups.

## asynchronous saxpy ([doc/examples/compute_transfer_overlap](compute_transfer_overlap))
This example demostrates overlap of computation and data transfers using async calls with host
synchronization mechanism.
It splits saxpy computation in two packages, each equal to half of the original problems.
Overlaps the computation on the first part of the data with the transfer of the second one to device.
And the computation on the second part of the data with transfering the first half of the results
back to the host.
No kernel modification is required compared to a fully blocking saxpy example.

The problem is split in 3 phases.
In the first phase the first tiles (halves of the corresponding arrays) are transferred to the device.
The second phase starts async computation of the previously transferred tiles and initiates async
transfer of the second halves of the working datasets.
The phase ends with a synchronization point at which both computation and data transfers should complete.
The 3rd phase starts with async copy of the first tile of processed data back to host.
Then it makes a blocking call to do computation on the second half of the data.
When that is ready the async transfer of the second half of the result is initiated potentially
overlapping with the ongoing transfer of the first half.

Execution of sync and async version can be schematically depicted as follows:
- sync
```
░░░░░░▤▤▤▤▤▤▤▤▤▤│░░░░░░▤▤▤▤▤▤▤▤▤▤│██████│▧▧▧▧▧▧▧▧▒▒▒▒▒▒
```
- async
```
░░░▤▤▤▤▤▤      │███            │███
   ░░░   ▤▤▤▤▤▤│░░░▤▤▤▤▤▤      │▧▧▧▧▒▒▒
               │   ░░░   ▤▤▤▤▤▤│    ▧▧▧▧▒▒▒
```
with
```
░- host-to-stage copy, ▤- stage-to-device copy,
▧- device-to-stage copy, ▒- stage-to-host blocking copy,
█- kernel execution
```

This example may not (and most probably is not) the most optimal way to do the thing.
It is written in effort to demonstate the async features in most unobstructed way while still doing
not something completely unreasonable.

## using  custom logger ([doc/examples/spdlog](spdlog))
Demonstrates using custom logger for logging Vuh and (when available) Vulkan instance diagnostic messages.
Instead of default option to log to ```std::cerr``` the ```spdlog``` library is used.
This example depends on ```spdlog``` to be available (and discoverable by CMake).
