// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <iostream>
#include <map>
#include <libPLCL.hpp>

using namespace PLCL;

std::string parseCSS(const Config::ConfigRoot &input, bool minify, size_t indent);
