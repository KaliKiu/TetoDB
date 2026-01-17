// Pager.cpp

#include "Pager.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fstream>
#include <iostream>

using namespace std;

Pager::Pager(const std::string& filename){
    fileDescriptor = open(filename.c_str(), O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);

    if(fileDescriptor == -1){
        std::cerr << "Error: Unable to open file " << filename << std::endl;
        exit(1);
    }

    struct stat st;
    if(fstat(fileDescriptor, &st) == 0){
        fileLength = st.st_size;
    }
    else{
        fileLength = 0;
    }


    for(int i = 0; i < MAX_PAGES; i++){
        pages[i] = nullptr;
    }
}

Pager::~Pager(){
    for(int i = 0;i<MAX_PAGES;i++) free(pages[i]);
    close(fileDescriptor);
}

void* Pager::GetPage(int pageNum){
    if(pageNum >= MAX_PAGES){
        std::cerr << "Error: Tried to fetch page number out of bounds." << std::endl;
        return nullptr;
    }


    if(pages[pageNum] != nullptr){
        return pages[pageNum];
    }

    void* page = malloc(PAGE_SIZE);
    int numPagesStored = fileLength / PAGE_SIZE;

    if(pageNum <= numPagesStored){
        lseek(fileDescriptor, pageNum * PAGE_SIZE, SEEK_SET);
        ssize_t bytesRead = read(fileDescriptor, page, PAGE_SIZE);
        
        if(bytesRead == -1){
            std::cerr << "Error reading file: " << errno << std::endl;
            exit(1);
        }
    }

    pages[pageNum] = page;
    return pages[pageNum];
}

void Pager::Flush(int pageNum, size_t size){
    if(pages[pageNum] == nullptr) return;

    off_t offset = lseek(fileDescriptor, pageNum * PAGE_SIZE, SEEK_SET);
    if(offset == -1){
        cerr << "Error seeking file." << std::endl;
        return;
    }

    ssize_t bytesWritten = write(fileDescriptor, pages[pageNum], size);
    if(bytesWritten == -1){
        cerr << "Error writing to file." << std::endl;
        return;
    }

    _commit(fileDescriptor);
}