#include <vuh/array.hpp>
#include <vuh/vuh.h>
#include <fstream>
#include <vector>

using std::begin;

/// write array of rgba values to a ppm image.
auto write_ppm(const char* filename
              , const uint32_t* data, uint32_t width, uint32_t height
              )-> void
{
   auto fout = std::ofstream(filename, std::ios::binary);
   fout << "P6" << "\n" << width << " " << height << " 255" << "\n";
   for(auto i = 0u; i < width*height; ++i){
      fout.put(char(*data++));
      fout.put(char(*data++));
      fout.put(char(*data++));
      ++data; // skip alpha channel
   }
}

auto main()-> int {
   const auto width = 320;
   const auto height = 240;

	auto instance = vuh::Instance();
	auto device = instance.devices().at(0); // just get the first compute-capable device

	auto mandel = vuh::Array<uint32_t, vuh::mem::Host>(device, 4*width*height);

	using Specs = vuh::typelist<uint32_t, uint32_t>;   // width, height of a workgroup
	struct Params{uint32_t width; uint32_t height;};   // width, height of an image
	auto program = vuh::Program<Specs, Params>(device, "mandelbrot.spv");
	program.grid(vuh::div_up(width, 32), vuh::div_up(height, 32))
          .spec(32, 32)({width, height}, mandel);   // run the kernel

   write_ppm("mandelebrot.ppm", mandel.data(), width, height);

   return 0;
}
