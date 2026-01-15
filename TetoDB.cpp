//TetoDB.cpp

#include <iostream>
#include <cstring>
#include <vector>
#include <sstream>

#include "CommandDispatcher.h"
#include "Database.h"

Database* DB_INSTANCE = nullptr;

int main(int argc, char* argv[]){
    if(argc<2){
        cout << "Need filename" <<endl;
        return -1;
    }

    string dbName = argv[1];

    DB_INSTANCE = new Database(dbName);

    
    while(DB_INSTANCE->running){
        std::cout << "TETO_DB >> ";

        std::string command;
        std::getline(std::cin, command);

        if(command[0] == '.') ProcessDotCommand(command);
        else {
            Result res = ProcessCommand(command);
            switch(res){
                case Result::OK:
                case Result::TABLE_ALREADY_EXISTS:
                case Result::TABLE_NOT_FOUND:
                case Result::OUT_OF_STORAGE:
                case Result::INVALID_SCHEMA:
                case Result::ERROR:
                    cout << "Execution code: " << (int)res << endl;
            }
        }

    }

    cout << "Saving and exiting..." << endl;
    delete DB_INSTANCE;

    return 0;
}