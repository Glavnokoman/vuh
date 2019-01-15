## Dependencies
- C++14 compliant compiler
- [CMake](https://cmake.org/download/) (build-only)
- [Vulkan-Headers](https://github.com/KhronosGroup/Vulkan-Headers)
- [Vulkan-Loader](https://github.com/KhronosGroup/Vulkan-Loader)
- [Glslang](https://github.com/KhronosGroup/glslang) (optional, build-only)
- [Catch2](https://github.com/catchorg/Catch2) (optional, build-only)
- [sltbench](https://github.com/ivafanas/sltbench) (optional, build-only)
- [spdlog](https://github.com/gabime/spdlog) (>=1.2.1)

## Compile with dependencies already in place
This assumes required dependencies are already present on your system and findable by ```cmake```.
Replace ```VUH_SOURCE_DIR``` by the path to vuh sources on your setup and build out-of-source.
```bash
cmake ${VUH_SOURCE_DIR}
cmake --build . --target install
```

## Install dependencies & compile
### Linux & Unix-like OSes
Install script depends on ```cmake``` and ```cget``` (```pip install cget```) to be available.
Replace ```VUH_SOURCE_DIR``` and ```DEPENDENCIES_INSTALL_DIR``` by their values on your system.
Build out of source.
```bash
export CGET_PREFIX=${DEPENDENCIES_INSTALL_DIR}
${VUH_SOURCE_DIR}/config/install_dependencies.sh
cmake -DCMAKE_PREFIX_PATH=${DEPENDENCIES_INSTALL_DIR} ${VUH_SOURCE_DIR}
cmake --build . --target install
```
### Windows
TBD.
Should be similar to above.

### macOS
```bash
export CGET_PREFIX=${DEPENDENCIES_INSTALL_DIR}
${VUH_SOURCE_DIR}/config/install_dependencies.sh
brew install spdlog
cmake -DCMAKE_PREFIX_PATH=${DEPENDENCIES_INSTALL_DIR} ${VUH_SOURCE_DIR}
cmake --build . --target install
```
