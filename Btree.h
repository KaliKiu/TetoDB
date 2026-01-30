// Btree.h
#pragma once

#include <cstdint>
#include <vector>

using namespace std;

class Pager;
class Table;


enum NodeType : uint8_t { INTERNAL, LEAF };

struct NodeHeader{
    NodeType type;
    uint8_t isRoot;
    uint16_t numCells;
    int32_t parent;
};

template<typename KeyType>
struct LeafCell{
    KeyType key;
    uint32_t rowId;
};

template<typename KeyType>
struct InternalCell{
    KeyType key;
    uint32_t rowId;
    uint32_t childPage;
};

template<typename KeyType>
struct LeafNode{
    NodeHeader header;
    uint32_t nextLeaf;
    LeafCell<KeyType> cells[0];
};

template<typename KeyType>
struct InternalNode{
    NodeHeader header;
    uint32_t rightChild;
    InternalCell<KeyType> cells[0];
};

template<typename KeyType>
struct InsertResult{
    bool success;
    bool didSplit;
    KeyType splitKey;
    uint32_t splitRowId;
    uint32_t rightChildPageNum;
};


template<typename KeyType>
class Btree{

public:
    Btree(Pager* p, Table* t);
    ~Btree();

    void CreateIndex(uint32_t rootPageNum);

    InsertResult<KeyType> Insert(KeyType key, uint32_t rowId);
    void SelectRange(KeyType L, KeyType R, vector<uint32_t>& outRowIds);
    uint32_t DeleteRange(KeyType L, KeyType R);


    uint32_t FindLeaf(uint32_t pageNum, KeyType key, uint32_t rowId);

private:
    void CreateNewRoot(NodeHeader* root, KeyType splitKey, uint32_t splitRowId, uint32_t rightChildPageNum);
    void InitializeLeafNode(LeafNode<KeyType>* node);

    uint32_t InternalNodeFindChild(InternalNode<KeyType>* node, KeyType targetKey, uint32_t targetRowId);
    uint16_t LeafNodeFindSlot(LeafNode<KeyType>* node, KeyType targetKey, uint32_t targetRowId);

    InsertResult<KeyType> InternalNodeInsert(InternalNode<KeyType>* node, KeyType key, uint32_t rowId, uint32_t rigthChildPage);
    InsertResult<KeyType> LeafNodeInsert(LeafNode<KeyType>* node, KeyType key, uint32_t rowId);

    bool LeafNodeInsertNonFull(LeafNode<KeyType>* node, KeyType key, uint32_t rowId);
    

    void InsertIntoParent(NodeHeader* leftChild, KeyType key, uint32_t rowId, uint32_t rightChildPageNum);
    void UpdateChildParents(InternalNode<KeyType>* parentNode, uint32_t parentPageNum);

    void LeafNodeSelectRange(LeafNode<KeyType>* node, KeyType L, KeyType R, vector<uint32_t>& outRowIds);
    uint16_t LeafNodeDeleteRange(LeafNode<KeyType>* node, KeyType L, KeyType R);
    



public:
    Pager* pager;
    Table* table;
    uint32_t rootPageNum;


public:
    inline static const uint32_t LEAF_NODE_SIZE = 4096;
    inline static const uint32_t INTERNAL_NODE_SIZE = 4096;
    inline static const uint32_t HEADER_SIZE = sizeof(NodeHeader);
    inline static const uint32_t LEAF_CELL_SIZE = sizeof(LeafCell<KeyType>);
    inline static const uint32_t INTERNAL_CELL_SIZE = sizeof(InternalCell<KeyType>);

    inline static const uint32_t LEAF_NODE_MAX_CELLS = (LEAF_NODE_SIZE - sizeof(LeafNode<KeyType>)) / LEAF_CELL_SIZE;
    inline static const uint32_t INTERNAL_NODE_MAX_CELLS = (INTERNAL_NODE_SIZE - sizeof(InternalNode<KeyType>)) / INTERNAL_CELL_SIZE;
};



