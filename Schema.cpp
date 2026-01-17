// Schema.cpp

#include "Cursor.h"
#include "Schema.h"
#include "Btree.h"
#include "Pager.h"


Column::Column(const string &name, Type type, size_t size, size_t offset)
    : columnName(name), type(type), size(size), offset(offset) {}

Column::~Column(){}

Row::Row(const vector<Column*> &schema){
    for(Column* c : schema){
        if(c->type == INT) value[c->columnName] = malloc(sizeof(int));
        else value[c->columnName] = malloc(c->size);
    }
}   

Row::~Row(){
    for(auto &p : value) free(p.second);
}

Table::Table(const string &name, const string &meta) 
    : tableName(name), rowCount(0), rowSize(0), rowsPerPage(0), metaName(meta), pager(new Pager(meta+"_"+name+".db"))
{}

Table::Table(const string &name, const string &meta, int rowCount) 
    : tableName(name), rowCount(rowCount), rowSize(0), metaName(meta), pager(new Pager(meta+"_"+name+".db"))
{}

Table::~Table(){
    for(Column* c : schema) delete c;
    schema.clear();

    for(int i = 0;i<MAX_PAGE;i++){
        if(pager->pages[i]) pager->Flush(i, PAGE_SIZE);
    }
    for(auto const& [colName, indexPager] : indexPagers) {
        delete indexPager;
    }
    delete pager;
}

Row* Table::ParseRow(stringstream &ss){
    string str;
    int num;

    Row* r = new Row(schema);
    for(Column* c : schema){
        if(c->type == INT){
            ss >> num;
            *(int*)(r->value[c->columnName]) = num;
        }
        else{
            ss >> quoted(str);
            size_t len = min(str.size(), c->size-1);
            void* dest = r->value[c->columnName];
            memset(dest, 0, c->size);
            memcpy(dest, &str[0], len);
        }
    }
    return r;
}

void Table::SerializeRow(Row* src, void* dest){
    if(src == nullptr || dest == nullptr) return;

    for(Column* c : schema){
        memset(dest, 0, c->size);
        memcpy(dest, src->value[c->columnName], c->size);
        dest = (char*)dest + c->size;
    }
}

void Table::DeserializeRow(void* src, Row* dest){
    if(src == nullptr || dest == nullptr) return;

    for(Column* c : schema){
        memcpy(dest->value[c->columnName], src, c->size);
        src = (char*)src + c->size;
    }
}

void Table::AddColumn(Column* c){
    schema.push_back(c);
    rowSize += c->size;
    rowsPerPage = PAGE_SIZE / rowSize;
    maxRows = rowsPerPage*MAX_PAGE;
}

void* Table::RowSlot(int rowNum){
    int pageNum = rowNum / rowsPerPage;

    if(pageNum >= MAX_PAGE) return nullptr;

    void* page = pager->GetPage(pageNum);

    if(page == nullptr) return nullptr;

    int offset = rowNum % rowsPerPage * rowSize;

    return (char*)page + offset;
}

void Table::CreateIndex(const string& columnName){
    string indexFileName = metaName+"_"+tableName+"_"+columnName+".btree";
    Pager* p = new Pager(indexFileName);

    indexPagers[columnName] = p;

    LeafNode* rootNode = (LeafNode*)p->GetPage(0);
    InitializeLeafNode(rootNode);

    p->Flush(0, PAGE_SIZE);
}

Cursor* Table::StartOfTable(){
    return new Cursor(this, 0);
}

Cursor* Table::EndOfTable(){
    return new Cursor(this, rowCount);
}
