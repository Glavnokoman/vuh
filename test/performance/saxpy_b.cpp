#include <sltbench/Bench.h>

#include <vuh/array.hpp>
#include <vuh/vuh.h>

#include <memory>
#include <vector>

namespace {

	/// Push-parameters to the kernel + some aux functions
	struct Params{
		uint32_t size; ///< size of a vector
		float a;       ///< saxpy scaling parameter

		auto operator== (const Params& other) const-> bool {return size == other.size && a == other.a;}
		auto operator!= (const Params& other) const-> bool {return !(*this == other);}

		friend auto operator<< (std::ostream& s, const Params& p)-> std::ostream& {
			return s << "{" << p.size << ", " << p.a << "}";
		}
	};

	using Program = vuh::Program<vuh::typelist<uint32_t>, Params>;

	auto instance = vuh::Instance();
	vuh::Device device = instance.devices().at(0);             ///< gpu device
	Program program = Program(device, "../shaders/saxpy.spv"); ///< kernel to run

	///
	struct DataHostVisible {
		Params p;
		std::vector<float> y;
		std::vector<float> x;
	};

	/// Fixture to just create and keep the host data
	struct FixDataHostVisible: private DataHostVisible {
		using Type = DataHostVisible;

		auto SetUp(const Params& p)-> Type& {
			if(p != this->p) {
				this->p = p;
				this->y = std::vector<float>(p.size, 3.14f);
				this->x = std::vector<float>(p.size, 6.28f);
			}
			return *this;
		}

		auto TearDown()-> void {}
	}; // class FixSaxpyFull

	/// Fixture copying data to device-local memory and binding all parameters to the kernel.
	struct FixCopyDataBindAll: private DataHostVisible {
		using Type = Program;
		static constexpr auto workgroup_size = 128u;

		auto SetUp(const Params& p)-> Type& {
			if(p != this->p){
				this->p = p;
				this->y = std::vector<float>(p.size, 3.14f);
				this->x = std::vector<float>(p.size, 6.28f);

				d_y = vuh::Array<float>(device, this->y);
				d_x = vuh::Array<float>(device, this->x);
				program.grid(p.size/workgroup_size)
				       .spec(workgroup_size)
				       .bind(p, d_y, d_x);
			}
			return program;
		}

		auto TearDown()-> void {}

	private:
		vuh::Array<float> d_y = vuh::Array<float>(device, 64);
		vuh::Array<float> d_x = vuh::Array<float>(device, 64);
	}; // struct FixCopyDataBindAll

	/// Benchmarked function.
	/// Copy host data to device-local memory, bind parameters and run the kernel.
	/// This is assumed to work with FixCreateHostData fixture.
	auto saxpy(DataHostVisible& data, const Params& /*params*/)-> void {
		auto d_y = vuh::Array<float>(device, data.y);
		auto d_x = vuh::Array<float>(device, data.x);

		program(data.p, d_y, d_x);
	}

	/// Benchmarked function.
	/// Just run the kernel, assumes the data copied and kernel all set up.
	/// This is supposed to be combined with FixCopyDataBindAll fixture.
	auto saxpy(Program& program, const Params& /*p*/)-> void {
		program.run();
	}

	/// Set of parameters to run benchmakrs on.
	static const auto params = std::vector<Params>({{32u, 2.f}, {128u, 2.f}, {1024u, 3.f}});
} // namespace

SLTBENCH_FUNCTION_WITH_FIXTURE_AND_ARGS(saxpy, FixDataHostVisible, params)
SLTBENCH_FUNCTION_WITH_FIXTURE_AND_ARGS(saxpy, FixCopyDataBindAll, params)

SLTBENCH_MAIN()
