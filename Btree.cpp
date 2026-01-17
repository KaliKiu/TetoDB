// Btree.cpp

#include "Btree.h"
#include "Common.h"
#include <cstring>
#include <iostream>

void InitializeLeafNode(LeafNode* node){
    node->header.type = LEAF;
    node->header.isRoot = 1;
    node->header.numCells = 0;
    node->header.parent = 0;

    memset(node->cells, 0, NODE_SIZE - HEADER_SIZE);
}

uint16_t LeafNodeFindSlot(LeafNode* node, int32_t targetKey, int32_t targetRowId){
    
    for(uint16_t i = 0; i<node->header.numCells; i++){
        if(node->cells[i].key > targetKey) return i;
        if(node->cells[i].key == targetKey && node->cells[i].rowId >= targetRowId) return i;
    }

    return node->header.numCells;
}

void LeafNodeInsert(LeafNode* node, int32_t key, int32_t rowId){
    if(node->header.numCells >= LEAF_NODE_MAX_CELLS){
        cout<<"Error: Node is full, splitting not implemented"<<endl;
        return;
    }

    uint16_t slot = LeafNodeFindSlot(node, key, rowId);

    int moveSize = (node->header.numCells - slot)*CELL_SIZE;
    if(moveSize){
        void* src = &node->cells[slot];
        void* dest = &node->cells[slot+1];
        memmove(dest, src, moveSize);
    }

    node->cells[slot].key = key;
    node->cells[slot].rowId = rowId;
    node->header.numCells++;
}

void LeafNodeSelectRange(LeafNode* node, int L, int R, vector<int>& outRowIds){
    for(uint16_t i = 0; i<=node->header.numCells; i++){
        int key = node->cells[i].key;
        if(key < L) continue;
        if(key>R) continue;
        outRowIds.push_back(node->cells[i].rowId);
    }
}


