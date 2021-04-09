
#ifndef THDB_BPTREE_OVERFLOW_PAGE_H_
#define THDB_BPTREE_OVERFLOW_PAGE_H_

#include "defines.h"
#include "macros.h"
#include "minios/os.h"
#include "page/page.h"

namespace thdb {

// B+Tree叶子节点的溢出节点，以支持multi-value
class BPTreeOverflowNode: public Page {
    bool _bModified;
    PageID _nNextID; // 溢出节点链表的下一个
    std::vector<PageSlotID> _iValueVec; // 值向量

public:
    // 请求一个新页面，并初始化
    BPTreeOverflowNode();
    // 从OS加载一个新页面，序列化到该对象
    BPTreeOverflowNode(PageID nPageID);
    ~BPTreeOverflowNode();

    Size GetCap() const;
    Size GetSize() const;
    bool Empty() const;
    bool Full() const;
    PageID GetNextPageID() const;
    void SetNextPageID(PageID nPageID);

    void Load();
    void Store();

    PageSlotID PopBack();
    void Clear();
    std::vector<PageSlotID> GetValues() const;
    bool Update(const PageSlotID& iOld, const PageSlotID& iNew);
    bool Insert(const PageSlotID& iValue);
    bool Delete(const PageSlotID& iValue);
};

} // namespace thdb

#endif
