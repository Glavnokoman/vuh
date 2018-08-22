/** doc me
*/

#include <vuh/array.hpp>
#include <vuh/vuh.h>
#include <vector>

auto main()-> int {
	constexpr auto arr_size = 1024;        // array size
	constexpr auto tile_size = arr_size/2; // processing chunk size
	constexpr auto grid_x = 64;            // workgroup x-dimension.

	auto y = std::vector<float>(arr_size, 1.0f); // host data
	auto x = std::vector<float>(arr_size, 2.0f);
	const auto a = 0.1f; // saxpy scaling constant

	auto instance = vuh::Instance();
	auto device = instance.devices().at(0);  // just get the first compute-capable device
	using Specs = vuh::typelist<uint32_t>;   // the only specialization constant is the workgroup dimension

	struct Params{uint32_t size; float a;};  // kernel parameters: array size, and saxpy scaling constant
	auto program = vuh::Program<Specs, Params>(device, "saxpy.spv");

	auto d_y = vuh::Array<float>(device, arr_size); // allocate memory on device (no initialization yet)
	auto d_x = vuh::Array<float>(device, arr_size);

	{ // phase 1. copy tiles to device
		// initiate copying to device and return synchronization token
		auto f_y = vuh::copy_async(begin(y), begin(y) + tile_size, device_begin(d_y));
		// if async operation is the last in the block (incl. host-side operations) no explicit token required.
		// In this case sync copy might suit better since that avoids temporary command buffer creation.
		vuh::copy_async(begin(x), begin(x) + tile_size, device_begin(d_x));
	} // here it blocks till both transfers are complete

	{ // phase 2. process first tiles, copy over the second chunk
		auto f_y = vuh::copy_async(begin(y) + tile_size, end(y), device_begin(d_y) + tile_size);
		auto f_x = vuh::copy_async(begin(x) + tile_size, end(x), device_begin(d_x) + tile_size);

		program.grid(tile_size/grid_x).spec(grid_x)
		       .run_async({tile_size, a}, vuh::array_view(d_y, 0, tile_size)
		                                , vuh::array_view(d_x, 0, tile_size));
	} // here it blocks again

	{ // phase 3. copy back first result tile, run kernel on second tiles
		auto f_y = vuh::copy_async(device_begin(d_y), device_begin(d_y) + tile_size, begin(y));

		// this is effectively blocking too.
		// We need result to be available to init the copy-back of the second chunk.
		// Meanwhile the first chunk is being copied in the background.
		program.run_async({tile_size, a}, vuh::array_view(d_y, tile_size, arr_size)
		                                , vuh::array_view(d_x, tile_size, arr_size));
		vuh::copy_async(device_begin(d_y) + tile_size, device_end(d_y), begin(y) + tile_size);
		f_y.wait(); // explicitly wait for the first chunk here.
	}

	return 0;
}
