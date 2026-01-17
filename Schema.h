//Schema.h

#pragma once


#include "Common.h"
#include "Pager.h"


#define MAX_PAGE 100
#define PAGE_SIZE 4096

class Cursor;

class Column{
public:
    Column(const string &name, Type type, size_t size, size_t offset);
    ~Column();

public:
    string columnName;
    Type type;
    size_t size;
    size_t offset;
};

class Row{
public:
    Row();
    Row(const vector<Column*> &schema);
    ~Row();

public:
    map<string, void*> value;
};

class Table{
public:
    Table(const string &name, const string &meta); // make new table
    Table(const string &name, const string &meta, int rowCount); // load table from disk
    ~Table();

    Row* ParseRow(stringstream &ss);
    void SerializeRow(Row* src, void* dest);
    void DeserializeRow(void* src, Row* dest);
    void AddColumn(Column* c);
    void* RowSlot(int rowNum);

    void CreateIndex(const string& columnName);

    Cursor* StartOfTable();
    Cursor* EndOfTable();

public:
    string metaName;
    string tableName;
    vector<Column*> schema;
    map<string, Pager*> indexPagers;
    Pager* pager;

    int rowCount;
    int rowSize;
    int rowsPerPage;
    int maxRows;

};

