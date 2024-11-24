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

enum VariableValueType {
    LITERAL,
    LITERAL_ARRAY,
    ELEMENT,
};

typedef std::vector<std::string> LiteralArray;
typedef std::variant<std::string, LiteralArray, Config::ConfigElement> VariableValue; // maybe make it lighter

std::string parseHTML(const Config::ConfigRoot &input, bool minify, size_t indent);
