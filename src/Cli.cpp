// SPDX-License-Identifier: GPL-3.0-only

#include <cstring>
#include <iostream>

#include "Cli.hpp"
#include "CMakeInfo.hpp"

Cli::Cli(int argc, char *argv[]) {
    if (argc == 0) {
        this->executableName = "PLCLToWeb";
    } else {
        this->executableName = argv[0];
    }
    if (argc < 2) {
        this->help = true;
    } else {
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
                if (i + 1 < argc) {
                    this->output = std::filesystem::path(argv[++i]).lexically_normal();
                } else {
                    std::cerr << "Expected output file after -o" << std::endl;
                    std::exit(1);
                }
            } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
                this->help = true;
            } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
                this->version = true;
            } else if (strcmp(argv[i], "--dont-minify") == 0) {
                this->dontMinify = true;
            } else if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--watch") == 0) {
                this->watch = true;
            } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--indent") == 0) {
                if (i + 1 < argc) {
                    if (std::isdigit(argv[++i][0])) {
                        this->indent = std::stoul(argv[i]);
                    } else {
                        std::cerr << "Expected positive number after --indent" << std::endl;
                        std::exit(1);
                    }
                } else {
                    std::cerr << "Expected indent after --indent" << std::endl;
                    std::exit(1);
                }
            }
            else {
                this->files.emplace_back(std::filesystem::absolute(argv[i]).lexically_normal());
            }
        }
    }
    if (this->files.empty()) {
        this->help = true;
    }
    if (this->output.empty()) {
        this->output = std::filesystem::current_path().lexically_normal();
    }
}

void Cli::printHelp() const {
    std::cout << "Usage: " << this->executableName << " [options] [files...]\n"
    "Options:\n"
    "  -h, --help  Display this information\n"
    "  -o, --output <path>  Output directory\n"
    "  -v, --version  Display version information\n"
    "  -w, --watch  Watch files for changes\n"
    "  --dont-minify  Don't minify the output\n"
    "    -i, --indent <size>  Indent size (in spaces), defaults to 4\n"
    "Supported file extensions:\n"
    "  .p(l)clhtml  HTML files\n"
    "  .p(l)clcss  CSS files"
    << std::endl;
}

void Cli::printVersion() {
    std::cout << "PLCLToWeb " << VERSION_MAJOR << "." << VERSION_MINOR << "." << VERSION_PATCH << std::endl;
}
