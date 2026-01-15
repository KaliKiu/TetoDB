// Pager.h

#pragma once

#include "Common.h"

#define MAX_PAGES 100
#define PAGE_SIZE 4096

class Pager {
public:
    Pager(const std::string& filename);
    ~Pager();

    void* GetPage(int pageNum);
    void Flush(int pageNum, size_t size);
    
public:
    int fileDescriptor;
    size_t fileLength;
    void* pages[MAX_PAGES];
};