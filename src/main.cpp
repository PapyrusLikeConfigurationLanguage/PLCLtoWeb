// SPDX-License-Identifier: GPL-3.0-only

#include <iostream>
#include <fstream>
#include <cstring>
#include <filesystem>
#include <libPLCL.hpp>

#include "CMakeInfo.hpp"
#include "CSS.hpp"
#include "HTML.hpp"

using namespace PLCL;

struct Cli {
    std::vector<std::string> files;
    std::string output = ".";
    bool help = false;
    bool version = false;
    bool dontMinify = false;
    size_t indent = 4;
};

int main(int argc, char *argv[]) {
    Cli cli;
    if (argc == 1) {
        cli.help = true;
    } else {
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
                if (i + 1 < argc) {
                    i++;
                    cli.output = std::string(argv[i]);
                } else {
                    std::cerr << "Expected output file after -o" << std::endl;
                    return 1;
                }
            } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
                cli.help = true;
            } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
                cli.version = true;
            } else if (strcmp(argv[i], "--dont-minify") == 0) {
                cli.dontMinify = true;
            } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--indent") == 0) {
                if (i + 1 < argc) {
                    i++;
                    if (std::isdigit(argv[i][0])) {
                        cli.indent = std::stoul(argv[i]);
                    } else {
                        std::cerr << "Expected positive number after --indent" << std::endl;
                        return 1;
                    }
                } else {
                    std::cerr << "Expected indent after --indent" << std::endl;
                    return 1;
                }
            }
            else {
                cli.files.emplace_back(argv[i]);
            }
        }
    }
    if (cli.version) {
        std::cout << "PLCLToWeb " << VERSION_MAJOR << "." << VERSION_MINOR << "." << VERSION_PATCH << std::endl;
        return 0;
    }
    if (cli.files.empty()) {
        cli.help = true;
    }
    if (cli.help) {
        std::cout << "Usage: " << argv[0] << " [options] [files...]" << '\n';
        std::cout << "Options:" << '\n';
        std::cout << "  -h, --help  Display this information" << '\n';
        std::cout << "  -o, --output <path>  Output directory" << '\n';
        std::cout << "  -v, --version  Display version information" << '\n';
        std::cout << "  --dont-minify  Don't minify the output" << '\n';
        std::cout << "    -i, --indent <size>  Indent size (in spaces), defaults to 4" << '\n';
        std::cout << "Supported file extensions:" << '\n';
        std::cout << "  .p(l)clhtml  HTML files" << '\n';
        std::cout << "  .p(l)clcss  CSS files" << '\n';
        return 0;
    }
    std::filesystem::create_directories(cli.output);
    for (const auto & file : cli.files) {
        std::ifstream ifs(file);
        std::string content((std::istreambuf_iterator<char>(ifs)),
                            (std::istreambuf_iterator<char>()));
        Config::ConfigRoot config = Config::ConfigRoot::fromString(content);
        size_t dot = file.find_last_of('.');
        std::string extension = file.substr(dot + 1);
        std::string output = cli.output + "/" + config.name + ".";
        if (Generic::iequals(extension, "p(l)clhtml")) {
            output += "html";
            std::string result = parseHTML(config, !cli.dontMinify, cli.indent);
            std::ofstream ofs(output);
            ofs << result;
        } else if (Generic::iequals(extension, "p(l)clcss")) {
            output += "css";
            std::string result = parseCSS(config, !cli.dontMinify, cli.indent);
            std::ofstream ofs(output);
            ofs << result;
        } else {
            std::cerr << "Unknown extension " << extension << std::endl;
            return 1;
        }
    }
    return 0;
}
