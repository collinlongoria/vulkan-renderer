// Collin Longoria
// 10/16/24

#include <iostream>
#include <stdexcept>
#include <cstdlib>

#include <HelloTriangle.h>

int main() {
    HelloTriangle app;

    try {
        app.run();
    } catch(const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
