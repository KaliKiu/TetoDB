//CommandParser.h

#pragma once

#include <string>
#include <vector>
#include <cctype>

//namespace more suitable here
namespace CommandParser {

    enum class CommandType{
        CreateTable,
        Insert,
        Select,
        Delete,
        System
    };
    //main struct, all commands inherit from this
    struct Command{
        CommandType type;
        std::string tablename;
        virtual ~Command()=default;
    };
    struct CreateTableCommand : Command{
        struct Column{
            std::string name;
            enum class Type{Int,Char}type;
            int extra;
        };
        std::vector<Column> columns;
    };
    //dont rly understand this one
    struct InsertCommand : Command{


    };
    struct SelectCommand : Command{
        bool hasWhere;
        std::string column;
        int min;
        int max;
    };
    struct DeleteCommand : Command{
        bool hasWhere;
        std::string column;
        int min;    //do i need that?
        int max;
    };

    std::vector<std::string> splitArgs(const std::string& input) {
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
};