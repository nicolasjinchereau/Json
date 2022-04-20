#include <cassert>
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include "Json.h"

std::string ReadFile(const std::string& filename)
{
    std::ifstream fin(filename, std::ios::in | std::ios::binary);

    if (!fin.good())
        throw std::runtime_error("failed to open file");

    fin.seekg(0, std::ios::end);
    auto sz = (size_t)fin.tellg();
    fin.seekg(0, std::ios::beg);

    if (sz == 0)
        throw std::runtime_error("file is empty");

    std::string contents;
    contents.resize(sz);
    fin.read(contents.data(), sz);

    return contents;
}

void WriteFile(const std::string& filename, const std::string& contents)
{
    std::ofstream fout(filename, std::ios::out | std::ios::binary);
    fout.write(contents.data(), contents.size());
    if(fout.bad())
        throw std::runtime_error("write failed");
}

void TestParsing()
{
    std::string text = ReadFile("test.json");
    Json obj = Json::Parse(text);
    std::string dump = obj.Dump(2);
    std::cout << dump << std::endl;

    std::cout << std::endl << std::endl;

    WriteFile("dump.json", text);

    std::string text2 = ReadFile("dump.json");
    Json obj2 = Json::Parse(text2);
    std::string dump2 = obj2.Dump(2);
    std::cout << dump2 << std::endl << std::endl;
}

struct Parent
{
    std::string name;
    uint64_t number{};
};

struct Child
{
    std::string name;
    int age{};
};

struct Family
{
    std::string address;
    std::vector<Parent> parents;
    std::vector<Child> children;
};

void to_json(Json& obj, const Parent& parent) {
    obj["name"] = parent.name;
    obj["number"] = parent.number;
}

void from_json(const Json& obj, Parent& parent) {
    parent.name = obj["name"];
    parent.number = obj["number"];
}

void to_json(Json& obj, const Child& child) {
    obj["name"] = child.name;
    obj["age"] = child.age;
}

void from_json(const Json& obj, Child& child) {
    child.name = obj["name"];
    child.age = obj["age"];
}

void to_json(Json& obj, const Family& family) {
    obj["address"] = family.address;
    obj["parents"] = family.parents;
    obj["children"] = family.children;
}

void from_json(const Json& obj, Family& family) {
    family.address = obj["address"];
    family.parents = obj["parents"];
    family.children = obj["children"];
}

void TestConversion()
{
    Family family;
    family.address = "500 Ocean Avenue";
    family.parents.push_back({ "Tom", 555'567'1234});
    family.parents.push_back({ "Jane", 555'765'4321});
    family.children.push_back({ "Sally", 5 });
    family.children.push_back({ "Chucky", 7 });
    family.children.push_back({ "Randy", 12 });
    family.children.push_back({ "Ronda", 15 });

    Json obj = family;
    std::cout << obj.Dump(2) << std::endl;

    Family result = obj;

    assert(result.address == family.address);
    assert(result.parents.size() == family.parents.size());
    assert(result.children.size() == family.children.size());
}

int main(int argc, char** argv)
{
    TestParsing();
    TestConversion();

    return 0;
}
