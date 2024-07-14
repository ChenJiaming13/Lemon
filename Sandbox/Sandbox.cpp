#include <spdlog/spdlog.h>
#include "HelloTriangleApplication.h"

int main()
{
	spdlog::set_level(spdlog::level::level_enum::trace);
	CHelloTriangleApplication App;
	App.run();
	return 0;
}