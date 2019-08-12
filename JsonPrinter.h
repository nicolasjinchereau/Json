/*---------------------------------------------------------------------------------------------
*  Copyright (c) 2019 Nicolas Jinchereau. All rights reserved.
*  Licensed under the MIT License. See License.txt in the project root for license information.
*--------------------------------------------------------------------------------------------*/

#pragma once
#include <string>
#include <sstream>
#include "JsonParser.h"

class JsonPrinter
{
    bool pretty = true;
    int indentWidth = 4;

    std::string MakeIndent(int indent) {
        return pretty ? std::string(indentWidth * indent, ' ') : "";
    }
public:
    
    JsonPrinter(bool pretty, int indentWidth)
        : pretty(pretty), indentWidth(indentWidth)
    {

    }

    std::string ToString(const std::unique_ptr<JSONValue>& value)
    {
        std::stringstream stream;
        ToStream(stream, 0, value);
        return stream.str();
    }

    void ToStream(std::stringstream& stream, int indent, const std::unique_ptr<JSONValue>& value)
    {
        switch (value->type())
        {
            default:
            case JsonValueType::Null:
                stream << "null";
                break;

            case JsonValueType::Object:
            {
                auto obj = (JSONObject*)value.get();

                stream << "{";
                if (pretty && !obj->values.empty()) stream << "\n";

                int i = 0;
                for (auto& e : obj->values)
                {
                    if (i++) {
                        stream << ",";
                        if (pretty) stream << "\n";
                    }
                    
                    stream << MakeIndent(indent + 1) << "\"" << e.first << "\":";
                    if (pretty) stream << " ";
                    ToStream(stream, indent + 1, e.second);
                }

                if (!obj->values.empty())
                {
                    if (pretty) stream << "\n";
                    stream << MakeIndent(indent);
                }
                
                stream << "}";
                break;
            }
            case JsonValueType::Array:
            {
                stream << "[";

                auto arr = (JSONArray*)value.get();

                if (pretty && !arr->values.empty()) stream << "\n";

                int i = 0;
                for (auto& e : arr->values)
                {
                    if (i++) {
                        stream << ",";
                        if (pretty) stream << "\n";
                    }

                    stream << MakeIndent(indent + 1);
                    ToStream(stream, indent + 1, e);
                }

                if (!arr->values.empty())
                {
                    if (pretty) stream << "\n";
                    stream << MakeIndent(indent);
                }
                
                stream << "]";
                break;
            }
            case JsonValueType::String:
            {
                auto stringVal = (JSONString*)value.get();
                stream << "\"" << stringVal->value << "\"";
                break;
            }
            case JsonValueType::Integer:
            {
                auto intVal = (JSONInteger*)value.get();
                stream << intVal->value;
                break;
            }
            case JsonValueType::Float:
            {
                auto floatVal = (JSONFloat*)value.get();
                stream << floatVal->value;
                break;
            }
            case JsonValueType::Boolean:
            {
                auto boolVal = (JSONBoolean*)value.get();
                stream << boolVal->value;
                break;
            }
        }
    }
};
