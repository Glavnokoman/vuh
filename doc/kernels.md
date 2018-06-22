# Kernels Usage
Kernel (```vuh::Program```) declaration and calls on a client (CPU side) should match the interface of the ```SPIR-V``` program executed on a GPU.
That interface consists mostly of Push Constants, Specialization Constants and Descriptor sets (buffer parameters).
This section sets up the correspondence between the two and illustrates different ways to pass the parameters and execute kernels.

The object of ```vuh::Program``` is initialized on a given device from the file containing the ```SPIR-V```code (or code itself in the form of binary dump std::vector<char>).
```cpp
auto program = vuh::Program(device, "kernel.spv");
```
In case the corresponding shader has non-empty specialization and/or push constants
interface that should be reflected by a template parameters to ```vuh::Program<Specs, PushConstants>```.

## Specialization Constants
Specialization constants are ```int```, ```float``` or ```bool``` values used as constants in the shader code but can be set up in the client code prior to kernel execution.
In shader specialization constants are declared like
```glsl
layout(constant_id = 0) const int arraySize;
layout(constant_id = 1) const float a = 0.1f; // with a default value
```
Here ```constant_id```-s are used to refer to particular constants in the client code. ```Vuh``` assumes the through numbering of constants starting from 0.
The client side ```vuh::Program``` declaration corresponding to above two constants interface would write
```cpp
using Specs = vuh::typelist<int, float>;
auto program = vuh::Program<Specs>(device, "kernel.spv");
```
Namely for each constant in the order of their IDs its type should be put into a typelist which is used to specialize the ```vuh::Program``` template.
To specify the value of constants call ```Program::spec()```.
```cpp
program.spec(42, 3.14f);
```
would set the above values of ```arraySize``` to 42 and ```a``` to 3.14.
These constants sustain their values until the next call to ```Program::spec()``` method.
Not setting specialization constants before launching a kernel would not trigger an error since ```vuh``` is not informed whether constants have default values or not.
One of the use of specialization constants might be setting up the workgroup dimensions:
```glsl
layout(local_size_x_id = 0, local_size_y_id = 1) in; // set up 2d workgroup
```

## Push Constants
Push constants are small structures (normally 128 Bytes) residing in the uniform block.
On client side those structures are pushed to a push-constants buffer and shader reads them before execution from a push_constant block.
In a shader code their declaration and usage look like
```glsl
layout(push_constant) uniform BlockName {
   float translation[3];
   float rotation[9];
   int time;
} instanceName; // instance name

 ... = instanceName.member2; // read a push constant in the shader body
```
On the client side there must be an exact same structure declared and passed as a second template parameter to ```vuh::Program```
```cpp
struct PushConstants {
   float translation[3];
   float rotation[9];
   int time;
};
auto program = vuh::Program<Specs, PushConstants>(device, "kernel.spv");
```
Here ```Specs``` typelist from a previous subsection used to define the specialization constants interface.
In case there are no specialization constants empty typelist should be passed as a template parameter ```vuh::Program<vuh::typelist<>, PushConstants>(...)```.
Currently ```Vuh``` only supports a single push constant per kernel.
Luckily this limitation is easy to bypass since structures in ```C++``` and ```GLSL``` are composable.
Push constants are set in ```vuh::Program::bind()``` call, which accepts push constants structure as a first parameter (unless push constants template parameter is omitted or explicitly set to empty typelist)
```cpp
```

## Array Parameters
In ```GLSL``` code array parameters are bound to variables by means of descriptors
```glsl
layout(std430, binding = 0) buffer lay0 { float arr_y[]; };
layout(std430, binding = 1) buffer lay1 { float arr_x[]; };
```
which allows to bind some buffers to the names ```arr_y```, ```arr_x``` and access their values in the body of ```GLSL``` program by indexes. For example ```arr_y[0]``` would access the first element of the buffer currently bound to ```arr_y```.
The binding is done on the CPU side of the program and handled by ```vuh```.

## Execution
### One-off
### Rebinding & Restarting
