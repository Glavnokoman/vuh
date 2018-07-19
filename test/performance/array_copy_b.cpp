#include <sltbench/Bench.h>

#include <vuh/array.hpp>
#include <vuh/vuh.h>

#include <cstdlib>
#include <memory>
#include <vector>

namespace {

	/// Push-parameters to the kernel + some aux functions
	struct Params{
		uint32_t size; ///< size of a vector

		auto operator== (const Params& other) const-> bool {return size == other.size;}
		auto operator!= (const Params& other) const-> bool {return !(*this == other);}

		friend auto operator<< (std::ostream& s, const Params& p)-> std::ostream& {
			return s << "{" << p.size << "}";
		}
	};

	using Program = vuh::Program<vuh::typelist<uint32_t>, Params>;

	auto instance = vuh::Instance();
	vuh::Device device = instance.devices().at(0);             ///< gpu device
//	Program program = Program(device, "../shaders/saxpy.spv"); ///< kernel to run

	///
	struct HostData {
		std::vector<float> host_array;
		vuh::Array<float, vuh::mem::Host> device_array{device, 64};
	};

	/// Fixture to just create and keep the host data
	struct FixCreateHostData: private HostData {
		using Type = HostData;

		auto SetUp(const Params& p)-> Type& {
			if(p.size != host_array.size()) {
				this->host_array = std::vector<float>(p.size);
				this->device_array = vuh::Array<float, vuh::mem::Host>(device, p.size);
				std::generate(begin(this->host_array), end(this->host_array), std::rand);
			}
			return *this;
		}

		auto TearDown()-> void {}
	};

	/// Benchmarked function.
	/// Copy host data to device host-visible memory
	/// Assumed to work with FixCreateHostData fixture.
	auto copy_host_to_host_visible(HostData& data, const Params& /*params*/)-> void {
		std::copy(begin(data.host_array), end(data.host_array), data.device_array.begin());
	}

	/// doc me
	auto copy_host_visible_to_host(HostData& data, const Params& /*params*/ )-> void {
		std::copy(data.device_array.begin(), data.device_array.end(), begin(data.host_array));
	}

	/// Set of parameters to run benchmakrs on.
	static const auto params = std::vector<Params>({{32u}, {128u}, {1024u}});
} // namespace

SLTBENCH_FUNCTION_WITH_FIXTURE_AND_ARGS(copy_host_to_host_visible, FixCreateHostData, params)
SLTBENCH_FUNCTION_WITH_FIXTURE_AND_ARGS(copy_host_visible_to_host, FixCreateHostData, params)


SLTBENCH_MAIN()
