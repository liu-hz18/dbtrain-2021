#include "index/index.h"
#include <cassert>
#include <vector>
#include <algorithm>

namespace thdb {
Index::Index(FieldType iType): _iKeyType(iType) {
  // 建立一个新的根结点，注意需要基于类型判断根结点的属性
  // 注意记录RootID
  if (iType == FieldType::INT_TYPE) {
    _nKeySize = 4;
  } else if (iType == FieldType::FLOAT_TYPE) {
    _nKeySize = 8;
  } else {
    throw IndexTypeException();
  }
  BPTreeNode* root = new BPTreeNode(_nKeySize, _iKeyType, NodeType::LEAF_NODE_TYPE);
  _nRootID = root->GetPageID();
  delete root;
}

Index::Index(PageID nPageID) {
  // 记录RootID即可
  _nRootID = nPageID;
  BPTreeNode* root = new BPTreeNode(_nRootID);
  _nKeySize = root->GetKeySize();
  _iKeyType = root->GetKeyType();
  delete root;
}

Index::~Index() {}

PageID Index::GetRootID() const { return _nRootID; }

// 递归处理上溢, 所有的新节点在这里产生，注意维护父子关系
void Index::SolveOverflow(BPTreeNode* curPage) {
    if (!curPage->needSplit()) {
        delete curPage;
        return;
    }
    Size half = curPage->GetCap() / 2;
    // 分裂出去右半部分
    BPTreeNode* neighborPage;
    if (curPage->isLeaf()) {
        neighborPage = new BPTreeNode(_nKeySize, _iKeyType, NodeType::LEAF_NODE_TYPE);
        neighborPage->_iOverflowVec.insert(neighborPage->_iOverflowVec.end(), curPage->_iOverflowVec.begin()+half+1, curPage->_iOverflowVec.end());
        curPage->_iOverflowVec.erase(curPage->_iOverflowVec.begin() + half+1, curPage->_iOverflowVec.end());
        neighborPage->_nNextID = curPage->_nNextID;
        curPage->_nNextID = neighborPage->GetPageID();
    } else {
        neighborPage = new BPTreeNode(_nKeySize, _iKeyType, NodeType::INNER_NODE_TYPE);
    }
    neighborPage->_iKeyVec.insert(neighborPage->_iKeyVec.end(), curPage->_iKeyVec.begin()+half+1, curPage->_iKeyVec.end());
    curPage->_iKeyVec.erase(curPage->_iKeyVec.begin() + half+1, curPage->_iKeyVec.end());
    neighborPage->_iChildVec.insert(neighborPage->_iChildVec.end(), curPage->_iChildVec.begin()+half+1, curPage->_iChildVec.end());
    curPage->_iChildVec.erase(curPage->_iChildVec.begin() + half+1, curPage->_iChildVec.end());
    neighborPage->_bModified = true;
    curPage->_bModified = true;
    // 维护新节点子代的父子关系
    if (!neighborPage->isLeaf()) {
        Size size = neighborPage->GetSize();
        for (Size i = 0; i < size; ++i) {
            PageID childPageID = neighborPage->_iChildVec[i].first;
            BPTreeNode* child = new BPTreeNode(childPageID);
            child->SetParentID(neighborPage->GetPageID());
            delete child;
        }
    }
    // 维护新节点父代的父子关系
    PageID fatherID = curPage->GetParentID();
    BPTreeNode* father;
    if (fatherID == NULL_PAGE) {
        assert(curPage->GetPageID() == _nRootID);
        father = new BPTreeNode(_nKeySize, _iKeyType, NodeType::INNER_NODE_TYPE);
        father->_iKeyVec.push_back(curPage->_iKeyVec[0]);
        father->_iChildVec.push_back(std::pair<PageID, SlotID>(curPage->GetPageID(), 0));
        _nRootID = fatherID = father->GetPageID();
    } else {
        father = new BPTreeNode(fatherID);
    }
    // 得到新节点key插入位置
    Rank rank = father->LowerBound(neighborPage->_iKeyVec[0]);
    father->_iKeyVec.insert(father->_iKeyVec.begin() + rank, neighborPage->_iKeyVec[0]);
    father->_iChildVec.insert(father->_iChildVec.begin() + rank, std::pair<PageID, SlotID>(neighborPage->GetPageID(), 0));
    father->_bModified = true;
    neighborPage->SetParentID(fatherID);
    curPage->SetParentID(fatherID);
    delete neighborPage;
    delete curPage;
    SolveOverflow(father);
}

// 递归处理下溢
void Index::SolveUnderflow(BPTreeNode* curPage) {
    if (!curPage->needMerge()) {
        delete curPage;
        return;
    }
    // 判断是不是根节点，如果根节点只有一个孩子，则树高降低一层
    PageID fatherID = curPage->GetParentID();
    PageID curPageID = curPage->GetPageID();
    if (fatherID == NULL_PAGE) {
        if (curPage->_iKeyVec.size() == 1 && !curPage->isLeaf()) { // 如果根节点有唯一的孩子
            _nRootID = curPage->_iChildVec[0].first; // 更改根节点
            BPTreeNode* newRootPage = new BPTreeNode(_nRootID);
            newRootPage->SetParentID(NULL_PAGE);
            // 根节点不再有用，回收
            delete curPage;
            delete newRootPage;
            MiniOS::GetOS()->DeletePage(curPageID);
        } else {
            delete curPage;
        }
        return;
    }
    // 得到父节点
    BPTreeNode* fatherNode = new BPTreeNode(fatherID);
    // 确定当前节点是父亲的第几个孩子
    Size rank = 0;
    while(fatherNode->_iChildVec[rank].first != curPageID) ++rank;

    // case 1: 向左兄弟借1个
    if (rank > 0) { // 有左兄弟
        PageID leftNodePageID = fatherNode->_iChildVec[rank-1].first;
        BPTreeNode* leftNode = new BPTreeNode(leftNodePageID);
        if (leftNode->canBeBorrow()) {
            Size size = leftNode->_iKeyVec.size();
            curPage->_iKeyVec.insert(curPage->_iKeyVec.begin(), leftNode->_iKeyVec[size-1]);
            leftNode->_iKeyVec.pop_back();
            curPage->_iChildVec.insert(curPage->_iChildVec.begin(), leftNode->_iChildVec[size-1]);
            leftNode->_iChildVec.pop_back();
            fatherNode->_iKeyVec[rank] = curPage->_iKeyVec[0];
            if (curPage->isLeaf()) {
                curPage->_iOverflowVec.insert(curPage->_iOverflowVec.begin(), leftNode->_iOverflowVec[size-1]);
                leftNode->_iOverflowVec.pop_back();
            } else {
                BPTreeNode* childNode = new BPTreeNode(curPage->_iChildVec[0].first);
                childNode->SetParentID(curPageID);
                delete childNode;
            }
            fatherNode->_bModified = true;
            leftNode->_bModified = true;
            curPage->_bModified = true;
            delete leftNode;
            delete curPage;
            delete fatherNode;
            return;
        }
        delete leftNode;
    }

    // case 2: 向右兄弟借1个
    if (rank < fatherNode->_iChildVec.size() - 1) {
        PageID rightNodePageID = fatherNode->_iChildVec[rank+1].first;
        BPTreeNode* rightNode = new BPTreeNode(rightNodePageID);
        if (rightNode->canBeBorrow()) {
            curPage->_iKeyVec.push_back(rightNode->_iKeyVec[0]);
            rightNode->_iKeyVec.erase(rightNode->_iKeyVec.begin());
            curPage->_iChildVec.push_back(rightNode->_iChildVec[0]);
            rightNode->_iChildVec.erase(rightNode->_iChildVec.begin());
            fatherNode->_iKeyVec[rank+1] = rightNode->_iKeyVec[0];
            if (curPage->isLeaf()) {
                curPage->_iOverflowVec.push_back(rightNode->_iOverflowVec[0]);
                rightNode->_iOverflowVec.erase(rightNode->_iOverflowVec.begin());
            } else {
                BPTreeNode* childNode = new BPTreeNode(curPage->_iChildVec.back().first);
                childNode->SetParentID(curPageID);
                delete childNode;
            }
            fatherNode->_bModified = true;
            rightNode->_bModified = true;
            curPage->_bModified = true;
            delete rightNode;
            delete fatherNode;
            delete curPage;
            return;
        }
        delete rightNode;
    }

    // case 3: 左右兄弟都不够借, 左右兄弟可以没有，但至少有一个。选择合并
    // 合并会引起父节点可能下溢，递归
    if (rank > 0) { // case 3.1: 和左兄弟合并
        PageID leftNodePageID = fatherNode->_iChildVec[rank-1].first;
        BPTreeNode* leftNode = new BPTreeNode(leftNodePageID);
        Size size_before = leftNode->_iKeyVec.size();
        leftNode->_iKeyVec.insert(leftNode->_iKeyVec.end(), curPage->_iKeyVec.begin(), curPage->_iKeyVec.end());
        leftNode->_iChildVec.insert(leftNode->_iChildVec.end(), curPage->_iChildVec.begin(), curPage->_iChildVec.end());
        if (leftNode->isLeaf()) {
            leftNode->_iOverflowVec.insert(leftNode->_iOverflowVec.end(), curPage->_iOverflowVec.begin(), curPage->_iOverflowVec.end());
            leftNode->_nNextID = curPage->_nNextID;
        } else {
            Size size_after = leftNode->_iKeyVec.size();
            for (Size i = size_before; i < size_after; ++i) {
                BPTreeNode* child = new BPTreeNode(leftNode->_iChildVec[i].first);
                child->SetParentID(leftNodePageID);
                delete child;
            }
        }
        fatherNode->_iKeyVec.erase(fatherNode->_iKeyVec.begin() + rank);
        fatherNode->_iChildVec.erase(fatherNode->_iChildVec.begin() + rank);
        fatherNode->_bModified = true;
        leftNode->_bModified = true;
        delete curPage;
        delete leftNode;
        MiniOS::GetOS()->DeletePage(curPageID);
    } else { // case 3.2: 和右兄弟合并
        PageID rightNodePageID = fatherNode->_iChildVec[rank+1].first;
        BPTreeNode* rightNode = new BPTreeNode(rightNodePageID);
        Size size_before = curPage->_iKeyVec.size();
        curPage->_iKeyVec.insert(curPage->_iKeyVec.end(), rightNode->_iKeyVec.begin(), rightNode->_iKeyVec.end());
        curPage->_iChildVec.insert(curPage->_iChildVec.end(), rightNode->_iChildVec.begin(), rightNode->_iChildVec.end());
        if (curPage->isLeaf()) {
            curPage->_iOverflowVec.insert(curPage->_iOverflowVec.end(), rightNode->_iOverflowVec.begin(), rightNode->_iOverflowVec.end());
            curPage->_nNextID = rightNode->_nNextID;
        } else {
            Size size_after = curPage->_iKeyVec.size();
            for (Size i = size_before; i < size_after; ++i) {
                BPTreeNode* child = new BPTreeNode(curPage->_iChildVec[i].first);
                child->SetParentID(curPageID);
                delete child;
            }
        }
        fatherNode->_iKeyVec.erase(fatherNode->_iKeyVec.begin() + rank + 1);
        fatherNode->_iChildVec.erase(fatherNode->_iChildVec.begin() + rank + 1);
        fatherNode->_bModified = true;
        curPage->_bModified = true;
        delete curPage;
        delete rightNode;
        MiniOS::GetOS()->DeletePage(rightNodePageID);
    }
    SolveUnderflow(fatherNode);
}

void Index::ClearInner(PageID nPageID) {
    BPTreeNode* root = new BPTreeNode(nPageID);
    Rank size = root->GetSize();
    if (root->isLeaf()) {
        for (Rank i = size-1; i >= 0; --i) {
            root->DeleteAllInLeaf(i);
        }
    } else {
        for (Rank i = 0; i < size; i++) {
            ClearInner(root->_iChildVec[i].first);
        }
    }
    delete root;
    MiniOS::GetOS()->DeletePage(nPageID);
}

void Index::Clear() {
  ClearInner(_nRootID);
}

bool Index::Insert(Field* pKey, const PageSlotID &iPair) {
    PageID leafPage = searchIntoLeafInsert(pKey);
    BPTreeNode* leaf = new BPTreeNode(leafPage);
    assert(leaf->isLeaf());
    leaf->InsertInLeaf(pKey, iPair);
    SolveOverflow(leaf);
    return true;
}

Size Index::Delete(Field *pKey) {
    PageID leafPage = searchIntoLeaf(pKey);
    BPTreeNode* leaf = new BPTreeNode(leafPage);
    assert(leaf->isLeaf());
    Rank rank = leaf->LessOrEqualBound(pKey);
    Size size = 0;
    if (rank < 0 || !Equal(leaf->_iKeyVec[rank], pKey, _iKeyType)) {
        delete leaf;
    } else {
        size = leaf->DeleteAllInLeaf(rank);
        SolveUnderflow(leaf);
    }
    return size;
}

bool Index::Delete(Field* pKey, const PageSlotID &iPair) {
    PageID leafPage = searchIntoLeaf(pKey);
    BPTreeNode* leaf = new BPTreeNode(leafPage);
    assert(leaf->isLeaf());
    Rank rank = leaf->LessOrEqualBound(pKey);
    bool exist = false;
    if (rank < 0 || !Equal(leaf->_iKeyVec[rank], pKey, _iKeyType)) {
        delete leaf;
    } else {
        exist = leaf->DeleteInLeaf(rank, iPair);
        SolveUnderflow(leaf);
    }
    return exist;
}

bool Index::Update(Field* pKey, const PageSlotID &iOld, const PageSlotID& iNew) {
    PageID leafPage = searchInfoLeafEqual(pKey);
    if (leafPage == NULL_PAGE) {
        return false;
    } else {
        BPTreeNode* leaf = new BPTreeNode(leafPage);
        assert(leaf->isLeaf());
        bool update = leaf->UpdateInLeaf(pKey, iOld, iNew);
        delete leaf;
        return update;
    }
}

std::vector<PageSlotID> Index::Range(Field *pLow, Field *pHigh) {
    PageID leafID = searchIntoLeaf(pLow);
    std::vector<PageSlotID> result;
    // 沿着叶节点链表找
    while (leafID != NULL_PAGE) {
        BPTreeNode* curleaf = new BPTreeNode(leafID);
        assert(curleaf->isLeaf());
        Size begin_rank = curleaf->LowerBound(pLow);
        if (begin_rank >= curleaf->GetSize()) {
            delete curleaf;
            break;
        } else {
            Size i;
            for (i = begin_rank; i < curleaf->GetSize(); ++i) {
                if (!Less(curleaf->_iKeyVec[i], pHigh, _iKeyType)) break;
                std::vector<PageSlotID> temp = curleaf->GetAllValueByRank(i);
                result.insert(result.end(), temp.begin(), temp.end());
            }
            if (i < curleaf->GetSize()) {
                leafID = curleaf->GetNextLeafID();
                delete curleaf;
            } else {
                delete curleaf;
                break;
            }
        }
    }
    return result;
}

PageID Index::searchIntoLeaf(Field* pKey) const {
    BPTreeNode* root = new BPTreeNode(_nRootID);
    while (!root->isLeaf()) {
        Rank rank = root->LessOrEqualBound(pKey);
        if (rank < 0) rank = 0;
        PageID nextLevelID = root->_iChildVec[rank].first;
        delete root;
        root = new BPTreeNode(nextLevelID);
    }
    PageID leafPage = root->GetPageID();
    delete root;
    return leafPage;
}

PageID Index::searchIntoLeafInsert(Field* pKey) {
    BPTreeNode* root = new BPTreeNode(_nRootID);
    while (!root->isLeaf()) {
        Rank rank = root->LessOrEqualBound(pKey);
        if (rank < 0) rank = 0;
        if (Less(pKey, root->_iKeyVec[rank], _iKeyType)) {
            root->_iKeyVec[rank] = pKey;
            root->_bModified = true;
        }
        PageID nextLevelID = root->_iChildVec[rank].first;
        delete root;
        root = new BPTreeNode(nextLevelID);
    }
    PageID leafPage = root->GetPageID();
    delete root;
    return leafPage;
}

PageID Index::searchInfoLeafEqual(Field* pKey) const {
    BPTreeNode* root = new BPTreeNode(_nRootID);
    while (!root->isLeaf()) {
        Rank rank = root->LessOrEqualBound(pKey);
        if (rank < 0) {
            delete root;
            return NULL_PAGE;
        }
        PageID nextLevelID = root->_iChildVec[rank].first;
        delete root;
        root = new BPTreeNode(nextLevelID);
    }
    Rank rank = root->LessOrEqualBound(pKey);
    if (rank < 0 || !Equal(root->_iKeyVec[rank], pKey, _iKeyType)) {
        delete root;
        return NULL_PAGE;
    } else {
        PageID leafPage = root->GetPageID();
        delete root;
        return leafPage;
    }
}

}  // namespace thdb
