#include "app/app.h"
#include <print>
#include <cstdlib>

int main() {
    auto app = App::create();
    if (!app) {
        std::println(stderr, "{}", app.error());
        return EXIT_FAILURE;
    }
    app->run();
    return EXIT_SUCCESS;
}
