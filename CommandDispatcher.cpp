// CommandDispatcher.cpp

#include "Database.h"
#include "Schema.h"
#include "CommandDispatcher.h"

extern Database* DB_INSTANCE;

Result ProcessCommand(string &line){
    stringstream ss;
    ss << line;

    string cmd, keyword, tableName;

    ss >> cmd >> keyword >> tableName;

    if(cmd == "create") return DB_INSTANCE->CreateTable(tableName, ss);
    if(cmd == "insert") return DB_INSTANCE->Insert(tableName, ss);
    if(cmd == "drop") return DB_INSTANCE->DropTable(tableName);
    if(cmd == "select") {
        Table* t = DB_INSTANCE->GetTable(tableName);

        if(t==nullptr) return Result::TABLE_NOT_FOUND;

        vector<Row*> rows;
        if(!(ss>>keyword)) DB_INSTANCE->SelectAll(t, rows);
        else{
            string columnName;
            int L, R;
            ss >> columnName >> L >> R;
            DB_INSTANCE->SelectWithRange(t, columnName, L, R, rows);
        }

        for(Row* r : rows){
            for(Column* c : t->schema){
                if(c->type == INT) cout << *(int*)(r->value[c->columnName]) << " | ";
                else cout << '"' << (char*)(r->value[c->columnName]) << "\" | ";
            }
            cout << endl;
            delete r;
        }

        return Result::OK;
    }

    return Result::ERROR;
} 

void ProcessDotCommand(string &line){
    stringstream ss;
    ss << line;

    string cmd, tableName;

    ss >> cmd;

    if(cmd == ".exit"){
        DB_INSTANCE->running = false;
        return;
    }
    if(cmd == ".help"){
        cout << "Read the docs, i aint helping" << endl;
        return;
    }
    if(cmd == ".tables"){
        for(auto &[name, table] : DB_INSTANCE->tables) cout << name << endl;
        return;
    }
    if(cmd == ".schema"){
        ss >> tableName;
        Table* t = DB_INSTANCE->GetTable(tableName);

        if(t == nullptr){
            cout << "Name does not exist" << endl;
            return;
        }

        for(Column* c : t->schema){
            cout<<c->columnName<<' '<<c->type<<' '<<c->size<<' '<<c->offset<<endl;
        }

        return;

    }
} 