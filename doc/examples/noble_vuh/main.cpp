#include <vuh/array.hpp>
#include <vuh/vuh.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <time.h>

using namespace std;

auto main()-> int {
	const float dt_sim = 0.1f; //.01simuliacijos dT ms
	const float sim_t = 1000; //simulacijos trukme ms
	const float dt_reg = 2; //.01isvedimo dT ms
	const uint N_reg = sim_t / dt_reg;// koks isvedimo matricu dydis
	const uint N_sim = sim_t / dt_sim; // simulacijos trukme, iteracijomis
	const uint cellCountY = 64;
	const uint cellCountX = 256;
	uint t_start, t_end;

	long long mem = sizeof(float)*(2*cellCountY*cellCountX*7*4 + cellCountY*cellCountX*6
	                               + 2*36*cellCountY*cellCountX
	                               + cellCountY*cellCountX*N_reg*3)/1.0e6;
	printf("mem size =  %d MB\n", mem);
	if (mem > 1000) { printf("continue?\n");getchar(); }

	const auto width = cellCountX;
	const auto height = cellCountY;

	auto n = std::vector<float>(cellCountY*cellCountX, 0);
	auto m = std::vector<float>(cellCountY*cellCountX, 0);
	auto h = std::vector<float>(cellCountY*cellCountX, 0);
	auto I = std::vector<float>(cellCountY*cellCountX, 0);
	auto V_YX = std::vector<float>(cellCountY*cellCountX, -85.f); // only device, TODO: ommit this line
	auto V_tYX_reg = std::vector<float>(cellCountY*cellCountX*N_reg, 0); //only for registration

	auto instance = vuh::Instance();
	auto device = instance.devices().at(0); // just get the first compute-capable device

	auto dev_n = vuh::Array<float>(device, n);
	auto dev_m = vuh::Array<float>(device, m);
	auto dev_h = vuh::Array<float>(device, h);
	auto dev_I = vuh::Array<float>(device, I);
	auto dev_V_YX = vuh::Array<float>(device, V_YX);
	auto dev_V_tYX_reg = vuh::Array<float>(device, V_tYX_reg);

	using Specs = vuh::typelist<uint32_t, uint32_t>;   // specs: currently- width, height of a workgroup
	struct Params{uint32_t width; uint32_t height; float dt_sim; uint32_t i;};   // push constants
	//    struct Params2{uint32_t width; uint32_t height; uint32_t i;};
	//    auto program_clr = vuh::Program<Specs, Params>(device, "kernel_clr.spv"); //clear (set to 0) float buffer

	auto program1 = vuh::Program<Specs, Params>(device, "kernel_noble_1.spv");
	auto program2 = vuh::Program<Specs, Params>(device, "kernel_noble_2.spv");
	program1.grid(width/32, height/32).spec(32, 32);
	program2.grid(width/32, height/32).spec(32, 32);

	// warm-up
	for(uint32_t i = 0; i < 100; i++) {
		uint i_reg = (N_reg*i)/N_sim;
		program1.run({width, height, dt_sim, i_reg}, dev_n, dev_m, dev_h, dev_I, dev_V_YX);
		program2.run({width, height, dt_sim, i_reg}, dev_I, dev_V_YX, dev_V_tYX_reg);
	}

	t_start = clock();
	for (uint32_t i=0; i < N_sim; i++) {
		uint i_reg = (N_reg*i)/N_sim;
		program1.run({width, height, dt_sim, i_reg}, dev_n, dev_m, dev_h, dev_I, dev_V_YX);
		program2.run({width, height, dt_sim, i_reg}, dev_I, dev_V_YX, dev_V_tYX_reg);
	}
	t_end = clock();
	std::cout << "\nTime required for execution " << N_sim << " times: "
	          << (float)(t_end - t_start) / CLOCKS_PER_SEC  << " seconds." << "\n";
	dev_V_tYX_reg.toHost(V_tYX_reg.begin());

	ofstream myfile("noble_outfile_V.binary", myfile.out | myfile.binary);
	if (!myfile.is_open()) { cout << "\nFailed to open file for write\n"; return 0;  }
	myfile.write((char*)V_tYX_reg.data(), sizeof(float)*width*height*N_reg);
	myfile.close();

	printf("noble_outfile_V.binary wrote");
	cout << "\nDone\n";

	return 0;
}
