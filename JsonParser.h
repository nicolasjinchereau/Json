/*---------------------------------------------------------------------------------------------
*  Copyright (c) 2019 Nicolas Jinchereau. All rights reserved.
*  Licensed under the MIT License. See License.txt in the project root for license information.
*--------------------------------------------------------------------------------------------*/

#pragma once
#include "JsonLexer.h"
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <utility>
#include <cassert>
#include <unordered_map>

enum class JsonValueType
{
    Null,
    Object,
    Array,
    String,
    Integer,
    Float,
    Boolean
};

class JSONValue
{
public:
    JSONValue() {}
    virtual ~JSONValue(){}
    virtual JsonValueType type() { return JsonValueType::Null; }
};

class JSONObject : public JSONValue
{
public:
    std::unordered_map<std::string, std::unique_ptr<JSONValue>> values;
    virtual JsonValueType type() override { return JsonValueType::Object; }

    template<typename U>
    JSONObject(U&& values) : values(std::forward<U>(values)) {}
};

class JSONArray : public JSONValue
{
public:
    std::vector<std::unique_ptr<JSONValue>> values;
    virtual JsonValueType type() override { return JsonValueType::Array; }

    template<typename U>
    JSONArray(U&& values) : values(std::forward<U>(values)) {}
};

class JSONString : public JSONValue
{
public:
    std::string value;
    virtual JsonValueType type() override { return JsonValueType::String; }

    template<typename U>
    JSONString(U&& value) : value(std::forward<U>(value)) {}
};

class JSONInteger : public JSONValue
{
public:
    int64_t value;
    virtual JsonValueType type() override { return JsonValueType::Integer; }

    template<typename U>
    JSONInteger(U&& value) : value(std::forward<U>(value)) {}
};

class JSONFloat : public JSONValue
{
public:
    double value;
    virtual JsonValueType type() override { return JsonValueType::Float; }
    
    template<typename U>
    JSONFloat(U&& value) : value(std::forward<U>(value)) {}
};

class JSONBoolean : public JSONValue
{
public:
    bool value;
    virtual JsonValueType type() override { return JsonValueType::Boolean; }
    
    template<typename U>
    JSONBoolean(U&& value) : value(std::forward<U>(value)) {}
};

class JsonParser
{
    JsonLexer lexer;
    JsonToken token;
public:
    JsonParser(const std::string& filename)
        : lexer(filename)
    {
        
    }

    std::unique_ptr<JSONValue> Parse()
    {
        if (!NextToken())
            throw std::runtime_error("input is empty");
        
        return ParseValue();
    }

private:

    bool NextToken(bool thrownOnEOF = true) {
        token = std::move(lexer.GetNextToken());
        return token.type != JsonTokenType::EndOfFile;
    }

    std::unique_ptr<JSONValue> ParseValue()
    {
        switch (token.type)
        {
        case JsonTokenType::ObjectStart:
            return std::unique_ptr<JSONValue>(ParseObject().release());
        case JsonTokenType::ArrayStart:
            return std::unique_ptr<JSONValue>(ParseArray().release());
        case JsonTokenType::String:
            return std::unique_ptr<JSONValue>(ParseString().release());
        case JsonTokenType::Integer:
            return std::unique_ptr<JSONValue>(ParseInteger().release());
        case JsonTokenType::Float:
            return std::unique_ptr<JSONValue>(ParseFloat().release());
        case JsonTokenType::Boolean:
            return std::unique_ptr<JSONValue>(ParseBoolean().release());
        case JsonTokenType::Null:
            return std::unique_ptr<JSONValue>(ParseNull().release());
        case JsonTokenType::EndOfFile:
            throw std::runtime_error("unexpected end of input");
        default:
            throw std::runtime_error("unexpected token");
        }
    }

    std::unique_ptr<JSONString> ParseString()
    {
        assert(token.type == JsonTokenType::String);
        auto ret = std::unique_ptr<JSONString>(new JSONString(token.storage.stringValue));
        NextToken(false);
        return ret;
    }

    std::unique_ptr<JSONInteger> ParseInteger()
    {
        assert(token.type == JsonTokenType::Integer);
        auto ret = std::unique_ptr<JSONInteger>(new JSONInteger(token.storage.intValue));
        NextToken(false);
        return ret;
    }

    std::unique_ptr<JSONFloat> ParseFloat()
    {
        assert(token.type == JsonTokenType::Float);
        auto ret = std::unique_ptr<JSONFloat>(new JSONFloat(token.storage.floatValue));
        NextToken(false);
        return ret;
    }

    std::unique_ptr<JSONBoolean> ParseBoolean()
    {
        assert(token.type == JsonTokenType::Boolean);
        auto ret = std::unique_ptr<JSONBoolean>(new JSONBoolean(token.storage.boolValue));
        NextToken(false);
        return ret;
    }

    std::unique_ptr<JSONValue> ParseNull()
    {
        assert(token.type == JsonTokenType::Null);
        auto ret = std::unique_ptr<JSONValue>(new JSONValue());
        NextToken(false);
        return ret;
    }

    std::unique_ptr<JSONObject> ParseObject()
    {
        assert(token.type == JsonTokenType::ObjectStart);
        NextToken();

        std::unordered_map<std::string, std::unique_ptr<JSONValue>> values;

        while (token.type != JsonTokenType::ObjectEnd)
        {
            if (token.type != JsonTokenType::String)
                throw std::runtime_error("expected string");

            std::unique_ptr<JSONString> key = ParseString();
            
            if (token.type != JsonTokenType::Colon)
                throw std::runtime_error("expected colon");

            NextToken();

            values[key->value] = ParseValue();

            if (token.type == JsonTokenType::Comma)
            {
                NextToken();

                if (token.type == JsonTokenType::ObjectEnd)
                    throw std::runtime_error("expected a value");
            }
            else
            {
                if (token.type != JsonTokenType::ObjectEnd)
                    throw std::runtime_error("expected '}'");
            }
        }

        NextToken();

        return std::unique_ptr<JSONObject>(new JSONObject(std::move(values)));
    }

    std::unique_ptr<JSONArray> ParseArray()
    {
        assert(token.type == JsonTokenType::ArrayStart);
        NextToken();

        std::vector<std::unique_ptr<JSONValue>> values;

        while (token.type != JsonTokenType::ArrayEnd)
        {
            values.push_back(ParseValue());

            if (token.type == JsonTokenType::Comma)
            {
                NextToken();

                if (token.type == JsonTokenType::ArrayEnd)
                    throw std::runtime_error("expected a value");
            }
            else
            {
                if (token.type != JsonTokenType::ArrayEnd)
                    throw std::runtime_error("expected ']'");
            }
        }

        NextToken();

        return std::unique_ptr<JSONArray>(new JSONArray(std::move(values)));
    }
};
