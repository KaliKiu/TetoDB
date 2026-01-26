// Pager.h

#pragma once

#include <string>
#include <vector>

using namespace std;


#define PAGE_SIZE 4096



class Pager {
public:
    Pager(const string& filename);
    ~Pager();

    void* GetPage(int pageNum);
    void Flush(int pageNum, size_t size);
    int GetUnusedPageNum(); 
public:
    vector<void*> pages;
    int fileDescriptor;
    int numPages;
    size_t fileLength;
    
};