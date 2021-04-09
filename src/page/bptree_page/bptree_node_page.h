#ifndef THDB_BPTREE_NODE_PAGE_H_
#define THDB_BPTREE_NODE_PAGE_H_

#include "defines.h"
#include "macros.h"
#include "field/fields.h"
#include "page/page.h"
#include "minios/os.h"
#include "exception/exceptions.h"

namespace thdb {

enum class NodeType {
    INNER_NODE_TYPE = 0,
    LEAF_NODE_TYPE = 1,
};

class Index;

// B+Tree节点，内部节点或叶子节点
class BPTreeNode: public Page {
    friend class Index;

    bool _bModified;
    NodeType _iNodeType; // 节点是内部节点还是叶子节点
    // key的类型和大小
    FieldType _iKeyType;
    Size _nKeySize;
    Size _nCap;
    PageID _nNextID;
    PageID _nParentID;
    // 键值对
    std::vector<Field*> _iKeyVec;
    std::vector<PageSlotID> _iChildVec;
    std::vector<PageID> _iOverflowVec;

public:
    BPTreeNode(Size nKeySize, FieldType iKeyType, NodeType iNodeType);
    BPTreeNode(PageID nPageID);
    ~BPTreeNode();
    Size GetKeySize() const;
    FieldType GetKeyType() const;
    Size GetCap() const;
    Size GetSize() const;
    bool Empty() const;
    bool Full() const;
    bool needSplit() const;
    bool needMerge() const;
    bool canBeBorrow() const;
    bool isLeaf() const;
    PageID GetNextLeafID() const;
    PageID GetParentID() const;
    void SetParentID(PageID nPageID);

    void Load();
    void Store();

    Rank LowerBound(Field *pKey); // >= pKey的第一个Key在KeyVec中的位置
    Rank UpperBound(Field *pKey); // > pKey的第一个Key在KeyVec中的位置
    Rank LessOrEqualBound(Field *pKey); // <= pKey 的最后一个Key在KeyVec中的位置
    Rank LessBound(Field* pKey); // < pKey 的最后一个Key在KeyVec中的位置

    std::vector<PageSlotID> GetAllValueByRank(Size rank) const;
    Size DeleteAllInLeaf(Size rank);
    bool DeleteInLeaf(Size rank, const PageSlotID &iPair);
    void InsertInLeaf(Field* pKey, const PageSlotID &iPair);
    bool UpdateInLeaf(Field* pKey, const PageSlotID &iOld, const PageSlotID& iNew);
};

} // namespace thdb

#endif
