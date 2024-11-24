// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <filesystem>
#include <string>
#include <vector>

struct Cli {
    std::vector<std::filesystem::path> files;
    std::filesystem::path output;
    bool help = false;
    bool version = false;
    bool dontMinify = false;
    bool watch = false;
    size_t indent = 4;
    std::string executableName;

    Cli(int argc, char *argv[]);
    void printHelp() const;
    static void printVersion();
};
