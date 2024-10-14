// SPDX-License-Identifier: GPL-3.0-only

#include "HTML.hpp"

std::string attributeHelper(const std::vector<Config::ConfigElementAttribute*> &attributes) {
    std::string result;
    for (const auto &attribute : attributes) {
        // ExampleAttributeName -> example-attribute-name
        std::string newName = attribute->name;
        for (size_t i = 0; i < newName.size(); i++) {
            if (isupper(newName[i]) && i != 0) {
                newName.insert(i, "-");
                i++;
            }
        }
        std::ranges::transform(newName, newName.begin(), ::tolower);

        if (std::holds_alternative<std::string>(attribute->value)) {
            result += " " + newName + "=\"" + std::get<std::string>(attribute->value) + "\"";
        } else if (std::holds_alternative<int64_t>(attribute->value)) {
            result += " " + newName + "=" + std::to_string(std::get<int64_t>(attribute->value));
        } else if (std::holds_alternative<Generic::float64_t>(attribute->value)) {
            result += " " + newName + "=" + std::to_string(std::get<Generic::float64_t>(attribute->value));
        } else if (std::holds_alternative<bool>(attribute->value) && std::get<bool>(attribute->value)) {
            result += " " + newName;
        }
    }
    return result;
}

std::string listHelper(const Config::ConfigList& list, const std::string_view& name, bool minify, size_t indent, size_t indentStart) {
    std::string result;
    for (auto &element : list.elements) {
        if (Generic::iequals(element->element->type, "_text")) {
            if (!minify) {
                result += "\n";
                result += std::string(indentStart, ' ');
            }
            if (element->element->attributes.empty()) {
                std::cerr << "(" << name << ")" << " _Text pseudo-elements should have the \"Content\" attribute" << std::endl;
            }
            if (element->element->attributes.size() > 1) {
                std::cerr << "(" << name << ")" << " _Text pseudo-elements shouldn't have more than 1 attribute" << std::endl;
            }
            for (const auto &attribute : element->element->attributes) {
                if (Generic::iequals(attribute->name, "Content")) {
                    if (std::holds_alternative<std::string>(attribute->value)) {
                        result += std::get<std::string>(attribute->value);
                    } else if (std::holds_alternative<int64_t>(attribute->value)) {
                        result += std::to_string(std::get<int64_t>(attribute->value));
                    } else if (std::holds_alternative<Generic::float64_t>(attribute->value)) {
                        result += std::to_string(std::get<Generic::float64_t>(attribute->value));
                    } else if (std::holds_alternative<bool>(attribute->value)) {
                        result += std::to_string(std::get<bool>(attribute->value));
                    }
                }
            }
        } else {
            std::string lowercaseType = element->element->type;
            std::ranges::transform(lowercaseType, lowercaseType.begin(), ::tolower);
            if (!minify) {
                result += "\n";
                result += std::string(indentStart, ' ');
            }
            result += "<" + lowercaseType;
            if (!element->element->attributes.empty())
                result += attributeHelper(element->element->attributes);
            result += ">";
            if (!element->element->lists.empty()) {
                for (const auto &innerList : element->element->lists) {
                    result += listHelper(*innerList, name, minify, indent, indentStart + indent);
                }
            }
            if (std::ranges::find(VOID_ELEMENTS, lowercaseType) == std::end(VOID_ELEMENTS)) {
                if (!minify) {
                    result += "\n";
                    result += std::string(indentStart, ' ');
                }
                result += "</" + lowercaseType + ">";
            }
        }
    }
    return result;
}

std::string parseHTML(const Config::ConfigRoot &input, bool minify, size_t indent) {
    if (!input.lists.empty()) {
        std::cerr << "(" << input.name << ")" << " Lists shouldn't be present in the root of an P(L)CLHTML file" << std::endl;
    }
    if (input.elements.empty()) {
        std::cerr << "(" << input.name << ")" << " No elements found in the root of an P(L)CLHTML file" << std::endl;
        return "";
    }
    if (input.elements.size() > 2) {
        std::cerr << "(" << input.name << ")" << " A P(L)CLHTML file should contain only 1 or 2 elements in its root" << std::endl;
    }

    std::string result;

    for (auto &element : input.elements) {
        if (Generic::iequals(element->type, "doctype")) {
            if (element->attributes.empty()) {
                std::cerr << "(" << input.name << ")" << " Doctype elements should have the \"Content\" attribute" << std::endl;
            }
            if (element->attributes.size() > 1) {
                std::cerr << "(" << input.name << ")" << " Doctype elements shouldn't have more than 1 attribute" << std::endl;
            }
            for (const auto &attribute : element->attributes) {
                if (Generic::iequals(attribute->name, "Content")) {
                    if (std::holds_alternative<std::string>(attribute->value)) {
                        result += "<!DOCTYPE " + std::get<std::string>(attribute->value) + ">";
                        if (!minify) {
                            result += "\n";
                        }
                    } else {
                        std::cerr << "(" << input.name << ")" << " Doctype elements should have a string value" << std::endl;
                    }
                }
            }
            continue;
        }
        if (Generic::iequals(element->type, "html")) {
            result += "<html";
            if (!element->attributes.empty())
                result += attributeHelper(element->attributes);
            result += ">";
            if (element->lists.empty()) {
                std::cerr << "(" << input.name << ")" << " The HTML element should contain the \"Elements\" list" << std::endl;
            }
            if (element->lists.size() > 1) {
                std::cerr << "(" << input.name << ")" << " The HTML element should contain only 1 list" << std::endl;
            }
            for (const auto &list : element->lists) {
                if (Generic::iequals(list->type, "elements")) {
                    result += listHelper(*list, input.name, minify, indent, indent);
                    if (!minify) {
                        result += "\n";
                    }
                } else {
                    std::cerr << "(" << input.name << ")" << " Unexpected list: " << list->type << std::endl;
                }
            }
            result += "</html>";
            continue;
        }
        std::cerr << "(" << input.name << ")" << " Unexpected element: " << element->type << std::endl;
    }
    return result;
}
