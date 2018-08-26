```Vuh``` helps to connect the CPU side of a ```C++``` ```GPGPU``` program using ```Vulkan```
to its GPU part (```SPIR-V``` binary).
It does not provide a complete isolation from ```Vulkan``` API
but rather a set of helpers to eliminate the boilerplate.
Knowledge of ```Vulkan``` programming model is not required to understand this tutorial
or start using ```vuh```.
It could be helpful however for better understanding of what is going on
and at the later stages.
In this tutorial the term ```kernel``` is used interchangeably with
```compute shader``` to designate a program to be executed on GPU.
The details of writing ```GLSL``` compute shaders as one of the main sources of ```SPIR-V```
binaries are to be read somewhere else.
Only the parts of it directly interfacing with the ```C++``` side are discussed.

- [Project setup](project_setup.md)
- [Basic program structure](basic_program_structure.md)
- [Arrays usage](array_usage.md)
- [Kernels usage](kernels.md)
- [Async operations](async_operations.md)
