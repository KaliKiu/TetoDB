// Btree.h
#pragma once

#include <cstdint>

enum NodeType : uint8_t { INTERNAL, LEAF };

struct NodeHeader{
    NodeType type;
    uint8_t isRoot;
    uint16_t numCells;
    int32_t parent;
};

struct LeafCell{
    int32_t key;
    int32_t rowId;
};

struct LeafNode{
    NodeHeader header;
    LeafCell cells[0];
};

void InitializeLeafNode(LeafNode* node);
void LeafNodeInsert(LeafNode* node, int32_t key, int32_t rowId);
uint16_t LeafNodeFindSlot(LeafNode* node, int32_t targetKey, int32_t targetRowId);
const uint32_t NODE_SIZE = 4096;
const uint32_t HEADER_SIZE = sizeof(NodeHeader);
const uint32_t CELL_SIZE = sizeof(LeafCell);

const uint32_t LEAF_NODE_MAX_CELLS = (NODE_SIZE - HEADER_SIZE) / CELL_SIZE;