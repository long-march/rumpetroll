
#include <iostream>
#include <fstream>
#include <array>

#include "../include/YggdrasilCore.h"

using namespace Yggdrasil;

int main(int argc, char ** argv)
{
	Program st;

	st.configure_io(1, 1);
	
	debug_callback = [] (std::string message) { std::cout << message; };

	/*
 	if (argc >= 2)
 	{
 		std::ifstream file(argv[1]);
 
 		if (file) st.make_graph(file);
 		else std::cout << "What?\n";
 	}
 	else
 		st.make_graph(std::cin);
	*/

	std::string code = "mk mult mult 5\nct input0>mult0\nct mult0>output0\ndone\n";
	st.make_graph(std::stringstream(code));
	
	for (uint n = 0; n < 100; n++) std::cout << st.run(n) << '\n';
}
