#include <vuh/vuh.h>
#include <vuh/array.hpp>

using std::begin;
using std::end;

auto test_device_arrays_float()-> void {
   constexpr auto arr_size = size_t(128);
   auto h1 = std::vector<float>(arr_size);

	auto instance = vuh::Instance();
	auto device = instance.devices().at(0);

	auto d0 = vuh::Array<float, vuh::mem::DeviceOnly>(device, arr_size);

   auto d1 = vuh::Array<float, vuh::mem::Device>(device, arr_size);
   auto d4 = vuh::Array<float, vuh::mem::Device>(device, h1);
   auto d6 = vuh::Array<float, vuh::mem::Device>(device, begin(h1), end(h1));
   auto d3 = vuh::Array<float, vuh::mem::Device>(device, arr_size,
                                                 [](size_t i){return float(i);});

   // auto d7 = vuh::Array<float, vuh::arr::Pool<vuh::arr::Device>>(pool, arr_size);
   // modifiers
   d1.fromHost(begin(h1), end(h1));

   // accessors
   d3.toHost(begin(h1));
   d3.toHost(begin(h1), [](auto x){return x*3;});
   d3.toHost(begin(h1), arr_size, [](auto x){return x*4;});
   auto h2 = d4.toHost<std::vector<float>>();
}

// provide host-side random access
auto test_mapped_arrays_float()-> void {
   constexpr auto arr_size = size_t(128);
   auto h1 = std::vector<float>(arr_size);

	auto instance = vuh::Instance();
	auto device = instance.devices().at(0);  // just get the first compute-capable device

   auto d1 = vuh::Array<float, vuh::mem::Unified>(device, arr_size);
   auto d2 = vuh::Array<float, vuh::mem::Unified>(device, arr_size, 2.71f);
   auto d6 = vuh::Array<float, vuh::mem::Unified>(device, begin(h1), end(h1));

   // random-access iterable
   std::copy(begin(d2), end(d2), begin(h1));
   d6[42] = 42.f;
}

// same host access interface as DeviceMapped arrays
auto test_host_arrays_float()-> void {
   constexpr auto arr_size = size_t(128);
   auto h1 = std::vector<float>(arr_size);

	auto instance = vuh::Instance();
	auto device = instance.devices().at(0);  // just get the first compute-capable device

   auto d1 = vuh::Array<float, vuh::mem::Host>(device, arr_size);
   auto d2 = vuh::Array<float, vuh::mem::Host>(device, arr_size, 2.71f);
   auto d6 = vuh::Array<float, vuh::mem::Host>(device, begin(h1), end(h1));

   // random access iterable
   std::copy(begin(d2), end(d2), begin(h1));
   d2[42] = 42.f;
}

auto main()-> int {
	return 0;
}
