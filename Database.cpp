// Database.cpp

#include <vector>
#include <map>
#include "Database.h"
#include "Schema.h"
#include "Cursor.h"
#include "Btree.h"
#include "Pager.h"


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
        if(type == "int"){
            t->AddColumn(new Column(name, INT, 4, offset));
            if(size) t->CreateIndex(name);
        }

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

    Row* r = t->ParseRow(ss);
    if(!r) return Result::INVALID_SCHEMA;
    
    Cursor* cursor = t->EndOfTable();

    int newId = t->rowCount;
    for(Column* c : t->schema){
        if(t->indexPagers.find(c->columnName)!=t->indexPagers.end()){
            Pager* idxPager = t->indexPagers[c->columnName];

            LeafNode* root = (LeafNode*)idxPager->GetPage(0);

            int32_t value = *(int32_t*)r->value[c->columnName];
            LeafNodeInsert(root, value, newId);

            idxPager->Flush(0, PAGE_SIZE);
        }

    }

    t->SerializeRow(r, cursor->Address());
    t->rowCount++;

    delete r;
    delete cursor;
    return Result::OK;
}

void Database::SelectAll(Table* t, vector<Row*>& res){

    res.clear();
    
    Cursor* cursor = t->StartOfTable();
    for(cursor; !cursor->endOfTable; cursor->AdvanceCursor()){
        Row* r = new Row(t->schema);
        t->DeserializeRow(cursor->Address(), r);
        res.push_back(r);
    }
    
    delete cursor;
}

void Database::SelectWithRange(Table* t, const string& columnName, int L, int R, vector<Row*>& res){

    res.clear();

    if(t->indexPagers.count(columnName) == 0){
        Cursor* cursor = t->StartOfTable();
        for(cursor; !cursor->endOfTable; cursor->AdvanceCursor()){
            Row* r = new Row(t->schema);
            t->DeserializeRow(cursor->Address(), r);
            int val = *(int*)r->value[columnName];
            if(L<=val&&val<=R) res.push_back(r);
            else delete r;
        }
        
        delete cursor;

        return;
    }

    Pager* idxPager = t->indexPagers[columnName];
    LeafNode* root = (LeafNode*)idxPager->GetPage(0);

    vector<int> selectedRowIds;
    LeafNodeSelectRange(root, L, R, selectedRowIds);

    sort(selectedRowIds.begin(), selectedRowIds.end());
    for(int rowId : selectedRowIds){
        Row* r = new Row(t->schema);
        void* src = t->RowSlot(rowId);
        t->DeserializeRow(src, r);
        res.push_back(r);
    }

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