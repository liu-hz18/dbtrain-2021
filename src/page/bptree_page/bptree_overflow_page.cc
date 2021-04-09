
#include "bptree_overflow_page.h"
#include <cassert>
#include <vector>
#include <algorithm>

namespace thdb {

const PageOffset OVERFLOW_NODE_NEXT_OFFSET = 4;
const PageOffset OVERFLOW_NODE_SIZE_OFFSET = 8;
const PageOffset OVERFLOW_NODE_DATA_OFFSET = 12;
const Size OVERFLOW_NODE_CAP = (PAGE_SIZE - OVERFLOW_NODE_DATA_OFFSET) / 8; // 溢出节点最多容纳的Value数量

// 请求一个新页面，并初始化
BPTreeOverflowNode::BPTreeOverflowNode(): Page(), _bModified(true), _nNextID(NULL_PAGE) {}

// 从OS加载一个新页面，序列化到该对象
BPTreeOverflowNode::BPTreeOverflowNode(PageID nPageID): Page(nPageID), _bModified(false) {
    Load();
}

BPTreeOverflowNode::~BPTreeOverflowNode() {
    if (_bModified) Store();
}

void BPTreeOverflowNode::Load() {
    MiniOS* minios = MiniOS::GetOS(); // 单例模式，不必销毁
    minios->ReadPage(_nPageID, (uint8_t *)&_nNextID, 4, OVERFLOW_NODE_NEXT_OFFSET);
    Size size;
    minios->ReadPage(_nPageID, (uint8_t *)&size, 4, OVERFLOW_NODE_SIZE_OFFSET);
    assert(size <= OVERFLOW_NODE_CAP);
    for (Size i = 0; i < size; ++i) {
        PageSlotID value;
        minios->ReadPage(_nPageID, (uint8_t *)&(value.first), 4, OVERFLOW_NODE_DATA_OFFSET + i * 8);
        minios->ReadPage(_nPageID, (uint8_t *)&(value.second), 4, OVERFLOW_NODE_DATA_OFFSET + i * 8 + 4);
        _iValueVec.push_back(value);
    }
}

void BPTreeOverflowNode::Store() {
    MiniOS* minios = MiniOS::GetOS(); // 单例模式，不必销毁
    minios->WritePage(_nPageID, (uint8_t *)&_nNextID, 4, OVERFLOW_NODE_NEXT_OFFSET);
    Size size = _iValueVec.size();
    minios->WritePage(_nPageID, (uint8_t *)&size, 4, OVERFLOW_NODE_SIZE_OFFSET);
    for (Size i = 0; i < size; ++i) {
        minios->WritePage(_nPageID, (uint8_t *)&(_iValueVec[i].first), 4, OVERFLOW_NODE_DATA_OFFSET + i * 8);
        minios->WritePage(_nPageID, (uint8_t *)&(_iValueVec[i].second), 4, OVERFLOW_NODE_DATA_OFFSET + i * 8 + 4);
    }
}

Size BPTreeOverflowNode::GetCap() const { return OVERFLOW_NODE_CAP; }

Size BPTreeOverflowNode::GetSize() const { return _iValueVec.size(); }

bool BPTreeOverflowNode::Empty() const { return _iValueVec.empty(); }

bool BPTreeOverflowNode::Full() const { return _iValueVec.size() == OVERFLOW_NODE_CAP; }

PageID BPTreeOverflowNode::GetNextPageID() const { return _nNextID; }

void BPTreeOverflowNode::SetNextPageID(PageID nPageID) { _nNextID = nPageID; _bModified = true; }

std::vector<PageSlotID> BPTreeOverflowNode::GetValues() const { return _iValueVec; }

bool BPTreeOverflowNode::Update(const PageSlotID& iOld, const PageSlotID& iNew) {
    for (Size i = 0; i < _iValueVec.size(); ++i) {
        if (_iValueVec[i] == iOld) {
            _iValueVec[i] = iNew;
            _bModified = true;
            return true;
        }
    }
    return false;
}

bool BPTreeOverflowNode::Insert(const PageSlotID& iValue) {
    if (_iValueVec.size() >= OVERFLOW_NODE_CAP) return false;
    _iValueVec.push_back(iValue);
    _bModified = true;
    return true;
}

bool BPTreeOverflowNode::Delete(const PageSlotID& iValue) {
    auto it = find(_iValueVec.begin(), _iValueVec.end(), iValue);
    if (it == _iValueVec.end()) {
        return false;
    } else {
        _iValueVec.erase(it);
        _bModified = true;
        return true;
    }
}

PageSlotID BPTreeOverflowNode::PopBack() {
    PageSlotID last = _iValueVec.back();
    _iValueVec.pop_back();
    _bModified = true;
    return last;
}

void BPTreeOverflowNode::Clear() {
    if (!Empty()) {
        _iValueVec.clear();
        _bModified = true;
    }
}

}  // namespace thdb
