// Database.cpp

#include "Database.h"


Database::Database(const string& name)
    : metaFileName(name), running(true) 
{
    LoadFromMeta();
}

Database::~Database(){
    FlushToMeta();
    for(auto const& [name, table] : tables) delete table;
    tables.clear();
    running = false;
}

Result Database::CreateTable(const string& tableName, stringstream& ss){
    if(tables.find(tableName) != tables.end()){
        cout << "Error: Table " << tableName << " already exists." << std::endl;
        return Result::TABLE_ALREADY_EXISTS;
    }
    string dbFileName = metaFileName+"_"+tableName + ".db";
    remove(dbFileName.c_str());

    Table* t = new Table(tableName, metaFileName);
    tables[tableName] = t;

    string name, type;
    size_t size, offset;
    offset = 0;
    

    while(ss >> name >> type >> size){
        if(type == "int") t->AddColumn(new Column(name, INT, size, offset));
        else t->AddColumn(new Column(name, STRING, size, offset));

        offset+=size;
    }

    return Result::OK;
}

Table* Database::GetTable(const string& name){
    if(tables.count(name)) return tables[name];
    return nullptr;
}

Result Database::DropTable(const string& name){
    if(tables.find(name) == tables.end()) return Result::TABLE_NOT_FOUND;
    
    delete tables[name]; 
    tables.erase(name);

    return Result::OK;
}

Result Database::Insert(const string& name, stringstream& ss){
    Table* t = GetTable(name);
    if(!t) return Result::TABLE_NOT_FOUND;

    if(t->rowCount >= t->maxRows) return Result::OUT_OF_STORAGE;

    void* slot = t->RowSlot(t->rowCount);
    if(!slot) return Result::OUT_OF_STORAGE;

    Row* r = t->ParseRow(ss);
    t->SerializeRow(r, slot);
    t->rowCount++;

    delete r;
    return Result::OK;
}

vector<Row*> Database::SelectAll(const string &name){
    Table* t = GetTable(name);
    if(!t) return {};

    vector<Row*> res;
    res.clear();

    for(int i = 0; i < t->rowCount; i++){
        void* slot = t->RowSlot(i);
        Row* r = new Row(t->schema);
        t->DeserializeRow(slot, r);
        res.push_back(r);
    }

    return res;
}

void Database::FlushToMeta() {
    ofstream ofs(metaFileName+".teto");
    if(!ofs.is_open()) return;

    ofs << tables.size() << endl;
    for(auto const& [name, table] : tables){
        ofs << name << " " << table->rowCount << " " << table->schema.size() << endl;
        for(Column* c : table->schema){
            ofs << c->columnName << " " << (int)c->type << " " << c->size << " " << c->offset << endl;
        }
    }
    ofs.close();
}

void Database::LoadFromMeta(){
    ifstream ifs(metaFileName+".teto");
    if(!ifs.is_open()) return;

    int numTables;
    if(!(ifs >> numTables)) return;

    for(int i = 0; i < numTables; i++){
        string tName;
        int rCount, colCount;
        ifs >> tName >> rCount >> colCount;

        Table* t = new Table(tName, metaFileName, rCount);
        for(int j = 0; j < colCount; j++){
            string cName;
            int cTypeInt;
            size_t cSize, cOffset;
            ifs >> cName >> cTypeInt >> cSize >> cOffset;
            t->AddColumn(new Column(cName, (Type)cTypeInt, cSize, cOffset));
        }
        tables[tName] = t;
    }
    ifs.close();
}