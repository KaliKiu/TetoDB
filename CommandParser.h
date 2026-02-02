//CommandParser.h

#pragma once

#include <string>
#include <vector>
#include <cctype>
#include <functional>

//namespace more suitable here
namespace CommandParser {
    enum class CommandType{
        Create,
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

     
   
    //functions

    std::unique_ptr<Command> parseCreate(const std::vector<std::string>& args);
    std::unique_ptr<Command> parseInsert(const std::vector<std::string>& args);
    std::unique_ptr<Command> parseSelect(const std::vector<std::string>& args);
    std::unique_ptr<Command> parseDelete(const std::vector<std::string>& args);
    std::unique_ptr<Command> parseSytem(const std::vector<std::string>& args);

    
    
    extern std::map<std::string, std::function<std::unique_ptr<Command>(const std::vector<std::string>&)>> parseFunctions;


    std::vector<std::string> splitArgs(const std::string& input);
    std::unique_ptr<Command> parse(std::string& input);
    
    
};