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

    if(argc>=3){
        string txtFileName = argv[2];
        ifstream txtFile(txtFileName);

        if(!txtFile.is_open()){
            cout<<"ERROR: couldnt open commands file"<<endl;
        }
        else{
            string line;
            while(getline(txtFile, line) && DB_INSTANCE->running){
                ExecuteCommand(line);
            }
            
        }
        txtFile.close();
        DB_INSTANCE->running = 0;
    }

    while(DB_INSTANCE->running){
        cout << "TETO_DB >> ";

        string line;
        getline(cin, line);

        ExecuteCommand(line);

    }

    cout << "Exiting..." << endl;
    delete DB_INSTANCE;

    return 0;
}