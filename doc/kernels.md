# Kernels Usage
To execute the kernel on a GPU one must first create the object of type ```vuh::Program``` matching the kernel interfaces (specialization constants, push constants and buffer bindings)
```cpp
auto program = vuh::Program(device, "kernel.spv");
```
and linking together the ```SPIR-V``` binary and a device to execute that code.
This object is used to set the execution grid sizes ```Program::grid()``` (amount of workgroups in each dimension), specify the values of kernel parameters (by calling ```Program::spec()``` and ```Program::bind()``` functions), and finally launch the kernel ```Program::run()```.
The workgroup dimensions are either hardcoded in the kernel code, or passed via specialization or push constants.
This section sets up the correspondence between the two sides and illustrates different ways to pass the parameters and execute kernels.

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
On the client side specialization constants interface is defined by the first template parameter of ```vuh::Program<>``` class.
The declaration corresponding to above two constants interface would write
```cpp
using Specs = vuh::typelist<int, float>;
auto program = vuh::Program<Specs>(device, "kernel.spv");
```
Namely for each constant in the order of their IDs its type should be put into a typelist used to specialize the ```vuh::Program```.
All constants are specified in a single call to ```Program::spec()``` with each parameter corresponding to constants in the order of their id-s.
```cpp
program.spec(42, 3.14f);
```
would set the above values of ```arraySize``` to 42 and ```a``` to 3.14.
Constants sustain their values until the next call to ```Program::spec()``` method.
Not setting specialization constants before launching a kernel would not trigger an error since ```vuh``` is not informed whether constants have default values or not.
One of the use of specialization constants might be setting up the workgroup dimensions:
```glsl
layout(local_size_x_id = 0, local_size_y_id = 1) in; // set up 2d workgroup
```

## Push Constants
Push constants are small structures (normally <= 128 Bytes) residing in the uniform block.
In a shader code their declaration and usage look like
```glsl
layout(push_constant) uniform BlockName {
   float x;
   float y;
   int t;
} instanceName; // instance name

 ... = instanceName.member2; // read a push constant in the shader body
```
On the client side there must be exact same structure declared and passed as a second template parameter to ```vuh::Program```
```cpp
struct PushConstants {
   float x;
   float y;
   int t;
};
auto program = vuh::Program<Specs, PushConstants>(device, "kernel.spv");
```
Here ```Specs``` typelist from a previous subsection used to define the specialization constants interface.
In case there are no specialization constants empty typelist should be passed as a template parameter ```vuh::Program<vuh::typelist<>, PushConstants>(...)```.
Currently ```Vuh``` only supports a single push constant structure per kernel.
Luckily this limitation is easy to bypass since structures in ```C++``` and ```GLSL``` are composable.
Push constants are set in ```vuh::Program::bind()``` call, which accepts push constants structure as a first parameter (unless push constants template parameter is omitted or explicitly set to the empty typelist)
```cpp
program.bind({3.14, 6.28, 42}, ...);
```

## Buffer Bindings
Storage buffers are used to input/output array-like parameters to a kernel.
They are declared in shader source using buffer storage qualifier and block syntax
```glsl
layout(std430, binding = 0) buffer lay0 { float arr_floats[]; };
layout(std430, binding = 0) buffer lay0 { int arr_ints[]; };
```
which allows to bind some buffers to the names ```arr_floats```, ```arr_ints``` and access their values with indexes.
Currently ```vuh``` only supports a single set per kernel and all bindings within a kernel must have consecutive id-s that define the order in which array parameters passed to kernel in ```vuh::Program::bind()``` call
```cpp
auto array_floats = vuh::Array<float>(device, ...);
auto array_ints = vuh::Array<int>(device, ...);
...
program.bind(array_floats, array_ints);
```
Such a call associates the storage buffers allocated on a device to the binding points in a shader code which is to be run.
Naturally buffers must be allocated on the same device where kernel is executed.
In case kernel has both push constants and storage buffers interface the push constants structure is passed as the first parameter to ```bind()``` and all buffer parameters follows
```cpp
program.bind({3.14, 6.28, 42}, array_floats, array_ints);
```
Array types and number on a ```C++``` side should match those in a kernel.
Array parameters bound like this can be used as both input and output parameters.
It is required that prior to calling ```bind()``` function one specifies the grid size and specialization constants (if applicable).
No error will be reported in case one forgets to do that.

## Execution
Once grid dimensions and all shader parameters are specified kernel may be scheduled for an execution on a GPU device.
This is done simply by calling ```Program::run()``` function which triggers kernel execution and returns control to the program once computation is complete.
In simple cases setting all required parameters may be chained, and a convenience ```Program::operator()``` used which combines the ```Program::bind()``` and ```Program::run()``` in a one-liner ``` program.grid().spec()({}, ...); ```.

Program can be rerun multiple times without rebinding or respecifying any parameters (for example when the data in input buffers is updated)
```cpp
...
program1.bind(array1, array2);
program2.bind(array2, array1);
for(;;){ // perpetual buzz
   program1.run(); // update array 1
   program2.run(); // update array 2
}
```
Alternatively one can set up the grid dimensions and specialization constants once, and rebind-rerun the kernel repeatedly if such a need occurs
```cpp
program.grid(128/64).spec(64);
for(size_t i = 0; i < n_repeat; ++i){
   program({128, a}, d_y, d_x);
}
```
