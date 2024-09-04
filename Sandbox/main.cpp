#include <spdlog/spdlog.h>
#include "HelloTriangleApplication.h"

int main()
{
	try
	{
		CHelloTriangleApplication App;
		App.run();
	}
	catch (const std::exception& e) {
		spdlog::error(e.what());
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
