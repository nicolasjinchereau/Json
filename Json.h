/*--------------------------------------------------------------*
*  Copyright (c) 2022 Nicolas Jinchereau. All rights reserved.  *
*--------------------------------------------------------------*/

#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cfloat>
#include <charconv>
#include <cstdint>
#include <cstddef>
#include <forward_list>
#include <iostream>
#include <initializer_list>
#include <list>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <sstream>
#include <type_traits>
#include <unordered_map>
#include <utf8.h>
#include <utility>
#include <vector>
#include <variant>

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
    typedef std::variant<std::nullptr_t, int64_t, double, bool, char, std::string> DataStorageType;

    DataStorageType data;
    JsonTokenType type = JsonTokenType::EndOfFile;
    std::string::iterator pos;

    JsonToken(){}

    JsonToken(JsonTokenType type, std::string::iterator pos, const std::string& value)
        : data(value), type(type), pos(pos){}

    JsonToken(JsonTokenType type, std::string::iterator pos, std::string&& value)
        : data(std::move(value)), type(type), pos(pos){}

    JsonToken(JsonTokenType type, std::string::iterator pos, int64_t value)
        : data(value), type(type), pos(pos){}

    JsonToken(JsonTokenType type, std::string::iterator pos, double value)
        : data(value), type(type), pos(pos){}

    JsonToken(JsonTokenType type, std::string::iterator pos, bool value)
        : data(value), type(type), pos(pos){}

    JsonToken(JsonTokenType type, std::string::iterator pos, char value)
        : data(value), type(type), pos(pos){}

    JsonToken(JsonTokenType type, std::string::iterator pos, std::nullptr_t value)
        : data(value), type(type), pos(pos){}

    int64_t GetInteger() const {
        return std::get<int64_t>(data);
    }

    double GetFloat() const {
        return std::get<double>(data);
    }

    bool GetBoolean() const {
        return std::get<bool>(data);
    }

    char GetChar() const {
        return std::get<char>(data);
    }

    const std::string& GetString() const {
        return std::get<std::string>(data);
    }

    std::string_view GetTypeName() const
    {
        static std::unordered_map<JsonTokenType, std::string_view> typeNames {
            { JsonTokenType::EndOfFile, "EndOfFile" },
            { JsonTokenType::ObjectStart, "ObjectStart" },
            { JsonTokenType::ObjectEnd, "ObjectEnd" },
            { JsonTokenType::ArrayStart, "ArrayStart" },
            { JsonTokenType::ArrayEnd, "ArrayEnd" },
            { JsonTokenType::Colon, "Colon" },
            { JsonTokenType::Comma, "Comma" },
            { JsonTokenType::String, "String" },
            { JsonTokenType::Integer, "Integer" },
            { JsonTokenType::Float, "Float" },
            { JsonTokenType::Boolean, "Boolean" },
            { JsonTokenType::Null, "Null" }
        };

        return typeNames[type];
    }
};

class JsonLexer
{
    size_t line{};
    size_t column{};
    std::string::iterator pos;
    std::string::iterator next;
    char32_t value{};
    std::string chars;
    int tabLength = 4;
public:

    bool IsEndOfFile() const {
        return pos == chars.end();
    }

    char32_t GetValue() const {
        return value;
    }

    size_t GetOffset() const {
        return pos - chars.begin();
    }

    JsonLexer(const std::string& text)
    {
        chars = text;

        pos = chars.begin();
        next = pos;
        value = utf8::next(next, chars.end());
        line = 0;
        column = 0;
    }

    static std::vector<JsonToken> Tokenize(const std::string& text)
    {
        std::vector<JsonToken> tokens;
        JsonLexer lexer(text);

        do {
            tokens.push_back(lexer.GetNextToken());
        } while (!lexer.IsEndOfFile());

        return tokens;
    }

    JsonToken GetNextToken()
    {
        SkipWhitespace();

        if (pos == chars.end()) {
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
            throw std::runtime_error(std::string("found unexpected input: ") + (char)value);
        }
    }

private:

    void SkipWhitespace()
    {
        while (pos != chars.end())
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

    void SkipChar()
    {
        assert(pos != next);
        pos = next;
        value = (next != chars.end()) ?
            utf8::next(next, chars.end()) : 0;
    }

    void SkipChars(ptrdiff_t count)
    {
        assert(pos != next);
        utf8::advance(pos, count, chars.end());
        next = pos;
        value = (next != chars.end()) ?
            utf8::next(next, chars.end()) : 0;
    }

    bool IsStartOfNumber(char32_t c) {
        return (c >= '0' && c <= '9') || c == '-' || c == '+';
    }

    bool IsStartOf(char32_t c) {
        return (c >= '0' && c <= '9') || c == '-' || c == '+';
    }

    JsonToken GetStringToken()
    {
        assert(value == U'\"');

        auto start = pos;
        SkipChar();

        std::string str;

        while (pos != chars.end())
        {
            if (value == U'\"')
            {
                SkipChar();
                return JsonToken(JsonTokenType::String, start, str);
            }
            else if (value == U'\\')
            {
                SkipChar();

                if (pos == chars.end())
                    throw std::runtime_error("unexpected end of input");

                if (value == U'\"') {
                    SkipChar();
                    utf8::append(U'\"', std::back_inserter(str));
                }
                else if (value == U'\\') {
                    SkipChar();
                    utf8::append(U'\\', std::back_inserter(str));
                }
                else if (value == U'r') {
                    SkipChar();
                    utf8::append(U'\r', std::back_inserter(str));
                }
                else if (value == U'n') {
                    SkipChar();
                    utf8::append(U'\n', std::back_inserter(str));
                }
                else if (value == U't') {
                    SkipChar();
                    utf8::append(U'\t', std::back_inserter(str));
                }
                else if (value == U'b') {
                    SkipChar();
                    utf8::append(U'\b', std::back_inserter(str));
                }
                else if (value == U'f') {
                    SkipChar();
                    utf8::append(U'\f', std::back_inserter(str));
                }
                else if (value == U'u')
                {
                    SkipChar();

                    if ((chars.end() - pos) < 4)
                        throw std::runtime_error("unexpected end of input");

                    char hex[4];

                    for (int i = 0; i < 4; ++i)
                    {
                        if (!isxdigit(value))
                            throw std::runtime_error("invalid unicode escape sequence");

                        hex[i] = (char)value;
                        SkipChar();
                    }

                    char32_t charValue = (char32_t)std::strtoul(hex, nullptr, 16);
                    utf8::append(charValue, std::back_inserter(str));
                }
                else
                {
                    utf8::append(value, std::back_inserter(str));
                    SkipChar();
                }
            }
            else
            {
                utf8::append(value, std::back_inserter(str));
                SkipChar();
            }
        }

        assert(pos == chars.end());
        throw std::runtime_error("unexpected end of input");
    }

    JsonToken GetNumberToken()
    {
        auto start = pos;
        auto beg = chars.data() + (pos - chars.begin());
        auto end = chars.data() + chars.size();

        double value;
        std::from_chars_result ret = std::from_chars(beg, end, value);
        if(ret.ec != std::errc())
            throw std::runtime_error("invalid number");

        auto len = ret.ptr - &start[0];
        std::string_view number(beg, beg + len);

        SkipChars(len);

        if(number.find('.') != std::string_view::npos)
        {
            return JsonToken(JsonTokenType::Float, start, value);
        }
        else
        {
            int64_t intValue;
            ret = std::from_chars(beg, end, intValue);
            if(ret.ec != std::errc())
                throw std::runtime_error("invalid number");

            return JsonToken(JsonTokenType::Integer, start, intValue);
        }
    }

    JsonToken GetBooleanToken()
    {
        auto start = pos;
        auto p = &*pos;

        bool val;

        if (std::equal(p, p + 4, "true")) {
            val = true;
            SkipChars(4);
        }
        else if (std::equal(p, p + 5, "false")) {
            val = false;
            SkipChars(5);
        }
        else {
            throw std::runtime_error("expected boolean literal");
        }

        return JsonToken(JsonTokenType::Boolean, start, val);
    }

    JsonToken GetNullToken()
    {
        auto start = pos;
        auto p = &*pos;

        if (std::equal(p, p + 4, "null"))
            SkipChars(4);
        else
            throw std::runtime_error("expected null literal");

        return JsonToken(JsonTokenType::Null, start, nullptr);
    }
};

class Json;

class JsonParser
{
    JsonLexer lexer;
    JsonToken token;
public:
    JsonParser(const std::string& text);
    Json Parse();

private:
    bool NextToken(bool thrownOnEOF = true);
    Json ParseValue();
    Json ParseString();
    Json ParseInteger();
    Json ParseFloat();
    Json ParseBoolean();
    Json ParseNull();
    Json ParseObject();
    Json ParseArray();
};

class JsonPrinter
{
    int indentWidth;
    bool pretty;

    void Indent(std::ostream& stream, int indent);
    void WriteEscaped(std::ostream& stream, const std::string& val);
public:

    JsonPrinter(int indentWidth);

    std::string ToString(const Json& val);
    void ToStream(std::ostream& stream, int indent, const Json& val);
};

template <typename T>
class has_to_string
{
    template <typename C, typename = decltype(to_string(std::declval<std::string&>(), std::declval<const T>()))>
    static std::true_type test(int);

    template <typename C>
    static std::false_type test(...);

public:
    static constexpr bool value = decltype(test<T>(0))::value;
};

template <typename T>
class has_from_string
{
    template <typename C, typename = decltype(from_string(std::declval<const std::string>(), std::declval<T&>()))>
    static std::true_type test(int);

    template <typename C>
    static std::false_type test(...);

public:
    static constexpr bool value = decltype(test<T>(0))::value;
};

template<class T>
inline constexpr bool has_from_string_v = has_from_string<T>::value;

template<class T>
inline constexpr bool has_to_string_v = has_to_string<T>::value;

template<class>
inline constexpr bool is_initializer_list = false;

template<class T>
inline constexpr bool is_initializer_list<std::initializer_list<T>> = true;

enum class JsonDataType
{
    Null,
    Object,
    Array,
    String,
    Integer,
    Float,
    Boolean
};

class Json
{
public:
    using NullType = std::nullptr_t;
    using ObjectType = std::unordered_map<std::string, Json>;
    using ArrayType = std::vector<Json>;
    using StringType = std::string;
    using IntegerType = int64_t;
    using FloatType = double;
    using BooleanType = bool;

    friend class JsonParser;

private:
    typedef std::variant<NullType, ObjectType, ArrayType, StringType, IntegerType, FloatType, BooleanType> DataStorageType;
    DataStorageType data;

public:
    Json(nullptr_t = nullptr)
        : data(nullptr) {}

    Json(const ObjectType& obj)
        : data(obj) {}

    Json(ObjectType&& obj)
        : data(std::move(obj)) {}

    Json(const ArrayType& arr)
        : data(arr) {}

    Json(ArrayType&& arr)
        : data(std::move(arr)) {}

    Json(const StringType& str)
        : data(str) {}

    Json(StringType&& str)
        : data(std::move(str)) {}

    template<class T> requires (std::is_integral_v<T> && !std::is_same_v<T, bool>)
    Json(const T& integer)
        : data((IntegerType)integer) {}

    template<class T> requires std::is_floating_point_v<T>
    Json(T floating)
        : data((FloatType)floating) {}

    template<class T> requires std::is_same_v<T, BooleanType>
    Json(T boolean)
        : data(boolean) {}

    template<class T, class S = std::decay_t<T>> requires (
        !std::is_same_v<S, Json> &&
        !std::is_integral_v<S> &&
        !std::is_same_v<S, BooleanType> &&
        !std::is_floating_point_v<S>)
    Json(T&& val) : Json() {
        to_json(*this, std::forward<T>(val));
    }

    static Json Object() {
        Json ret;
        ret.data.emplace<ObjectType>();
        return ret;
    }

    static Json Array() {
        Json ret;
        ret.data.emplace<ArrayType>();
        return ret;
    }

    static Json String() {
        Json ret;
        ret.data.emplace<StringType>();
        return ret;
    }

    template<class T> requires (!std::is_same_v<std::decay_t<T>, Json>)
    Json& operator=(T&& val) {
        to_json(*this, std::forward<T>(val));
        return *this;
    }

    using StringValueType = StringType::value_type;

    template<class T> requires (
        !std::is_same_v<T, Json>
        && !std::is_void_v<T>
        && !std::is_pointer_v<T>
        && !std::is_same_v<T, std::nullptr_t>
        && !std::is_same_v<T, StringValueType>
        && !is_initializer_list<T>
        && !std::is_same_v<T, std::string_view>
        )
    operator T() const {
        return Get<T>();
    }

    static Json Parse(const std::string& text) {
        JsonParser parser(text);
        return parser.Parse();
    }

    std::string Dump(int indent = -1)
    {
        JsonPrinter printer(indent);
        return printer.ToString(*this);
    }

    JsonDataType GetType() const
    {
        std::array<JsonDataType, 7> types {
            JsonDataType::Null,
            JsonDataType::Object,
            JsonDataType::Array,
            JsonDataType::String,
            JsonDataType::Integer,
            JsonDataType::Float,
            JsonDataType::Boolean
        };
        
        return types[data.index()];
    }

    bool IsNull() const {
        return std::holds_alternative<NullType>(data);
    }

    bool IsObject() const {
        return std::holds_alternative<ObjectType>(data);
    }

    bool IsArray() const {
        return std::holds_alternative<ArrayType>(data);
    }

    bool IsString() const {
        return std::holds_alternative<StringType>(data);
    }

    bool IsInteger() const {
        return std::holds_alternative<IntegerType>(data);
    }

    bool IsFloat() const {
        return std::holds_alternative<FloatType>(data);
    }

    bool IsBoolean() const {
        return std::holds_alternative<BooleanType>(data);
    }

    Json& GetAt(size_t index) {
        return GetArray().at(index);
    }

    const Json& GetAt(size_t index) const {
        return GetArray().at(index);
    }

    Json& GetAt(const ObjectType::key_type& key) {
        return GetObject().at(key);
    }

    const Json& GetAt(const ObjectType::key_type& key) const {
        return GetObject().at(key);
    }
    
    Json& operator[](size_t index)
    {
        if (IsNull())
            data.emplace<ArrayType>();

        if (index >= GetArray().size())
            GetArray().resize(index + 1);

        return GetArray()[index];
    }

    const Json& operator[](size_t index) const {
        return GetArray().at(index);
    }

    Json& operator[](const ObjectType::key_type& key)
    {
        if (IsNull())
            data.emplace<ObjectType>();

        return GetObject()[key];
    }

    Json& operator[](const char* key)
    {
        if (IsNull())
            data.emplace<ObjectType>();

        return GetObject()[key];
    }

    const Json& operator[](const ObjectType::key_type& key) const {
        return GetObject().at(key);
    }

    const Json& operator[](const char* key) const {
        return GetObject().at(key);
    }
    
    ObjectType& GetObject() {
        return std::get<ObjectType>(data);
    }

    const ObjectType& GetObject() const {
        return std::get<ObjectType>(data);
    }

    ArrayType& GetArray() {
        return std::get<ArrayType>(data);
    }

    const ArrayType& GetArray() const {
        return std::get<ArrayType>(data);
    }

    StringType& GetString() {
        return std::get<StringType>(data);
    }

    const StringType& GetString() const {
        return std::get<StringType>(data);
    }

    IntegerType& GetInteger() {
        return std::get<IntegerType>(data);
    }

    const IntegerType& GetInteger() const {
        return std::get<IntegerType>(data);
    }

    FloatType& GetFloat() {
        return std::get<FloatType>(data);
    }

    const FloatType& GetFloat() const {
        return std::get<FloatType>(data);
    }

    BooleanType& GetBoolean() {
        return std::get<BooleanType>(data);
    }

    const BooleanType& GetBoolean() const {
        return std::get<BooleanType>(data);
    }

    template<class T>
    T Get() const
    {
        T val;
        from_json(*this, val);
        return val;
    }

    template<class T>
    T Get(T defaultValue) const
    {
        T val;
        
        if (!IsNull())
            from_json(*this, val);
        else
            val = defaultValue;

        return val;
    }

    template<>
    Json Get<Json>() const {
        return *this;
    }

    template<class T>
    T GetValue(const ObjectType::key_type& key, T defaultValue) const
    {
        T ret;

        auto it = GetObject().find(key);
        if (it != GetObject().end()) {
            ret = it->second.Get<T>();
        }
        else {
            ret = std::move(defaultValue);
        }

        return ret;
    }

    bool IsEmpty() const
    {
        if (IsObject())
            return GetObject().empty();
        else if (IsArray())
            return GetArray().empty();
        else if (IsString())
            return GetString().empty();
        else if (IsNull())
            return true;
        else // IsInteger() || IsFloat() || IsBoolean()
            return false;
    }

    size_t GetSize() const
    {
        if (IsObject())
            return GetObject().size();
        else if (IsArray())
            return GetArray().size();
        else if (IsString())
            return GetString().size();
        else if (IsNull())
            return 0;
        else // IsInteger() || IsFloat() || IsBoolean()
            return 1;
    }

    void Clear()
    {
        if (IsObject())
            GetObject().clear();
        else if (IsArray())
            GetArray().clear();
        else if (IsString())
            GetString().clear();
        else if (IsInteger())
            GetInteger() = 0;
        else if (IsFloat())
            GetFloat() = 0;
        else if (IsBoolean())
            GetBoolean() = false;
    }
    
    void PushBack(const Json& val)
    {
        if (IsNull())
            data.emplace<ArrayType>();

        GetArray().push_back(val);
    }

    void PushBack(Json&& val)
    {
        if (IsNull())
            data.emplace<ArrayType>();

        GetArray().push_back(std::move(val));
    }
    
    template<class ObjectIter, class ArrayIter, class NullIter = std::nullptr_t>
    class base_const_iterator
    {
        std::variant<NullIter, ObjectIter, ArrayIter> var;
    public:
        base_const_iterator() : var(NullIter{}){}
        base_const_iterator(ObjectIter it) : var(it){}
        base_const_iterator(ArrayIter it) : var(it){}

        bool IsNullIter() const {
            return std::holds_alternative<NullIter>(var);
        }

        bool IsObjectIter() const {
            return std::holds_alternative<ObjectIter>(var);
        }

        bool IsArrayIter() const {
            return std::holds_alternative<ArrayIter>(var);
        }

        NullIter GetNullIter() const {
            return std::get<NullIter>(var);
        }

        ObjectIter GetObjectIter() const {
            return std::get<ObjectIter>(var);
        }

        ObjectIter& GetObjectIter() {
            return std::get<ObjectIter>(var);
        }

        ArrayIter GetArrayIter() const {
            return std::get<ArrayIter>(var);
        }

        ArrayIter& GetArrayIter() {
            return std::get<ArrayIter>(var);
        }

        const ObjectType::key_type& key() const {
            return GetObjectIter()->first;
        }

        const Json& value() const
        {
            if (IsObjectIter())
                return GetObjectIter()->second;
            else if (IsArrayIter())
                return *GetArrayIter();

            throw std::runtime_error("null");
        }

        const Json& operator*() const {
            return value();
        }

        const Json* operator->() const {
            return &value();
        }

        base_const_iterator operator++(int)
        {
            base_const_iterator ret = *this;

            if (IsObjectIter())
                ++GetObjectIter();
            else if (IsArrayIter())
                ++GetArrayIter();
            else
                throw std::runtime_error("null");

            return ret;
        }

        base_const_iterator& operator++()
        {
            if (IsObjectIter())
                ++GetObjectIter();
            else if (IsArrayIter())
                ++GetArrayIter();
            else
                throw std::runtime_error("null");

            return *this;
        }

        bool operator==(const base_const_iterator& it) const
        {
            assert(var.index() == it.var.index());

            bool result;

            if (IsObjectIter() && it.IsObjectIter())
                result = GetObjectIter() == it.GetObjectIter();
            else if (IsArrayIter() && it.IsArrayIter())
                result = GetArrayIter() == it.GetArrayIter();
            else
                result = GetNullIter() == it.GetNullIter();

            return result;
        }

        bool operator!=(const base_const_iterator& it) const {
            return !(*this == it);
        }
    };

    template<class ObjectIter, class ArrayIter>
    class base_iterator : public base_const_iterator<ObjectIter, ArrayIter>
    {
    public:
        Json& value()
        {
            if (this->IsObjectIter())
                return this->GetObjectIter()->second;
            else if (this->IsArrayIter())
                return *this->GetArrayIter();

            throw std::runtime_error("null");
        }

        Json& operator*() {
            return value();
        }

        Json* operator->() {
            return &value();
        }
    };

    using const_iterator = base_const_iterator<ObjectType::const_iterator, ArrayType::const_iterator>;
    using iterator = base_iterator<ObjectType::iterator, ArrayType::iterator>;

    iterator begin()
    {
        if(IsObject())
            return { GetObject().begin() };
        else if(IsArray())
            return { GetArray().begin() };

        return {};
    }

    iterator end()
    {
        if(IsObject())
            return { GetObject().end() };
        else if(IsArray())
            return { GetArray().end() };

        return {};
    }

    const_iterator begin() const
    {
        if(IsObject())
            return { GetObject().cbegin() };
        else if(IsArray())
            return { GetArray().cbegin() };

        return {};
    }

    const_iterator end() const
    {
        if(IsObject())
            return { GetObject().cend() };
        else if(IsArray())
            return { GetArray().cend() };

        return {};
    }

    const_iterator find(const ObjectType::key_type& key) const
    {
        if (IsObject())
            return { GetObject().find(key) };
        
        return {};
    }

    iterator find(const ObjectType::key_type& key)
    {
        if (IsObject())
            return { GetObject().find(key) };

        return {};
    }
};

inline void to_json(Json& obj, const Json::ObjectType& val) {
    obj = Json(val);
}

inline void from_json(const Json& obj, Json::ObjectType& val) {
    val = obj.GetObject();
}

inline void to_json(Json& obj, const Json::ArrayType& val) {
    obj = Json(val);
}

inline void from_json(const Json& obj, Json::ArrayType& val) {
    val = obj.GetArray();
}

template<class T> requires std::is_constructible_v<typename Json::StringType, T>
inline void to_json(Json& obj, const T& val) {
    obj = Json(Json::StringType(val));
}

inline void from_json(const Json& obj, Json::StringType& val) {
    val = obj.GetString();
}

template<class T> requires (std::is_integral_v<T> && !std::is_same_v<T, bool>)
inline void to_json(Json& obj, const T& val) {
    obj = Json((Json::IntegerType)val);
}

template<class T> requires (std::is_integral_v<T> && !std::is_same_v<T, bool>)
inline void from_json(const Json& obj, T& val) {
    val = (T)obj.GetInteger();
}

template<class T> requires std::is_floating_point_v<T>
inline void to_json(Json& obj, const T& val) {
    obj = Json((Json::FloatType)val);
}

template<class T> requires std::is_floating_point_v<T>
inline void from_json(const Json& obj, T& val) {
    val = (T)obj.GetFloat();
}

template<class T> requires std::is_same_v<typename Json::BooleanType, T>
inline void to_json(Json& obj, const T& val) {
    obj = Json(val);
}

template<class T> requires std::is_same_v<typename Json::BooleanType, T>
inline void from_json(const Json& obj, T& val) {
    val = obj.GetBoolean();
}

inline void to_json(Json& obj, const std::nullptr_t& val) {
    obj = Json();
}

inline void from_json(const Json& obj, std::nullptr_t& val) {
    val = nullptr;
}

template<class T>
inline void to_json(Json& obj, const std::forward_list<T>& cont)
{
    Json val = Json::Array();

    for(auto& elem : cont) {
        val.PushBack(elem);
    }

    obj = std::move(val);
}

template<class T>
inline void from_json(const Json& obj, std::forward_list<T>& cont)
{
    auto& arr = obj.GetArray();
    cont.clear();

    for (auto it = arr.rbegin(); it != arr.rend(); ++it) {
        cont.push_front(it->Get<T>());
    }
}

template<class T>
inline void to_json(Json& obj, const std::list<T>& cont)
{
    Json val = Json::Array();

    for(auto& elem : cont) {
        val.PushBack(elem);
    }

    obj = std::move(val);
}

template<class T>
inline void from_json(const Json& obj, std::list<T>& cont)
{
    auto& arr = obj.GetArray();
    cont.clear();

    for(auto& val : arr) {
        cont.push_back(val.Get<T>());
    }
}

template<class T, size_t N>
inline void to_json(Json& obj, const std::array<T, N>& cont)
{
    Json val = Json::Array();

    for(size_t i = 0; i != N; ++i) {
        val.PushBack(cont[i]);
    }

    obj = std::move(val);
}

template<class T, size_t N>
inline void from_json(const Json& obj, std::array<T, N>& cont)
{
    auto& arr = obj.GetArray();
    
    for(size_t i = 0; i != N; ++i) {
        cont[i] = arr[i].Get<T>();
    }
}

template<class T>
inline void to_json(Json& obj, const std::vector<T>& cont)
{
    Json val = Json::Array();

    for(auto& elem : cont)
        val.PushBack(elem);

    obj = std::move(val);
}

template<class T>
inline void from_json(const Json& obj, std::vector<T>& cont)
{
    auto& arr = obj.GetArray();
    cont.clear();

    for(auto& val : arr) {
        cont.push_back(val.Get<T>());
    }
}

template<class K, class T, class H> requires (std::is_constructible_v<typename Json::StringType, K> || has_to_string<K>::value)
inline void to_json(Json& obj, const std::unordered_map<K, T, H>& cont)
{
    Json ret = Json::Object();
    auto& objectValue = ret.GetObject();

    for(auto& [key, value] : cont)
    {
        if constexpr (std::is_constructible_v<typename Json::StringType, K>)
        {
            objectValue[key] = Json(value);
        }
        else
        {
            std::string k;
            to_string(k, key);
            objectValue[k] = Json(value);
        }
    }

    obj = std::move(ret);
}

template<class K, class T, class H> requires (std::is_constructible_v<K, typename Json::StringType> || has_from_string<K>::value)
inline void from_json(const Json& obj, std::unordered_map<K, T, H>& cont)
{
    auto& objectValue = obj.GetObject();
    cont.clear();

    for(auto& [key, val] : objectValue)
    {
        if constexpr (std::is_constructible_v<K, typename Json::StringType>)
        {
            cont[key] = val.Get<T>();
        }
        else
        {
            K k;
            from_string(key, k);
            cont[k] = val.Get<T>();
        }
    }
}

template<class K, class T, class H>
inline void to_json(Json& obj, const std::map<K, T, H>& cont)
{
    Json ret = Json::Object();
    auto& objectValue = ret.GetObject();

    for(auto& [key, value] : cont)
        objectValue[Json(key)] = Json(value);

    obj = std::move(ret);
}

template<class K, class T, class H>
inline void from_json(const Json& obj, std::map<K, T, H>& cont)
{
    auto& objectValue = obj.GetObject();
    cont.clear();

    for(auto& [key, val] : objectValue) {
        cont[Json(key)] = val.Get<T>();
    }
}

inline JsonParser::JsonParser(const std::string& text)
    : lexer(text){}

inline Json JsonParser::Parse()
{
    if (!NextToken())
        throw std::runtime_error("input is empty");

    return ParseValue();
}

inline bool JsonParser::NextToken(bool thrownOnEOF) {
    token = std::move(lexer.GetNextToken());
    return token.type != JsonTokenType::EndOfFile;
}

inline Json JsonParser::ParseValue()
{
    switch (token.type)
    {
    case JsonTokenType::ObjectStart:
        return ParseObject();
    case JsonTokenType::ArrayStart:
        return ParseArray();
    case JsonTokenType::String:
        return ParseString();
    case JsonTokenType::Integer:
        return ParseInteger();
    case JsonTokenType::Float:
        return ParseFloat();
    case JsonTokenType::Boolean:
        return ParseBoolean();
    case JsonTokenType::Null:
        return ParseNull();
    case JsonTokenType::EndOfFile:
        throw std::runtime_error("unexpected end of input");
    default:
        throw std::runtime_error("unexpected token");
    }
}

inline Json JsonParser::ParseString()
{
    assert(token.type == JsonTokenType::String);
    auto ret = Json(token.GetString());
    NextToken(false);
    return ret;
}

inline Json JsonParser::ParseInteger()
{
    assert(token.type == JsonTokenType::Integer);
    auto ret = Json(token.GetInteger());
    NextToken(false);
    return ret;
}

inline Json JsonParser::ParseFloat()
{
    assert(token.type == JsonTokenType::Float);
    auto ret = Json(token.GetFloat());
    NextToken(false);
    return ret;
}

inline Json JsonParser::ParseBoolean()
{
    assert(token.type == JsonTokenType::Boolean);
    auto ret = Json(token.GetBoolean());
    NextToken(false);
    return ret;
}

inline Json JsonParser::ParseNull()
{
    assert(token.type == JsonTokenType::Null);
    auto ret = Json();
    NextToken(false);
    return ret;
}

inline Json JsonParser::ParseObject()
{
    assert(token.type == JsonTokenType::ObjectStart);
    NextToken();

    Json::ObjectType values;

    while (token.type != JsonTokenType::ObjectEnd)
    {
        if (token.type != JsonTokenType::String)
            throw std::runtime_error("expected string");

        Json key = ParseString();

        if (token.type != JsonTokenType::Colon)
            throw std::runtime_error("expected colon");

        NextToken();

        values[key.GetString()] = ParseValue();

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

    return Json(std::move(values));
}

inline Json JsonParser::ParseArray()
{
    assert(token.type == JsonTokenType::ArrayStart);
    NextToken();

    std::vector<Json> values;

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

    return Json(std::move(values));
}

inline void JsonPrinter::Indent(std::ostream& stream, int indent)
{
    if(pretty)
    {
        int totalIndent = indent * indentWidth;

        for(int i = 0; i != totalIndent; ++i)
            stream.put(' ');
    }
}

inline JsonPrinter::JsonPrinter(int indentWidth)
    : indentWidth(indentWidth),
    pretty(indentWidth != -1)
{
}

inline std::string JsonPrinter::ToString(const Json& value)
{
    std::stringstream stream;
    ToStream(stream, 0, value);
    return stream.str();
}

inline void JsonPrinter::WriteEscaped(std::ostream& stream, const std::string& val)
{
    stream << '\"';

    for(auto& c : val)
    {
        if(c == '\"')
            stream << '\\' << '\"';
        else if (c == '\\')
            stream << '\\' << '\\';
        else if(c == '\r')
            stream << '\\' << 'r';
        else if(c == '\n')
            stream << '\\' << 'n';
        else if(c == '\t')
            stream << '\\' << 't';
        else if(c == '\b')
            stream << '\\' << 'b';
        else if(c == '\f')
            stream << '\\' << 'f';
        else
            stream << c;
    }

    stream << '\"';
}

inline void JsonPrinter::ToStream(std::ostream& stream, int indent, const Json& value)
{
    switch (value.GetType())
    {
    default:
    case JsonDataType::Null:
        stream << "null";
        break;

    case JsonDataType::Object:
    {
        auto& obj = value.GetObject();

        stream << '{';
        if (pretty && !obj.empty()) stream << '\n';

        int i = 0;
        for (auto& [key, val] : obj)
        {
            if (i++) {
                stream << ',';
                if (pretty) stream << '\n';
            }

            Indent(stream, indent + 1);

            WriteEscaped(stream, key);
            stream << ':';
            if (pretty) stream << ' ';
            ToStream(stream, indent + 1, val);
        }

        if (!obj.empty())
        {
            if (pretty) stream << '\n';
            Indent(stream, indent);
        }

        stream << '}';
        break;
    }
    case JsonDataType::Array:
    {
        stream << '[';

        auto& arr = value.GetArray();

        if (pretty && !arr.empty()) stream << '\n';

        int i = 0;
        for (auto& elem : arr)
        {
            if (i++) {
                stream << ',';
                if (pretty) stream << '\n';
            }

            Indent(stream, indent + 1);
            ToStream(stream, indent + 1, elem);
        }

        if (!arr.empty())
        {
            if (pretty) stream << '\n';
            Indent(stream, indent);
        }

        stream << ']';
        break;
    }
    case JsonDataType::String:
    {
        auto& val = value.GetString();
        WriteEscaped(stream, val);
        break;
    }
    case JsonDataType::Integer:
    {
        auto integer = value.Get<Json::IntegerType>();
        stream << integer;
        break;
    }
    case JsonDataType::Float:
    {
        auto floating = value.Get<Json::FloatType>();

        constexpr int MaxDigits = 325;
        char chars[MaxDigits];

        auto ret = std::to_chars(
            &chars[0], &chars[0] + MaxDigits,
            floating, std::chars_format::general);

        std::string_view str(&chars[0], ret.ptr);
        if(str.find('.') == std::string_view::npos)
        {
            auto p = ret.ptr;
            *p++ = '.';
            *p++ = '0';
            str = std::string_view(&chars[0], p);
        }

        stream << str;

        break;
    }
    case JsonDataType::Boolean:
    {
        auto boolean = value.Get<Json::BooleanType>();
        stream << std::boolalpha << boolean;
        break;
    }
    }
}
