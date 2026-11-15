#include "app/app_controller.h"

int main(int argc, char* argv[]) {
    const stegoscan::CliOptions options = stegoscan::parse_cli_args(argc, argv);
    return stegoscan::run_application(options);
}
