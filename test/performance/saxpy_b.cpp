#include <sltbench/Bench.h>

#include <example_filter.h>
#include <vulkan_helpers.hpp>

#include <memory>
#include <vector>

namespace {

struct Params{
   uint32_t width;
   uint32_t height;
   float a;
   
   auto operator== (const Params& other) const-> bool {
      return width == other.width && height == other.height && a == other.a;
   }
   auto operator!= (const Params& other) const-> bool { return !(*this == other); }
   
   friend auto operator<< (std::ostream& s, const Params& p)-> std::ostream& {
      s << "{" << p.width << ", " << p.height << ", " << p.a << "}";
      return s;
   }
};

struct DataFixFull {
   ExampleFilter f{"shaders/saxpy.spv"};
   Params p;
   std::vector<float> y;
   std::vector<float> x;
};

struct FixSaxpyFull: private DataFixFull {
   using Type = DataFixFull;
   
   auto SetUp(const Params& p)-> Type& {
      if(p != this->p) {
         this->p = p;
         y = std::vector<float>(p.width*p.height, 3.1f);
         x = std::vector<float>(p.width*p.height, 1.9f);
      }
      return *this;
   }
   
   auto TearDown()-> void {}
}; // class FixSaxpyFull

struct FixShaderOnly: private DataFixFull {
   using Type = ExampleFilter;
   
   struct DeviceData{
      explicit DeviceData(const DataFixFull& d)
         : d_y{vuh::Array<float>::fromHost(d.y, d.f.device, d.f.physDevice)}
         , d_x{vuh::Array<float>::fromHost(d.x, d.f.device, d.f.physDevice)}
      {}
      
      vuh::Array<float> d_y;
      vuh::Array<float> d_x;
   };
   
   
   auto SetUp(const Params& p)-> Type& {
      if(p != this->p){
         this->p = p;
         y = std::vector<float>(p.width*p.height, 3.1f);
         x = std::vector<float>(p.width*p.height, 1.9f);
         
         f.unbindParameters();
         _dev_data = std::make_unique<DeviceData>(static_cast<const DataFixFull&>(*this));
         f.bindParameters(_dev_data->d_y, _dev_data->d_x, {p.width, p.height, p.a});
      }
      return f;
   }
   
   auto TearDown()-> void {}
   
private:
   std::unique_ptr<DeviceData> _dev_data;
}; // struct FixShaderOnly

/// Copy arrays data to gpu device, setup the kernel and run it.
auto saxpy(DataFixFull& fix, const Params& p)-> void {
   auto d_y = vuh::Array<float>::fromHost(fix.y, fix.f.device, fix.f.physDevice);
   auto d_x = vuh::Array<float>::fromHost(fix.x, fix.f.device, fix.f.physDevice);
   
   fix.f(d_y, d_x, {fix.p.width, fix.p.height, fix.p.a});
}

/// Just run the kernel, assumes the data has been copied and shader is all set up.
auto saxpy(ExampleFilter& f, const Params& p)-> void {
   f.run();
}

static const auto params = std::vector<Params>({{32u, 32u, 2.f}, {128, 128, 2.f}, {1024, 1024, 3.f}});

} // namespace


SLTBENCH_FUNCTION_WITH_FIXTURE_AND_ARGS(saxpy, FixSaxpyFull, params);
SLTBENCH_FUNCTION_WITH_FIXTURE_AND_ARGS(saxpy, FixShaderOnly, params);

SLTBENCH_MAIN();
