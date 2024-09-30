#include "Server.hpp"
#include <iostream>

int main() {
	try {
		Server server(8080, 10);
		server.run();
		
		return 0;
	} catch (const std::exception& e) {
		std::cerr << "err: " << e.what() << std::endl;
		return EXIT_FAILURE;
		
	}
}
