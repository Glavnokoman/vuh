/** Example demonstrating the overlap of computation and data transfer operations.
# Async operations with host synchronization.
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
This example may not (and most probably is not) the most optimal way to do the thing.
It is written in effort to demonstate the async features in most unobstructed way while still doing
not something completely unreasonable.

Keypoints:
- synchronization point can be either a call to Delayed<>::wait() or just the end of the scope
- calling async function and ignoring its return is essentially a blocking call (but may not be doing exactly the same!)
- async data transfer between host and device potentially blocks for the duration of a hidden copy to/from the staging buffer
 	+ host-to-device blocks at call site
	+ device-to-host only initiates staging buffer to host transfer only at synchronization point, and then blocks for the duration of it.
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
	const auto a = 0.1f;                         // saxpy scaling constant

	auto instance = vuh::Instance();
	auto device = instance.devices().at(0);  // just get the first compute-capable device
	using Specs = vuh::typelist<uint32_t>;   // the only specialization constant is the workgroup dimension

	struct Params{uint32_t size; float a;};  // kernel parameters: array size, and saxpy scaling constant
	auto program = vuh::Program<Specs, Params>(device, "saxpy.spv");

	auto d_y = vuh::Array<float>(device, arr_size); // allocate memory on device
	auto d_x = vuh::Array<float>(device, arr_size); // but do not transfer any data yet

	{ // phase 1. copy tiles to device
		// initiate copying to device and return synchronization token
		auto t_y = vuh::copy_async(begin(y), begin(y) + tile_size, device_begin(d_y));
		// this is essentially a blocking call
		vuh::copy_async(begin(x), begin(x) + tile_size, device_begin(d_x));
	} // here it blocks till both transfers are complete

	{ // phase 2. process first tiles, copy over the second chunk
		auto t_p = program.grid(tile_size/grid_x).spec(grid_x)
		                   .run_async({tile_size, a}, vuh::array_view(d_y, 0, tile_size)
		                                            , vuh::array_view(d_x, 0, tile_size));

		auto t_y = vuh::copy_async(begin(y) + tile_size, end(y), device_begin(d_y) + tile_size);
		auto t_x = vuh::copy_async(begin(x) + tile_size, end(x), device_begin(d_x) + tile_size);
	} // here it blocks again

	{ // phase 3. copy back first half of the result, run kernel on second tiles
		auto t_1 = vuh::copy_async(device_begin(d_y), device_begin(d_y) + tile_size, begin(y));

		// We need result to be available to init the copy-back of the second chunk. Hence blocking call.
		program({tile_size, a}, vuh::array_view(d_y, tile_size, arr_size)
		                      , vuh::array_view(d_x, tile_size, arr_size));
		auto t_2 = vuh::copy_async(device_begin(d_y) + tile_size, device_end(d_y), begin(y) + tile_size);
		t_1.wait(); // explicitly wait for the first chunk here (think of staging buffers and destruction order)
	}

	return 0;
}
