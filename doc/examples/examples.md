# vuh examples
## Mandelbrot (```doc/examples/mandelbrot/```)
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
Each pixel value here is only accessed once by the kernel so it makes sense to allocate output array directly on host.
Set is rendered in 2D workgroups of (32, 32) and corresponding 2D grid to cover the complete image.
And then the data is dumped to disk as a ppm file.
Note that ```write_ppm``` accepts just normal ```uint32_t*``` pointer.

## saxpy (```doc/examples/saxpy/```)
Calculate 1D scaled vector addition.
Classical example to demonstrate computational frameworks/libraries.

## using  custom logger
