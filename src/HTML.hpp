// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <iostream>
#include <libPLCL.hpp>

using namespace PLCL;

inline const std::string VOID_ELEMENTS[] = {
    "area",
    "base",
    "br",
    "col",
    "command",
    "embed",
    "hr",
    "img",
    "input",
    "keygen",
    "link",
    "meta",
    "param",
    "source",
    "track",
    "wbr"
};

std::string parseHTML(const Config::ConfigRoot &input, bool minify, size_t indent);
