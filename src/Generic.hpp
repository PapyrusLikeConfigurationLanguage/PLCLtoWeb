// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <utility>
#include <libPLCL.hpp>

using namespace PLCL;

inline static std::string attributeValueToString(Generic::ValueType value) {
    if (std::holds_alternative<std::string>(value)) {
        return std::get<std::string>(value);
    } else if (std::holds_alternative<int64_t>(value)) {
        return std::to_string(std::get<int64_t>(value));
    } else if (std::holds_alternative<Generic::float64_t>(value)) {
        return std::to_string(std::get<Generic::float64_t>(value));
    } else if (std::holds_alternative<bool>(value)) {
        return std::to_string(std::get<bool>(value));
    } else {
        std::unreachable();
    }
}