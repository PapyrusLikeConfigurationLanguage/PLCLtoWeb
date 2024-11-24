// SPDX-License-Identifier: GPL-3.0-only

#include <map>
#include <memory>
#include <utility>
#include "HTML.hpp"
#include "Generic.hpp"

std::string listHelper(const Config::ConfigList& list, const std::string_view& name, const std::map<std::string, const Config::ConfigElement*>& templates, const std::map<std::string, std::shared_ptr<VariableValue>>* variables, bool minify, size_t indent, size_t indentStart);

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

// is called when listHelper encounters an element whose type is in the templates map
std::string templateHelper(const Config::ConfigElement* element, const std::map<std::string, const Config::ConfigElement*> templates, bool minify, size_t indent, size_t indentStart) {
    std::string result;
    std::map<std::string, std::shared_ptr<VariableValue>> variables;
    for (const auto &attribute : element->attributes) {
        std::string lowercaseName = attribute->name;
        std::ranges::transform(lowercaseName, lowercaseName.begin(), ::tolower);
        if (variables.contains(lowercaseName)) {
            std::cerr << "(" << element->type << ")" << " Variable " << lowercaseName << " already exists" << std::endl;
            continue;
        }
        if (std::holds_alternative<std::string>(attribute->value)) {
            variables.emplace(lowercaseName, std::make_shared<VariableValue>(std::get<std::string>(attribute->value)));
        } else if (std::holds_alternative<int64_t>(attribute->value)) {
            variables.emplace(lowercaseName, std::make_shared<VariableValue>(std::to_string(std::get<int64_t>(attribute->value))));
        } else if (std::holds_alternative<Generic::float64_t>(attribute->value)) {
            variables.emplace(lowercaseName, std::make_shared<VariableValue>(std::to_string(std::get<Generic::float64_t>(attribute->value))));
        } else if (std::holds_alternative<bool>(attribute->value)) {
            variables.emplace(lowercaseName, std::make_shared<VariableValue>(std::to_string(std::get<bool>(attribute->value))));
        }
    }
    for (const auto &list : element->lists) {
        if (Generic::iequals(list->type, "variablevalues")) {
            for (const auto &listElement : list->elements) {
                const Config::ConfigElement *child_element = listElement->element;
                if (child_element == nullptr) {
                    std::cerr << "(" << element->type << ")" << " Unexpected null element in the _VariableValues list" << std::endl;
                    continue;
                }
                if (!Generic::iequals(child_element->type, "VariableValue")) {
                    std::cerr << "(" << child_element->type << ")" << " Expected VariableValue, got " << element->type << std::endl;
                    continue;
                }
                std::string variableName;
                VariableValueType type;
                for (const auto &attribute : child_element->attributes) {
                    if (Generic::iequals(attribute->name, "Name")) {
                        variableName = attributeValueToString(attribute->value);
                    } else if (Generic::iequals(attribute->name, "Type")) {
                        if (Generic::iequals(attributeValueToString(attribute->value), "Literal")) {
                            type = LITERAL;
                        } else if (Generic::iequals(attributeValueToString(attribute->value), "LiteralArray")) {
                            type = LITERAL_ARRAY;
                        } else if (Generic::iequals(attributeValueToString(attribute->value), "Element")) {
                            type = ELEMENT;
                        } else {
                            std::cerr << "(" << child_element->type << ")" << " Expected Literal, LiteralArray or Element, got " << attributeValueToString(attribute->value) << std::endl;
                        }
                    }
                }
                if (variableName.empty()) {
                    std::cerr << "(" << child_element->type << ")" << " VariableValue elements need to have the \"Name\" attribute" << std::endl;
                    continue;
                }
                std::ranges::transform(variableName, variableName.begin(), ::tolower);
                if (variables.contains(variableName)) {
                    std::cerr << "(" << child_element->type << ")" << " Variable " << variableName << " already exists" << std::endl;
                    continue;
                }
                if (type == LITERAL) {
                    std::string value;
                    for (const auto &attribute : child_element->attributes) {
                        if (Generic::iequals(attribute->name, "Value")) {
                            value = attributeValueToString(attribute->value);
                        } else {
                            std::cerr << "(" << child_element->type << ")" << " Expected Value, got " << attribute->name << std::endl;
                        }
                    }
                    if (value.empty()) {
                        std::cerr << "(" << child_element->type << ")" << " Literal VariableValue elements need to have the \"Value\" attribute" << std::endl;
                        continue;
                    }
                    variables.emplace(variableName, std::make_shared<VariableValue>(value));
                } else if (type == LITERAL_ARRAY) {
                    std::vector<std::string> value;
                    for (const auto &list : child_element->lists) {
                        if (Generic::iequals(list->type, "Value")) {
                            if (list->elements.size() != 1) {
                                std::cerr << "(" << child_element->type << ")" << " Expected 1 element, got " << list->elements.size() << std::endl;
                                continue;
                            }
                            Config::ConfigElement *listElement = list->elements[0]->element;
                            if (Generic::iequals(listElement->type, "_LiteralList")) {
                                std::ranges::remove_if(listElement->attributes, [](const Config::ConfigElementAttribute* attribute) {
                                    std::string name = attribute->name;
                                    std::ranges::transform(name, name.begin(), ::tolower);
                                    return !name.starts_with("element");
                                });
                                for (auto &attribute : listElement->attributes) {
                                    attribute->name = attribute->name.substr(7);
                                }
                                std::ranges::sort(listElement->attributes, [](const Config::ConfigElementAttribute* a, const Config::ConfigElementAttribute* b) {
                                    return std::stoi(a->name) < std::stoi(b->name);
                                });
                                for (const auto &attribute : listElement->attributes) {
                                    std::string attribute_string = attributeValueToString(attribute->value);
                                    value.push_back(attribute_string);
                                }
                            } else {
                                std::cerr << "(" << child_element->type << ")" << " Expected _LiteralList, got " << listElement->type << std::endl;
                            }
                        } else {
                            std::cerr << "(" << child_element->type << ")" << " Expected Value, got " << list->type << std::endl;
                        }
                    }
                    if (value.empty()) {
                        std::cerr << "(" << child_element->type << ")" << " LiteralArray VariableValue elements need to have the \"Value\" attribute" << std::endl;
                        continue;
                    }
                    variables.emplace(variableName, std::make_shared<VariableValue>(value));
                } else if (type == ELEMENT) {
                    for (const auto &list : child_element->lists) {
                        if (Generic::iequals(list->type, "Value")) {
                            if (list->elements.size() != 1) {
                                std::cerr << "(" << child_element->type << ")" << " Expected 1 element, got " << list->elements.size() << std::endl;
                                continue;
                            }
                            variables.emplace(variableName, std::make_shared<VariableValue>(*list->elements[0]->element));
                        } else {
                            std::cerr << "(" << child_element->type << ")" << " Expected Value, got " << list->type << std::endl;
                        }
                    }
                    std::cerr << "(" << child_element->type << ")" << " No Value list found in VariableValue" << std::endl;
                }
            }
        }
    }
    std::string lowercaseType = element->type;
    std::ranges::transform(lowercaseType, lowercaseType.begin(), ::tolower);
    const Config::ConfigElement *templateElement = templates.at(lowercaseType);
    // have to go over the lists twice for the variables to be available
    for (const auto &list : templateElement->lists) {
        if (Generic::iequals(list->type, "variables")) {
            for (const auto &listElement : list->elements) {
                const Config::ConfigElement* child_element = listElement->element;
                if (child_element == nullptr) {
                    std::cerr << "(" << element->type << ")" << " Unexpected null element in the _Variables list" << std::endl;
                    continue;
                }
                if (!Generic::iequals(child_element->type, "Variable")) {
                    std::cerr << "(" << child_element->type << ")" << " Expected Variable, got " << element->type << std::endl;
                    continue;
                }
                std::string variableName;
                for (const auto &attribute : child_element->attributes) {
                    if (Generic::iequals(attribute->name, "Name")) {
                        variableName = attributeValueToString(attribute->value);
                    } else if (Generic::iequals(attribute->name, "Default")) {
                        if (!variables.contains(variableName)) {
                            std::ranges::transform(variableName, variableName.begin(), ::tolower);
                            std::string value = attributeValueToString(attribute->value);
                            variables.emplace(variableName, std::make_shared<VariableValue>(value));
                        }
                    } else {
                        std::cerr << "(" << child_element->type << ")" << " Expected Name, got " << attribute->name << std::endl;
                    }
                }
            }
        }
    }
    for (const auto &list : templateElement->lists) {
        if (Generic::iequals(list->type, "elements")) {
            result += listHelper(*list, element->type, templates, &variables, minify, indent, indentStart);
        }
    }

    return result;
}

std::string listHelper(const Config::ConfigList& list, const std::string_view& name, const std::map<std::string, const Config::ConfigElement*>& templates, const std::map<std::string, std::shared_ptr<VariableValue>>* variables, bool minify, size_t indent, size_t indentStart) {
    std::string result;
    for (const auto &element : list.elements) {
        if (element->element == nullptr) {
            std::cerr << "(" << name << ")" << " ConfigListElement doesn't contain a ConfigElement" << std::endl;
            continue;
        }
        Config::ConfigElement *child_element = element->element;
        if (Generic::iequals(child_element->type, "_text")) {
            if (!minify) {
                result += "\n";
                result += std::string(indentStart, ' ');
            }
            if (child_element->attributes.empty() && child_element->lists.empty()) {
                std::cerr << "(" << name << ")" << " _Text pseudo-elements should have the \"Content\" attribute or the \"Bindings\" list"
                          << std::endl;
            }
            if (child_element->attributes.size() > 1) {
                std::cerr << "(" << name << ")" << " _Text pseudo-elements shouldn't have more than 1 attribute"
                          << std::endl;
            }
            for (const auto &lists : child_element->lists) {
                if (Generic::iequals(lists->type, "_Bindings")) {
                    if (variables == nullptr) {
                        std::cerr << "(" << name << ")" << " Bindings cannot be used outside of a template" << std::endl;
                        continue;
                    }
                    for (const auto &listElement : lists->elements) {
                        Config::ConfigElement *element = listElement->element;
                        if (element == nullptr) {
                            std::cerr << "(" << name << ")" << " Unexpected null element in Bindings" << std::endl;
                            continue;
                        }
                        if (!Generic::iequals(element->type, "Binding")) {
                            std::cerr << "(" << name << ")" << " Expected Binding, got " << element->type << std::endl;
                            continue;
                        }
                        std::string source;
                        std::string target;
                        for (const auto &attribute : element->attributes) {
                            if (Generic::iequals(attribute->name, "Source")) {
                                source = attributeValueToString(attribute->value);
                            } else if (Generic::iequals(attribute->name, "Target")) {
                                target = attributeValueToString(attribute->value);
                            } else {
                                std::cerr << "(" << name << ")" << " Expected Source or Target, got " << attribute->name << std::endl;
                            }
                        }
                        if (source.empty() || target.empty()) {
                            std::cerr << "(" << name << ")" << " Binding elements need to have the \"Source\" and \"Target\" attributes" << std::endl;
                            continue;
                        }
                        if (!Generic::iequals(target, "content")) {
                            std::cerr << "(" << name << ")" << " Binding elements for the \"_Text\" element should have the \"Target\" attribute set to \"Content\"" << std::endl;
                            continue;
                        }
                        std::ranges::transform(source, source.begin(), ::tolower);
                        if (variables->contains(source)) {
                            VariableValue* value = variables->at(source).get();
                            if (value == nullptr) {
                                std::cerr << "(" << name << ")" << " Unexpected null value" << std::endl;
                                continue;
                            }
                            if (std::holds_alternative<std::string>(*value)) {
                                result += std::get<std::string>(*value);
                            } else {
                                std::cerr << "(" << name << ")" << " Binding variable " << source << " is not a Literal" << std::endl;
                            }
                        } else {
                            std::cerr << "(" << name << ")" << " Binding variable " << source << " not found in variables" << std::endl;
                        }
                    }
                }
            }
            for (const auto &attribute: child_element->attributes) {
                if (Generic::iequals(attribute->name, "Content")) {
                    result += attributeValueToString(attribute->value);
                }
            }
        } else if (Generic::iequals(child_element->type, "_BindingLoop")) {
            if (variables == nullptr) {
                std::cerr << "(" << name << ")" << " _BindingLoops cannot be used outside of a template" << std::endl;
                continue;
            }
            std::string source;
            for (const auto &attribute : child_element->attributes) {
                if (Generic::iequals(attribute->name, "Source")) {
                    source = attributeValueToString(attribute->value);
                } else {
                    std::cerr << "(" << name << ")" << " Expected Source, got " << attribute->name << std::endl;
                }
            }
            if (source.empty()) {
                std::cerr << "(" << name << ")" << " _BindingLoop elements need to have the \"Source\" attribute" << std::endl;
                continue;
            }
            std::ranges::transform(source, source.begin(), ::tolower);
            if (!variables->contains(source)) {
                std::cerr << "(" << name << ")" << " Binding variable " << source << " not found in variables" << std::endl;
                continue;
            }
            VariableValue* value = variables->at(source).get();
            if (value == nullptr) {
                std::cerr << "(" << name << ")" << " Unexpected null value" << std::endl;
                continue;
            }
            if (!std::holds_alternative<LiteralArray>(*value)) {
                std::cerr << "(" << name << ")" << " Binding variable " << source << " is not a LiteralArray" << std::endl;
                continue;
            }
            LiteralArray arr = std::get<LiteralArray>(*value);
            std::map<std::string, std::shared_ptr<VariableValue>> newVariables = *variables;
            for (const auto& literal : arr) {
                std::erase_if(newVariables, [source](const std::pair<std::string, std::shared_ptr<VariableValue>>& element) {
                    return element.first == source;
                });
                newVariables.emplace(source, std::make_shared<VariableValue>(literal));
                for (const auto &list : child_element->lists) {
                    if (Generic::iequals(list->type, "elements")) {
                        result += listHelper(*list, name, templates, &newVariables, minify, indent, indentStart);
                    }
                }
            }
        } else {
            for (const auto &attribute : child_element->attributes) { // FIXME: what was this even for
            }
            std::string lowercaseType = child_element->type;
            std::ranges::transform(lowercaseType, lowercaseType.begin(), ::tolower);
            if (!minify && !templates.contains(lowercaseType)) {
                result += "\n";
                result += std::string(indentStart, ' ');
            }
            if (templates.contains(lowercaseType)) {
                result += templateHelper(child_element, templates, minify, indent, indentStart);
                continue;
            }
            for (const auto &innerList : child_element->lists) {
                if (Generic::iequals(innerList->type, "_Bindings")) {
                    if (variables == nullptr) {
                        std::cerr << "(" << name << ")" << " Bindings cannot be used outside of a template" << std::endl;
                        continue;
                    }
                    for (const auto &listElement : innerList->elements) {
                        if (!Generic::iequals(listElement->element->type, "Binding")) {
                            std::cerr << "(" << name << ")" << " Expected Binding, got " << listElement->element->type << std::endl;
                            continue;
                        }
                        std::string source;
                        std::string target;
                        for (const auto &attribute : listElement->element->attributes) {
                            if (Generic::iequals(attribute->name, "Source")) {
                                source = attributeValueToString(attribute->value);
                            } else if (Generic::iequals(attribute->name, "Target")) {
                                target = attributeValueToString(attribute->value);
                            } else {
                                std::cerr << "(" << name << ")" << " Expected Source or Target, got " << attribute->name << std::endl;
                            }
                        }
                        if (source.empty() || target.empty()) {
                            std::cerr << "(" << name << ")" << " Binding elements need to have the \"Source\" and \"Target\" attributes" << std::endl;
                            continue;
                        }
                        std::ranges::transform(source, source.begin(), ::tolower);
                        if (variables->contains(source)) {
                            VariableValue *value = variables->at(source).get();
                            if (value == nullptr) {
                                std::cerr << "(" << name << ")" << " Unexpected null value" << std::endl;
                                continue;
                            }
                            std::string string_value;
                            if (std::holds_alternative<std::string>(*value)) {
                                string_value = std::get<std::string>(*value);
                            } else if (std::holds_alternative<LiteralArray>(*value)) {
                                std::cerr << "(" << name << ")" << " _BindingLoop not used for a LiteralArray. Getting the first element" << std::endl;
                                string_value = std::get<LiteralArray>(*value).at(0);
                            } else if (std::holds_alternative<Config::ConfigElement>(*value)) {
                                std::cerr << "FIXME: ELEMENT" << std::endl;
                                string_value = "FIXME: ELEMENT";
                                //string_value = listHelper(*value->element.lists[0], name, templates, variables, minify, indent, indentStart);
                            }
                            bool found = false;
                            for (const auto &attribute : child_element->attributes) {
                                if (Generic::iequals(attribute->name, target)) {
                                    attribute->value = string_value;
                                    found = true;
                                    break;
                                }
                            }
                            if (!found) {
                                Generic::ValueType value = string_value;
                                auto* newAttribute = new Config::ConfigElementAttribute(target, value);
                                child_element->attributes.push_back(newAttribute);
                            }
                        } else {
                            std::cerr << "(" << name << ")" << " Binding variable " << source << " not found in variables" << std::endl;
                        }
                    }
                }
            }
            result += "<" + lowercaseType;
            if (!child_element->attributes.empty())
                result += attributeHelper(child_element->attributes);
            result += ">";
            bool hasChildren = false;
            if (!child_element->lists.empty()) {
                for (const auto &innerList : child_element->lists) {
                    if (!Generic::iequals(innerList->type, "_Bindings")) {
                        hasChildren = true;
                        result += listHelper(*innerList, name, templates, variables, minify, indent,
                                             indentStart + indent);
                    }
                }
            }
            if (std::ranges::find(VOID_ELEMENTS, lowercaseType) == std::end(VOID_ELEMENTS)) {
                if (!minify && hasChildren) {
                    result += '\n';
                    result += std::string(indentStart, ' ');
                }
                result += "</" + lowercaseType + ">";
            }
        }
    }
    return result;
}

std::string parseHTML(const Config::ConfigRoot &input, bool minify, size_t indent) {
    std::map<std::string, const Config::ConfigElement*> templates;
    if (input.elements.empty()) {
        std::cerr << "(" << input.name << ")" << " No elements found in the root of an P(L)CLHTML file" << std::endl;
        return "";
    }
    if (input.elements.size() > 2) {
        std::cerr << "(" << input.name << ")" << " A P(L)CLHTML file should contain only 1 or 2 elements in its root" << std::endl;
    }

    for (const auto &list: input.lists) {
        if (Generic::iequals(list->type, "_templates")) {
            for (const auto &listElement : list->elements) {
                const Config::ConfigElement* element = listElement->element;
                if (element == nullptr) {
                    std::cerr << "(" << input.name << ")" << " Unexpected null element in the _Templates list" << std::endl;
                    continue;
                }
                if (!Generic::iequals(element->type, "Template")) {
                    std::cerr << "(" << input.name << ")" << " Expected Template, got " << element->type << std::endl;
                    continue;
                }
                std::string templateName;
                for (const auto &attribute : element->attributes) {
                    if (Generic::iequals(attribute->name, "Name")) {
                        templateName = attributeValueToString(attribute->value);
                    } else {
                        std::cerr << "(" << input.name << ")" << " Expected Name, got " << attribute->name << std::endl;
                    }
                }
                if (templateName.empty()) {
                    std::cerr << "(" << input.name << ")" << " Template elements need to have the \"Name\" attribute. Ignoring template nr. " << listElement->id << std::endl;
                    continue;
                }
                std::ranges::transform(templateName, templateName.begin(), ::tolower);
                if (templates.contains(templateName)) {
                    std::cerr << "(" << input.name << ")" << " Template " << templateName << " already exists. Ignoring template nr. " << listElement->id << std::endl;
                    continue;
                }
                templates.emplace(templateName, element);
            }
        } else {
            std::cerr << "(" << input.name << ")" << " Unexpected list: " << list->type << std::endl;
        }
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
                    result += listHelper(*list, input.name, templates, nullptr, minify, indent, indent);
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