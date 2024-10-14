// SPDX-License-Identifier: GPL-3.0-only

#include "CSS.hpp"

std::string elementInsideHelper(const Config::ConfigElement &input, bool minify, size_t indent) {
    std::string result;
    for (const auto &attribute : input.attributes) {
        if (attribute->name[0] == '_') {
            continue;
        }

        std::string newName = attribute->name;
        for (size_t i = 0; i < newName.size(); i++) {
            if (isupper(newName[i]) && i != 0) {
                newName.insert(i, "-");
                i++;
            }
        }
        std::ranges::transform(newName, newName.begin(), ::tolower);
        if (!minify) {
            result += std::string(indent, ' ');
        }
        result += newName + ":";
        if (!minify) {
            result += " ";
        }
        if (std::holds_alternative<std::string>(attribute->value)) {
            result += std::get<std::string>(attribute->value);
        } else if (std::holds_alternative<int64_t>(attribute->value)) {
            result += std::to_string(std::get<int64_t>(attribute->value));
        } else if (std::holds_alternative<Generic::float64_t>(attribute->value)) {
            result += std::to_string(std::get<Generic::float64_t>(attribute->value));
        } else if (std::holds_alternative<bool>(attribute->value)) {
            result += std::to_string(std::get<bool>(attribute->value));
        }
        result += ";";
        if (!minify) {
            result += "\n";
        }
    }
    return result;
}

std::string elementHelper(const Config::ConfigElement &input, const std::string_view& name, const std::map<std::string, std::string> &templates, bool minify, size_t indent) {
    std::string result;
    std::string afterMain;
    std::string type;
    for (const auto &attribute : input.attributes) {
        if (Generic::iequals(input.type, "_all")) {
            type = "tag";
            break;
        }
        if (Generic::iequals(attribute->name, "_type")) {
            if (std::holds_alternative<std::string>(attribute->value)) {
                type = std::get<std::string>(attribute->value);
            } else if (std::holds_alternative<int64_t>(attribute->value)) {
                type = std::to_string(std::get<int64_t>(attribute->value));
            } else if (std::holds_alternative<Generic::float64_t>(attribute->value)) {
                type = std::to_string(std::get<Generic::float64_t>(attribute->value));
            } else if (std::holds_alternative<bool>(attribute->value)) {
                type = std::to_string(std::get<bool>(attribute->value));
            }
            break;
        }
    }

    if (type.empty()) {
        std::cerr << "(" << name << ")" << " Element " << input.type << " doesn't have a _type, assuming \"Tag\"" << std::endl;
    }

    if (Generic::iequals(type, "class")) {
        result += ".";
    } else if (Generic::iequals(type, "id")) {
        result += "#";
    } else if (!Generic::iequals(type, "tag")) {
        std::cerr << "(" << name << ")" << " Unknown _type: " << type << ", assuming \"Tag\"" << std::endl;
    }

    if (Generic::iequals(input.type, "_all")) {
        result += "*";
        if (!minify) {
            result += " ";
        }
        result += "{";
    } else {
        result += input.type + (!minify ? " " : "") + "{" + (!minify ? "\n" : "");
    }
    for (const auto &list : input.lists) {
        if (Generic::iequals(list->type, "_pseudoelements")) {
            for (const auto &element : list->elements) {
                Config::ConfigElement *newElement = element->element;
                newElement->type = input.type + "::" + newElement->type;
                afterMain += elementHelper(*newElement, name, templates, minify, indent);
            }
        }
        if (Generic::iequals(list->type, "_pseudoclasses")) {
            for (const auto &element : list->elements) {
                Config::ConfigElement *newElement = element->element;
                newElement->type = input.type + ":" + newElement->type;
                auto newAttribute = Config::ConfigElementAttribute("_type", type);
                newElement->attributes.push_back(&newAttribute);
                afterMain += elementHelper(*newElement, name, templates, minify, indent);
            }
        }
        if (Generic::iequals(list->type, "_templates")) {
            for (const auto &element : list->elements) {
                if (!Generic::iequals(element->element->type, "Template")) {
                    std::cerr << "(" << name << ")" << " Expected Template, got " << element->element->type << std::endl;
                    continue;
                }
                std::string templateName;
                for (const auto &attribute : element->element->attributes) {
                    if (Generic::iequals(attribute->name, "Name")) {
                        if (std::holds_alternative<std::string>(attribute->value)) {
                            templateName = std::get<std::string>(attribute->value);
                        } else if (std::holds_alternative<int64_t>(attribute->value)) {
                            templateName = std::to_string(std::get<int64_t>(attribute->value));
                        } else if (std::holds_alternative<Generic::float64_t>(attribute->value)) {
                            templateName = std::to_string(std::get<Generic::float64_t>(attribute->value));
                        } else if (std::holds_alternative<bool>(attribute->value)) {
                            templateName = std::to_string(std::get<bool>(attribute->value));
                        }
                    } else {
                        std::cerr << "(" << name << ")" << " Expected Name, got " << attribute->name << std::endl;
                    }
                }
                if (!templates.contains(templateName)) {
                    std::cerr << "(" << name << ")" << " Template " << element->element->attributes[0]->name << " not found" << std::endl;
                    continue;
                }
                result += templates.at(templateName);
            }
        }
    }
    result += elementInsideHelper(input, minify, indent);
    result += "}";
    if (!minify) {
        result += "\n";
    }
    result += afterMain;
    return result;
}

std::string parseCSS(const Config::ConfigRoot &input, bool minify, size_t indent) {
    std::map<std::string, std::string> templates;
    std::string result;
    for (const auto &list : input.lists) {
        if (Generic::iequals(list->type, "_templates")) {
            for (const auto &element : list->elements) {
                templates.emplace(element->element->type, elementInsideHelper(*element->element, minify, indent));
            }
            continue;
        }
        std::cerr << "(" << input.name << ")" << " Unknown list type: " << list->type << std::endl;
    }
    for (const auto &element : input.elements) {
        result += elementHelper(*element, input.name, templates, minify, indent);
    }
    return result;
}
