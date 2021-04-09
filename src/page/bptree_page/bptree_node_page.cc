
#include "bptree_node_page.h"
#include "bptree_overflow_page.h"
#include <cassert>
#include <vector>
#include <algorithm>

namespace thdb {

const PageOffset NODE_TYPE_OFFSET = 4;
const PageOffset NODE_KEY_TYPE_OFFSET = 8;
const PageOffset NODE_KEY_SIZE_OFFSET = 12;
const PageOffset NODE_SIZE_OFFSET = 16;
const PageOffset NODE_NEXT_OFFSET = 20;
const PageOffset NODE_PARENT_OFFSET = 24;
const PageOffset NODE_DATA_OFFSET = 28;

BPTreeNode::BPTreeNode(Size nKeySize, FieldType iKeyType, NodeType iNodeType): 
    Page(), _bModified(true), _iNodeType(iNodeType), _iKeyType(iKeyType),
    _nKeySize(nKeySize) {
    _nCap = (PAGE_SIZE - NODE_DATA_OFFSET) / (_nKeySize + 8 + 4);
    _nNextID = NULL_PAGE;
    _nParentID = NULL_PAGE;
}
BPTreeNode::BPTreeNode(PageID nPageID): Page(nPageID), _bModified(false) {
    Load();
}
BPTreeNode::~BPTreeNode() {
    if (_bModified) Store();
}

Size BPTreeNode::GetKeySize() const { return _nKeySize; }
FieldType BPTreeNode::GetKeyType() const { return _iKeyType; }
Size BPTreeNode::GetCap() const { return _nCap; }
Size BPTreeNode::GetSize() const { return _iKeyVec.size(); }
bool BPTreeNode::Empty() const { return _iKeyVec.empty(); }
bool BPTreeNode::Full() const { return _iKeyVec.size() >= _nCap; }
bool BPTreeNode::needSplit() const { return _iKeyVec.size() > _nCap; }
bool BPTreeNode::needMerge() const { return _iKeyVec.size() < (_nCap + 1) / 2; }
bool BPTreeNode::canBeBorrow() const { return _iKeyVec.size() > (_nCap + 1) / 2; }
bool BPTreeNode::isLeaf() const { return _iNodeType == NodeType::LEAF_NODE_TYPE; }
PageID BPTreeNode::GetParentID() const { return _nParentID; }
void BPTreeNode::SetParentID(PageID nPageID) { _nParentID = nPageID; _bModified = true; }

PageID BPTreeNode::GetNextLeafID() const {
    assert(isLeaf());
    return _nNextID;
}

std::vector<PageSlotID> BPTreeNode::GetAllValueByRank(Size rank) const {
    assert(rank < _iKeyVec.size());
    std::vector<PageSlotID> result{_iChildVec[rank]};
    if (isLeaf()) {
        PageID curOverflowPageID = _iOverflowVec[rank];
        while(curOverflowPageID != NULL_PAGE) {
            BPTreeOverflowNode* overflowPage = new BPTreeOverflowNode(curOverflowPageID);
            std::vector<PageSlotID> one_result = overflowPage->GetValues();
            result.insert(result.end(), one_result.begin(), one_result.end());
            curOverflowPageID = overflowPage->GetNextPageID();
            delete overflowPage;
        }
    }
    return result;
}

Size BPTreeNode::DeleteAllInLeaf(Size rank) {
    assert(isLeaf());
    assert(rank < _iKeyVec.size());
    Size size = 1;
    PageID curOverflowPageID = _iOverflowVec[rank];
    PageID pageIDtoDelete;
    while(curOverflowPageID != NULL_PAGE) {
        BPTreeOverflowNode* overflowPage = new BPTreeOverflowNode(curOverflowPageID);
        size += overflowPage->GetSize();
        pageIDtoDelete = curOverflowPageID;
        curOverflowPageID = overflowPage->GetNextPageID();
        delete overflowPage;
        MiniOS::GetOS()->DeletePage(pageIDtoDelete);
    }
    _iKeyVec.erase(_iKeyVec.begin() + rank);
    _iChildVec.erase(_iChildVec.begin() + rank);
    _iOverflowVec.erase(_iOverflowVec.begin() + rank);
    _bModified = true;
    return size;
}

bool BPTreeNode::DeleteInLeaf(Size rank, const PageSlotID &iPair) {
    assert(isLeaf());
    assert(rank < _iKeyVec.size());
    bool exist = false;
    if (_iChildVec[rank] == iPair) {
        exist = true;
        bool can_replace = false;
        PageID curOverflowPageID = _iOverflowVec[rank];
        while (curOverflowPageID != NULL_PAGE) {
            BPTreeOverflowNode* overflowPage = new BPTreeOverflowNode(curOverflowPageID);
            if (!overflowPage->Empty()) {
                _iChildVec[rank] = overflowPage->PopBack();
                can_replace = true;
                delete overflowPage;
                break;
            } else {
                curOverflowPageID = overflowPage->GetNextPageID();
                delete overflowPage;
            }
        }
        if (!can_replace) {
            DeleteAllInLeaf(rank);
        }
    } else { // 遍历溢出节点, 如果溢出节点变空了，也不删除溢出节点，因为这种情况比较少
        PageID curOverflowPageID = _iOverflowVec[rank];
        while(curOverflowPageID != NULL_PAGE) {
            BPTreeOverflowNode* overflowPage = new BPTreeOverflowNode(curOverflowPageID);
            if (overflowPage->Delete(iPair)) {
                delete overflowPage;
                exist = true;
                break;
            } else {
                curOverflowPageID = overflowPage->GetNextPageID();
                delete overflowPage;
            }
        }
    }
    if (exist) _bModified = true;
    return exist;
}

void BPTreeNode::InsertInLeaf(Field* pKey, const PageSlotID &iPair) {
    assert(isLeaf());
    Rank rank = LowerBound(pKey);
    if (rank >= int(_iKeyVec.size()) || !Equal(_iKeyVec[rank], pKey, _iKeyType)) {
        _iKeyVec.insert(_iKeyVec.begin() + rank, pKey->Copy());
        _iChildVec.insert(_iChildVec.begin() + rank, iPair);
        _iOverflowVec.insert(_iOverflowVec.begin() + rank, NULL_PAGE);
    } else { // key已经存在
        PageID curOverflowPageID = _iOverflowVec[rank];
        std::vector<BPTreeOverflowNode*> page_list;
        while (curOverflowPageID != NULL_PAGE) {
            BPTreeOverflowNode* overflowPage = new BPTreeOverflowNode(curOverflowPageID);
            page_list.push_back(overflowPage);
            if (overflowPage->Insert(iPair)) {
                break;
            } else {
                curOverflowPageID = overflowPage->GetNextPageID();
            }
        }
        if (curOverflowPageID == NULL_PAGE) {
            BPTreeOverflowNode* overflowPage = new BPTreeOverflowNode();
            if (page_list.size() > 0) {
                page_list.back()->SetNextPageID(overflowPage->GetPageID());
            } else {
                _iOverflowVec[rank] = overflowPage->GetPageID();
            }
            overflowPage->Insert(iPair);
            delete overflowPage;
        }
        for (Size i = 0; i < page_list.size(); ++i) {
            delete page_list[i];
        }
    }
    _bModified = true;
}

bool BPTreeNode::UpdateInLeaf(Field* pKey, const PageSlotID &iOld, const PageSlotID& iNew) {
    bool exist = false;
    Rank rank = LowerBound(pKey);
    if (rank >= int(_iKeyVec.size()) || !Equal(_iKeyVec[rank], pKey, _iKeyType)) {
        exist = false;
    } else {
        if (_iChildVec[rank] == iOld) {
            _iChildVec[rank] = iNew;
            exist = true;
        } else {
            PageID curOverflowPageID = _iOverflowVec[rank];
            while (curOverflowPageID != NULL_PAGE) {
                BPTreeOverflowNode* overflowPage = new BPTreeOverflowNode(curOverflowPageID);
                if (overflowPage->Update(iOld, iNew)) {
                    exist = true;
                    delete overflowPage;
                    break;
                } else {
                    curOverflowPageID = overflowPage->GetNextPageID();
                    delete overflowPage;
                }
            }
        }
    }
    if (exist) _bModified = true;
    return exist;
}

void BPTreeNode::Load() {
    MiniOS* minios = MiniOS::GetOS(); // 单例模式，不必销毁
    minios->ReadPage(_nPageID, (uint8_t *)&_iNodeType, 4, NODE_TYPE_OFFSET);
    minios->ReadPage(_nPageID, (uint8_t *)&_iKeyType, 4, NODE_KEY_TYPE_OFFSET);
    minios->ReadPage(_nPageID, (uint8_t *)&_nKeySize, 4, NODE_KEY_SIZE_OFFSET);
    Size size;
    minios->ReadPage(_nPageID, (uint8_t *)&size, 4, NODE_SIZE_OFFSET);
    minios->ReadPage(_nPageID, (uint8_t *)&_nNextID, 4, NODE_NEXT_OFFSET);
    minios->ReadPage(_nPageID, (uint8_t *)&_nParentID, 4, NODE_PARENT_OFFSET);
    _nCap = (PAGE_SIZE - NODE_DATA_OFFSET) / (_nKeySize + 8 + 4);
    // assert(size <= _nCap);
    PageOffset valueBegin = NODE_DATA_OFFSET + _nCap * _nKeySize;
    PageOffset overflowBegin = valueBegin + _nCap * 8;
    uint8_t* dst = new uint8_t[_nKeySize];
    if (_iNodeType == NodeType::INNER_NODE_TYPE) {
        for (Size i = 0; i < size; ++i) {
            PageSlotID value;
            Field* _field;
            minios->ReadPage(_nPageID, dst, _nKeySize, NODE_DATA_OFFSET + i * _nKeySize);
            minios->ReadPage(_nPageID, (uint8_t *)&(value.first), 4, valueBegin + i * 8);
            if (_iKeyType == FieldType::INT_TYPE) {
                _field = new IntField(dst, _nKeySize);
            } else if (_iKeyType == FieldType::FLOAT_TYPE) {
                _field = new FloatField(dst, _nKeySize);
            } else {
                throw IndexTypeException();
            }
            _iKeyVec.push_back(_field);
            _iChildVec.push_back(value);
        }
    } else {
        for (Size i = 0; i < size; ++i) {
            PageSlotID value;
            PageID page;
            Field* _field;
            minios->ReadPage(_nPageID, dst, _nKeySize, NODE_DATA_OFFSET + i * _nKeySize);
            minios->ReadPage(_nPageID, (uint8_t *)&(value.first), 4, valueBegin + i * 8);
            minios->ReadPage(_nPageID, (uint8_t *)&(value.second), 4, valueBegin + i * 8 + 4);
            minios->ReadPage(_nPageID, (uint8_t *)&page, 4, overflowBegin + i * 4);
            if (_iKeyType == FieldType::INT_TYPE) {
                _field = new IntField(dst, _nKeySize);
            } else if (_iKeyType == FieldType::FLOAT_TYPE) {
                _field = new FloatField(dst, _nKeySize);
            } else {
                throw IndexTypeException();
            }
            _iKeyVec.push_back(_field);
            _iChildVec.push_back(value);
            _iOverflowVec.push_back(page);
        }
    }
    delete[] dst;
}

void BPTreeNode::Store() {
    MiniOS* minios = MiniOS::GetOS(); // 单例模式，不必销毁
    minios->WritePage(_nPageID, (uint8_t *)&_iNodeType, 4, NODE_TYPE_OFFSET);
    minios->WritePage(_nPageID, (uint8_t *)&_iKeyType, 4, NODE_KEY_TYPE_OFFSET);
    minios->WritePage(_nPageID, (uint8_t *)&_nKeySize, 4, NODE_KEY_SIZE_OFFSET);
    Size size = _iKeyVec.size();
    assert(size <= _nCap);
    minios->WritePage(_nPageID, (uint8_t *)&size, 4, NODE_SIZE_OFFSET);
    minios->WritePage(_nPageID, (uint8_t *)&_nNextID, 4, NODE_NEXT_OFFSET);
    minios->WritePage(_nPageID, (uint8_t *)&_nParentID, 4, NODE_PARENT_OFFSET);
    PageOffset valueBegin = NODE_DATA_OFFSET + _nCap * _nKeySize;
    PageOffset overflowBegin = valueBegin + _nCap * 8;
    uint8_t* dst = new uint8_t[_nKeySize];
    if (_iNodeType == NodeType::INNER_NODE_TYPE) {
        for (Size i = 0; i < size; ++i) {
            _iKeyVec[i]->GetData(dst, _nKeySize);
            minios->WritePage(_nPageID, dst, _nKeySize, NODE_DATA_OFFSET + i * _nKeySize);
            minios->WritePage(_nPageID, (uint8_t *)&(_iChildVec[i].first), 4, valueBegin + i * 8);
        }
    } else {
        for (Size i = 0; i < size; ++i) {
            _iKeyVec[i]->GetData(dst, _nKeySize);
            minios->WritePage(_nPageID, dst, _nKeySize, NODE_DATA_OFFSET + i * _nKeySize);
            minios->WritePage(_nPageID, (uint8_t *)&(_iChildVec[i].first), 4, valueBegin + i * 8);
            minios->WritePage(_nPageID, (uint8_t *)&(_iChildVec[i].second), 4, valueBegin + i * 8 + 4);
            minios->WritePage(_nPageID, (uint8_t *)&(_iOverflowVec[i]), 4, overflowBegin + i * 4);
        }
    }
    delete[] dst;
}

// >= pKey的第一个Key在KeyVec中的位置
Rank BPTreeNode::LowerBound(Field *pKey) {
    // 二分查找找下界，此处给出实现
    // 边界的理解非常重要，可以自行重新测试一下
    Rank nBegin = 0, nEnd = _iKeyVec.size();
    while (nBegin < nEnd) {
        Rank nMid = (nBegin + nEnd) / 2;
        if (!Less(_iKeyVec[nMid], pKey, _iKeyType)) {
            nEnd = nMid;
        } else {
            nBegin = nMid + 1;
        }
    }
    return nBegin;
}

// > pKey的第一个Key在KeyVec中的位置
Rank BPTreeNode::UpperBound(Field *pKey) {
    // 二分查找找上界，此处给出实现
    // 边界的理解非常重要，可以自行重新测试一下
    Rank nBegin = 0, nEnd = _iKeyVec.size();
    while (nBegin < nEnd) {
        Rank nMid = (nBegin + nEnd) / 2;
        if (Greater(_iKeyVec[nMid], pKey, _iKeyType)) {
            nEnd = nMid;
        } else {
            nBegin = nMid + 1;
        }
    }
    return nBegin;
}

// <= pKey 的最后一个Key在KeyVec中的位置
Rank BPTreeNode::LessOrEqualBound(Field *pKey) {
    if (Empty()) return -1;
    if (Less(pKey, _iKeyVec[0], _iKeyType)) return -1;
    return UpperBound(pKey) - 1;
}

// < pKey 的最后一个Key在KeyVec中的位置
Rank BPTreeNode::LessBound(Field* pKey) {
    if (Empty()) return -1;
    if (Less(pKey, _iKeyVec[0], _iKeyType)) return -1;
    return LowerBound(pKey) - 1;
}

} // namespace thdb
