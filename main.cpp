#include <iostream>
#include <string>
#include "JsonLexer.h"
#include "JsonParser.h"
#include "JsonPrinter.h"
using namespace std;

// Note: Compile as C++14 to avoid deprecation errors about codecvt in JsonLexer

int main(int argc, char** argv)
{
    // read JSON from file into a JSONValue
    auto filename = "test.json";
    JsonParser parser(filename);
    auto jsonValue = parser.Parse();

    // convert JSONValue back to JSON and print to console
    JsonPrinter printer(true, 4);
    std::string json = printer.ToString(jsonValue);
    std::cout << json << std::endl;

    return 0;
}
