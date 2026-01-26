// Database.cpp

#include <vector>
#include <stack>
#include <map>
#include "Database.h"
#include "Schema.h"
#include "Btree.h"
#include "Pager.h"


Database::Database(const string& name)
    : metaFileName(name), running(true) 
{
    LoadFromMeta();
}

Database::~Database(){
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
    offset = Table::ROW_HEADER_SIZE;
    

    while(ss >> name >> type >> size){
        if(type == "int"){
            t->AddColumn(new Column(name, INT, 4, offset));
            if(size) t->CreateIndex(name);
            size = 4;
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

    Row* r = t->ParseRow(ss);

    if(!r) return Result::INVALID_SCHEMA;
    

    int newRowId = t->GetNextRowId();
    //cout<<"new row id "<<newRowId<<endl;

    
    for(Column* c : t->schema){
        if(t->indexPagers.find(c->columnName)!=t->indexPagers.end()){
            Pager* idxPager = t->indexPagers[c->columnName];
            int32_t value = *(int32_t*)r->value[c->columnName];
            uint32_t leafPageNum = BtreeFindLeaf(idxPager, 0, value, newRowId);
            LeafNode* leaf = (LeafNode*)idxPager->GetPage(leafPageNum);

            
            InsertResult res = LeafNodeInsert(t, leaf, idxPager, value, newRowId);
            if(res.didSplit){
                if(leaf->header.isRoot) CreateNewRoot((NodeHeader*)leaf, idxPager, res.splitKey, res.splitRowId, res.rightChildPageNum);
                else{
                    InsertIntoParent(idxPager, (NodeHeader*)leaf, res.splitKey, res.splitRowId, res.rightChildPageNum);
                }
            
            } 
            
            // idxPager->Flush(0, PAGE_SIZE);
            // if(res.didSplit) idxPager->Flush(res.rightChildPageNum, PAGE_SIZE);
        }

    }

    t->SerializeRow(r, t->RowSlot(newRowId));

    delete r;

    return Result::OK;
}

void Database::SelectAll(Table* t, vector<Row*>& res){

    res.clear();
    
    for(uint32_t i = 0;i<t->rowCount; i++){
        if(t->IsRowDeleted(i)) continue;
        Row* r = new Row(t->schema);
        void* slot = t->RowSlot(i);
        t->DeserializeRow(slot, r);
        res.push_back(r);
    }
}

int Database::DeleteAll(Table* t){
    int deletedCount = 0;
    for(uint32_t i = 0;i<t->rowCount; i++){
        if(t->IsRowDeleted(i)) continue;
        t->MarkRowDeleted(i);
        deletedCount++;
    }
    return deletedCount;
}

void Database::SelectWithRange(Table* t, const string& columnName, int L, int R, vector<Row*>& res){

    res.clear();

    if(t->indexPagers.count(columnName) == 0){
        
        for(uint32_t i = 0; i<t->rowCount; i++){
            if(t->IsRowDeleted(i)) continue;
            Row* r = new Row(t->schema);
            void* slot = t->RowSlot(i);
            t->DeserializeRow(slot, r);
            int val = *(int*)r->value[columnName];
            if(L<=val&&val<=R) res.push_back(r);
            else delete r;
        }
        return;
    }

    Pager* idxPager = t->indexPagers[columnName];
    uint32_t leafPageNum = BtreeFindLeaf(idxPager, 0, L, 0);
    
    vector<int> selectedRowIds;

    bool firstPage = 1;
    
    while(leafPageNum != 0 || (firstPage && leafPageNum == 0)){
        LeafNode* leaf = (LeafNode*)idxPager->GetPage(leafPageNum);
        LeafNodeSelectRange(t, leaf, L, R, selectedRowIds);

        if(leaf->header.numCells > 0){
            int lastKey = leaf->cells[leaf->header.numCells - 1].key;
            if(lastKey > R) break;
        }
        firstPage = 0;
        leafPageNum = leaf->nextLeaf;
    }
    

    sort(selectedRowIds.begin(), selectedRowIds.end());
    for(int rowId : selectedRowIds){
        if(t->IsRowDeleted(rowId)) continue;
        Row* r = new Row(t->schema);
        void* src = t->RowSlot(rowId);
        t->DeserializeRow(src, r);
        res.push_back(r);
    }

}

int Database::DeleteWithRange(Table* t, const string& columnName, int L, int R){
    if(t->indexPagers.count(columnName) == 0){
        int deletedCount = 0;
        for(uint32_t i = 0; i<t->rowCount; i++){
            if(t->IsRowDeleted(i)) continue;
            
            void* slot = t->RowSlot(i);
            Row* r = new Row(t->schema);
            t->DeserializeRow(slot, r);
            
            int val = *(int*)r->value[columnName];
            if(L<=val&&val<=R){
                t->MarkRowDeleted(i);
                deletedCount++;
            }
            delete r;
        }

        return deletedCount;
    }

    Pager* idxPager = t->indexPagers[columnName];
    return BtreeDelete(t, idxPager, L, R);
}

void Database::FlushToMeta() {
    ofstream ofs(metaFileName+".teto");
    if(!ofs.is_open()) return;

    ofs << tables.size() << endl;
    for(auto const& [name, table] : tables){
        ofs << name << " " << table->rowCount << " " << table->schema.size() << endl;
        for(Column* c : table->schema){
            if(c->type==INT){
                bool hasIndex = table->indexPagers.count(c->columnName);
                ofs << c->columnName << " " << (int)c->type << " " << hasIndex << " " << c->offset << endl;
            }
            else ofs << c->columnName << " " << (int)c->type << " " << c->size << " " << c->offset << endl;
        }
        ofs << table->freeList.size() << endl;
        while(!table->freeList.empty()){
            ofs << table->freeList.top() << " ";
            table->freeList.pop();
        }
        ofs << endl;
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
            if(cTypeInt == INT){
                bool hasIndex = cSize;
                t->AddColumn(new Column(cName, (Type)cTypeInt, 4, cOffset));
                if(hasIndex) t->CreateIndex(cName);
            }
            else t->AddColumn(new Column(cName, (Type)cTypeInt, cSize, cOffset));
        }

        int freeListSize;
        if (ifs >> freeListSize) {
            for(int k=0; k<freeListSize; k++) {
                int id;
                ifs >> id;
                t->freeList.push(id);
            }
        }

        tables[tName] = t;
    }
    ifs.close();
}

void Database::Commit(){
    FlushToMeta();

    for(auto const& [name, table] : tables){
        if(table->pager) table->pager->FlushAll();

        for(auto const& [col, idxPager] : table->indexPagers){
            if(idxPager) idxPager->FlushAll();
        }
    }
}