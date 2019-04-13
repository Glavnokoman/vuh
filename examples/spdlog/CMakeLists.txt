cmake_minimum_required(VERSION 3.8)
project(vuh_example_spdlog)

find_package(Vuh REQUIRED)

find_package(spdlog)
if(TARGET spdlog::spdlog)
   find_package(Threads REQUIRED)

   add_executable(vuh_example_spdlog main.cpp)
   target_link_libraries(vuh_example_spdlog PUBLIC vuh::vuh spdlog::spdlog Threads::Threads)
else()
   message("Vuh spdlog example requires 'spdlog' library")
endif()
