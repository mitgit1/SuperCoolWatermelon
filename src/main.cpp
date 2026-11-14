#include "app/Application.h"
#include <iostream>

int main() {
    try {
        Application app;
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "FATAL: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
