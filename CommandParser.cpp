
#include "CommandParser.h"
#include "Common.h"

#include <sstream>
#include <algorithm>
#include <iomanip>
#include <map>

namespace CommandParser{
    using Commandptr = std::unique_ptr<CommandParser::Command>;
    std::map<std::string, std::function<std::unique_ptr<CommandParser::Command>(const std::vector<std::string>&)>> parseFunctions={
        {"CREATE", CommandParser::parseCreate},
        {"INSERT", CommandParser::parseInsert},
        {"SELECT", CommandParser::parseSelect},
        {"DELETE", CommandParser::parseDelete}
    };

    std::vector<std::string> CommandParser::splitArgs(const std::string& input) {
        std::vector<std::string> args;
        std::string arg;
        bool inQuotes = false;

        for (size_t i = 0; i < input.size(); ++i) {
            char t = input[i];

            if (t == '"') {
                inQuotes = !inQuotes;
                continue;
            }
            // split on whitespace if not inside quotes
            if (!inQuotes && std::isspace(static_cast<unsigned char>(t))) {
                if (!arg.empty()) {
                    args.push_back(arg);
                    arg.clear();
                }
            } else {
                arg += t;
            }
        }
        if (!arg.empty())
            args.push_back(arg);

        return args;
    }
    Commandptr CommandParser::parseCreate(const std::vector<std::string>& args){

    }
    Commandptr CommandParser::parseInsert(const std::vector<std::string>& args){

    }
    Commandptr CommandParser::parseSelect(const std::vector<std::string>& args){

    }
    Commandptr CommandParser::parseDelete(const std::vector<std::string>& args){

    }
    Commandptr CommandParser::parse(std::string& input){
        auto args = splitArgs(input);
        if(args.empty())return nullptr;
        std::string cmd = args[0];
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
        
        //search in map<string,function> if string exists, call respective parse<Type> function
        if(auto it =parseFunctions.find(cmd);it !=parseFunctions.end()){
            return it->second(args);
        }
        //if something went wrong:
        return nullptr;
    }
}