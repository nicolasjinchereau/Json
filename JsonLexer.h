/*---------------------------------------------------------------------------------------------
*  Copyright (c) 2019 Nicolas Jinchereau. All rights reserved.
*  Licensed under the MIT License. See License.txt in the project root for license information.
*--------------------------------------------------------------------------------------------*/

#pragma once
#include <string>
#include <unordered_map>
#include <cstdint>
#include <cfloat>
#include <memory>
#include <fstream>
#include <codecvt>
#include <cassert>
#include <vector>
#include <variant>
#include <stdexcept>
#include <iostream>

using namespace std::string_literals;

enum class JsonTokenType
{
    EndOfFile,
    ObjectStart,
    ObjectEnd,
    ArrayStart,
    ArrayEnd,
    Colon,
    Comma,
    String,
    Integer,
    Float,
    Boolean,
    Null
};

struct JsonToken
{
    union ValueType {
        int64_t intValue;
        double floatValue;
        bool boolValue;
        char charValue;
        std::string stringValue;

        ValueType(){}
        ~ValueType(){}
    };

    JsonTokenType type = JsonTokenType::EndOfFile;
    size_t pos = -1;
    ValueType storage;

    JsonToken() = default;

    JsonToken(const JsonToken& tok) : type(tok.type), pos(tok.pos)
    {
        if (type == JsonTokenType::String)
            new (&storage.stringValue) std::string(tok.storage.stringValue);
        else
            memcpy(&storage, &tok.storage, sizeof(ValueType));
    }

    JsonToken(JsonToken&& tok) : type(tok.type), pos(tok.pos)
    {
        if (type == JsonTokenType::String)
            new (&storage.stringValue) std::string(std::move(tok.storage.stringValue));
        else
            memcpy(&storage, &tok.storage, sizeof(ValueType));
    }

    JsonToken& operator=(const JsonToken& tok)
    {
        if (type == JsonTokenType::String) {
            typedef std::string stype;
            storage.stringValue.~stype();
        }

        type = tok.type;
        pos = tok.pos;

        if (type == JsonTokenType::String)
            new (&storage.stringValue) std::string(tok.storage.stringValue);
        else
            memcpy(&storage, &tok.storage, sizeof(ValueType));

        return *this;
    }

    JsonToken& operator=(JsonToken&& tok)
    {
        if (type == JsonTokenType::String) {
            typedef std::string stype;
            storage.stringValue.~stype();
        }

        type = tok.type;
        pos = tok.pos;

        if (type == JsonTokenType::String)
            new (&storage.stringValue) std::string(std::move(tok.storage.stringValue));
        else
            memcpy(&storage, &tok.storage, sizeof(ValueType));

        return *this;
    }

    JsonToken(JsonTokenType type, size_t pos, const std::string& value)
        : type(type), pos(pos)
    {
        new (&storage.stringValue) std::string(value);
    }

    JsonToken(JsonTokenType type, size_t pos, std::string&& value)
        : type(type), pos(pos)
    {
        new (&storage.stringValue) std::string(std::move(value));
    }

    JsonToken(JsonTokenType type, size_t pos, int64_t value)
        : type(type), pos(pos)
    {
        storage.intValue = value;
    }

    JsonToken(JsonTokenType type, size_t pos, double value)
        : type(type), pos(pos)
    {
        storage.floatValue = value;
    }

    JsonToken(JsonTokenType type, size_t pos, bool value)
        : type(type), pos(pos)
    {
        storage.boolValue = value;
    }

    JsonToken(JsonTokenType type, size_t pos, char value)
        : type(type), pos(pos)
    {
        storage.charValue = value;
    }

    JsonToken(JsonTokenType type, size_t pos, std::nullptr_t value)
        : type(type), pos(pos)
    {
        storage.intValue = 0;
    }

    ~JsonToken()
    {
        if (type == JsonTokenType::String) {
            typedef std::string stype;
            storage.stringValue.~stype();
        }
    }

    const std::string& typeName() const
    {
        static std::string types[] = {
            "EndOfFile",
            "ObjectStart",
            "ObjectEnd",
            "ArrayStart",
            "ArrayEnd",
            "Colon",
            "Comma",
            "String",
            "Integer",
            "Float",
            "Boolean",
            "Null"
        };

        return types[(int)type];
    }
};

class JsonLexer
{
    size_t line;
    size_t column;
    size_t offset;
    char32_t value;
    std::basic_string<int, std::char_traits<int>> chars;
    int tabLength = 4;
public:

    bool endOfFile() const {
        return offset == chars.size();
    }

    char32_t getValue() const {
        return value;
    }

    size_t getOffset() const {
        return offset;
    }

    JsonLexer(const std::string& filename)
    {
        std::ifstream fin(filename, std::ios::in | std::ios::binary);

        if (!fin.good())
            throw std::runtime_error("failed to open file: "s + filename);

        fin.seekg(0, std::ios::end);
        auto sz = (size_t)fin.tellg();
        fin.seekg(0, std::ios::beg);

        if (sz == 0)
            throw std::runtime_error("file is empty: "s + filename);

        auto buffer = std::unique_ptr<char[]>(new char[sz]);
        fin.read(buffer.get(), sz);

        std::locale loc(std::locale(), new std::codecvt_utf8<char32_t>);
        std::basic_ifstream<char32_t> fin2(filename);
        fin2.imbue(loc);
        
        fin2.seekg(0, std::ios::end);

        std::wstring_convert<std::codecvt_utf8<int>, int> cvt;
        chars = cvt.from_bytes(buffer.get(), buffer.get() + sz);
        
        line = 0;
        column = 0;
        offset = 0;
        value = chars[0];
    }

    static std::vector<JsonToken> Tokenize(const std::string& filename)
    {
        std::vector<JsonToken> tokens;
        JsonLexer lexer(filename);

        do {
            tokens.push_back(lexer.GetNextToken());
        } while (!lexer.endOfFile());

        return tokens;
    }

    JsonToken GetNextToken()
    {
        SkipWhitespace();

        auto pos = offset;

        if (offset == chars.size()) {
            return JsonToken(JsonTokenType::EndOfFile, pos, (char)EOF);
        }
        else if (value == '{') {
            SkipChar();
            return JsonToken(JsonTokenType::ObjectStart, pos, '{');
        }
        else if (value == '}') {
            SkipChar();
            return JsonToken(JsonTokenType::ObjectEnd, pos, '}');
        }
        else if (value == '[') {
            SkipChar();
            return JsonToken(JsonTokenType::ArrayStart, pos, '[');
        }
        else if (value == ']') {
            SkipChar();
            return JsonToken(JsonTokenType::ArrayEnd, pos, ']');
        }
        else if (value == ':') {
            SkipChar();
            return JsonToken(JsonTokenType::Colon, pos, ':');
        }
        else if (value == ',') {
            SkipChar();
            return JsonToken(JsonTokenType::Comma, pos, ',');
        }
        else if (value == '\"') {
            return GetStringToken();
        }
        else if ((value >= '0' && value <= '9') || value == '-' || value == '+') {
            return GetNumberToken();
        }
        else if (value == 't' || value == 'f') {
            return GetBooleanToken();
        }
        else if (value == 'n') {
            return GetNullToken();
        }
        else {
            throw std::runtime_error("found unexpected input: "s + (char)value);
        }
    }

private:
    
    void SkipWhitespace()
    {
        while (offset != chars.size())
        {
            if (value == ' ') // spaces
            {
                ++column;
            }
            else if (value == '\t')
            {
                column += tabLength;
            }
            else if (value == '\n') // new line
            {
                ++line;
                column = 0;
            }
            else if (value == '\v' || value == '\f' || value == '\r')
            {
                // ignore
            }
            else
            {
                // found non-whitespace character
                break;
            }

            SkipChar();
        }
    }

    void SkipChar() {
        assert(offset <= chars.size());
        value = chars[++offset];
    }

    void SkipChars(int count) {
        assert(offset <= chars.size() - count);
        value = chars[offset += count];
    }

    bool IsStartOfNumber(char32_t c) {
        return (c >= '0' && c <= '9') || c == '-' || c == '+';
    }

    bool IsStartOf(char32_t c) {
        return (c >= '0' && c <= '9') || c == '-' || c == '+';
    }

    JsonToken GetStringToken()
    {
        assert(value == '\"');
        
        auto start = offset;
        SkipChar();

        std::u32string str;
        
        while (offset != chars.size())
        {
            if (value == '\"')
            {
                SkipChar();

                std::wstring_convert<std::codecvt_utf8<int>, int> cvt;
                const int* p = (const int*)str.data();
                auto u8str = cvt.to_bytes(p, p + str.length());

                return JsonToken(JsonTokenType::String, start, u8str);
            }
            else if (value == '\\')
            {
                SkipChar();

                if (offset == chars.size())
                    throw std::runtime_error("unexpected end of input");

                if (value == '\"') {
                    SkipChar();
                    str += '\"';
                }
                else if (value == '\\') {
                    SkipChar();
                    str += '\\';
                }
                else if (value == '/') {
                    SkipChar();
                    str += '/';
                }
                else if (value == 'b') {
                    SkipChar();
                    str += '\b';
                }
                else if (value == 'f') {
                    SkipChar();
                    str += '\f';
                }
                else if (value == 'n') {
                    SkipChar();
                    str += '\n';
                }
                else if (value == 'r') {
                    SkipChar();
                    str += '\r';
                }
                else if (value == 't') {
                    SkipChar();
                    str += '\t';
                }
                else if (value == 'u')
                {
                    SkipChar();

                    if (offset > chars.size() - 4)
                        throw std::runtime_error("unexpected end of input");

                    char hex[4];

                    for (int i = 0; i < 4; ++i)
                    {
                        if (!isxdigit(value))
                            throw std::runtime_error("invalid unicode escape sequence");

                        hex[i] = (char)value;
                        SkipChar();
                    }
                    
                    str += (char32_t)std::strtoul(hex, nullptr, 16);
                }
                else
                {
                    str += value;
                    SkipChar();
                }
            }
            else
            {
                str += value;
                SkipChar();
            }
        }

        assert(offset == chars.size());
        throw std::runtime_error("unexpected end of input");
    }

    JsonToken GetNumberToken()
    {
        size_t numberStart = offset;
        int sign = 1;
        
        // SIGN
        if (value == '-') {
            sign = -1;
            SkipChar();
        }
        else if (value == '+') {
            SkipChar();
        }

        // MANTISSA
        size_t mantStart = offset;
        int64_t mantissa = 0;
        int64_t exponent = 0;
        bool hasDecimal = false;
        
        while (offset < chars.size())
        {
            if (isdigit(value)) {

                if (hasDecimal)
                    --exponent;

                mantissa = mantissa * 10 + (value - U'0');
                SkipChar();
            }
            else if (value == '.' && !hasDecimal) {
                hasDecimal = true;
                SkipChar();
                continue;
            }
            else {
                break;
            }
        }

        //if (offset - mantStart == 0)
        //    throw runtime_error("invalid number (no whole part)");

        if (value == 'e' || value == 'E')
        {
            SkipChar();

            int expSign = 1;

            if (value == '-') {
                expSign = -1;
                SkipChar();
            }
            else if (value == '+') {
                SkipChar();
            }

            int64_t exp = 0;

            while (offset < chars.size() && isdigit(value)) {
                exp = exp * 10 + (value - U'0');
                SkipChar();
            }

            exponent += exp * expSign;

            // exponent is required if 'e' or 'E' are present
            //if (expStart == expEnd)
            //    throw runtime_error("invalid number");
        }

        if (exponent >= 0) {
            auto wholeNumber = mantissa * (int64_t)(std::pow(10, exponent) + 0.5) * sign;
            return JsonToken(JsonTokenType::Integer, numberStart, wholeNumber);
        }

        auto fractNumber = (double)std::copysign((mantissa * std::pow(10.0L, exponent)), sign);
        return JsonToken(JsonTokenType::Float, numberStart, fractNumber);
    }

    JsonToken GetBooleanToken()
    {
        size_t pos = offset;
        auto p = &chars[offset];
        
        bool val;

        if (std::equal(p, p + 4, U"true")) {
            val = true;
            SkipChars(4);
        }
        else if (std::equal(p, p + 5, U"false") == 0) {
            val = false;
            SkipChars(5);
        }
        else {
            throw std::runtime_error("expected boolean literal");
        }

        return JsonToken(JsonTokenType::Boolean, pos, val);
    }

    JsonToken GetNullToken()
    {
        size_t pos = offset;
        auto p = &chars[offset];

        if (std::equal(p, p + 4, U"null"))
            SkipChars(4);
        else
            throw std::runtime_error("expected null literal");

        return JsonToken(JsonTokenType::Null, pos, nullptr);
    }
};
