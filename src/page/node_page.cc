#include "page/node_page.h"

#include <assert.h>
#include <float.h>

#include "exception/exceptions.h"
#include "field/fields.h"
#include "macros.h"
#include "minios/os.h"

namespace thdb {

const PageOffset LEAF_OFFSET = 8;
const PageOffset USED_SLOT_OFFSET = 12;
const PageOffset KEY_LEN_OFFSET = 16;
const PageOffset KEY_TYPE_OFFSET = 20;

NodePage::NodePage(Size nKeyLen, FieldType iKeyType, bool bLeaf)
    : _nKeyLen(nKeyLen), _iKeyType(iKeyType), _bLeaf(bLeaf) {
  // TODO: 基于自己实现的Store算法确定最大容量
  // TODO: 如果为中间结点，注意可能需要初始化第一个子结点
}

NodePage::NodePage(Size nKeyLen, FieldType iKeyType, bool bLeaf,
                   const std::vector<Field *> &iKeyVec,
                   const std::vector<PageSlotID> &iChildVec)
    : _nKeyLen(nKeyLen),
      _iKeyType(iKeyType),
      _bLeaf(bLeaf),
      _iKeyVec(iKeyVec),
      _iChildVec(iChildVec) {
  // TODO: 基于自己实现的Store算法确定最大容量
}

NodePage::NodePage(PageID nPageID) : Page(nPageID) {
  // TODO: 从格式化页面中导入结点信息
  // TODO: 确定最大容量
}

NodePage::~NodePage() {
  // TODO: 将结点信息格式化并写回到页面中
  // TODO: 注意析构KeyVec中的指针
}

bool NodePage::Insert(Field *pKey, const PageSlotID &iPair) {
  // TODO: 需要基于结点类型判断执行过程
  // 叶结点：
  // 1.确定插入位置后插入数据即可
  // 中间结点:
  // 1.确定执行插入函数的子结点
  // 2.对应的子结点执行插入函数
  // 3.判断子结点是否为满结点，满结点时执行分裂
  // 4.子结点分裂情况下需要更新KeyVec和ChildVec

  // ALERT:
  // 中间结点执行插入过程中，需要考虑到实际中间结点为空结点的特殊情况进行特判
  // ALERT: 对于头结点的插入可能更新头结点的Key值
  // ALERT: KeyVec中的Key的赋值需要使用深拷贝，否则会出现析构导致的问题
  // ALERT: 上层保证每次插入的iPair不同
}

Size NodePage::Delete(Field *pKey) {
  // TODO: 需要基于结点类型判断执行过程
  // 叶结点：
  // 1.确定删除位置后删除数据即可
  // 中间结点:
  // 1.确定执行删除函数的子结点
  // 2.对应的子结点执行删除函数
  // 3.判断子结点是否为满结点，空结点时清除空结点
  // 4.删除空结点情况下需要更新KeyVec和ChildVec

  // ALERT: 注意删除结点过程中如果清除了Key则需要析构
  // ALERT:
  // 注意存在键值相同的情况发生，所以需要保证所有需要执行删除函数的子结点都执行了删除函数
  // ALERT: 可以适当简化合并函数，例如不删除空的中间结点
}

bool NodePage::Delete(Field *pKey, const PageSlotID &iPair) {
  // TODO: 需要基于结点类型判断执行过程
  // 叶结点：
  // 1.确定删除位置后删除数据即可
  // 中间结点:
  // 1.确定执行删除函数的子结点
  // 2.对应的子结点执行删除函数
  // 3.判断子结点是否为满结点，空结点时清除空结点
  // 4.删除空结点情况下需要更新KeyVec和ChildVec

  // ALERT:
  // 由于Insert过程中保证了没用相同的Value值，所以只要成功删除一个结点即可保证删除成功
}

bool NodePage::Update(Field *pKey, const PageSlotID &iOld,
                      const PageSlotID &iNew) {
  // TODO: 需要基于结点类型判断执行过程
  // 叶结点：
  // 1.确定更新位置后更新数据即可
  // 中间结点:
  // 1.确定执行更新函数的子结点
  // 2.对应的子结点执行更新函数

  // ALERT: 由于更新函数不改变结点内存储的容量，所以不需要结构变化
  // ALERT:
  // 由于Insert过程中保证了没用相同的Value值，所以只要成功更新一个结点即可保证更新成功
}

std::vector<PageSlotID> NodePage::Range(Field *pLow, Field *pHigh) {
  // TODO: 需要基于结点类型判断执行过程
  // 叶结点：
  // 1.确定上下界范围，返回这一区间内的所有Value值
  // 中间结点:
  // 1.确定所有可能包含上下界范围的子结点
  // 2.依次对添加各个子结点执行查询函数所得的结果

  // ALERT: 注意叶结点可能为空结点，需要针对这种情况进行特判
}

void NodePage::Clear() {
  // TODO: 需要基于结点类型判断执行过程
  // 叶结点：直接释放占用空间
  // 中间结点：先释放子结点空间，之后释放自身占用空间
}

bool NodePage::Full() const { return _iKeyVec.size() == _nCap; }
bool NodePage::Empty() const { return _iKeyVec.size() == 0; }

FieldType NodePage::GetType() const { return _iKeyType; }

Field *NodePage::FirstKey() const {
  if (Empty()) return nullptr;
  return _iKeyVec[0];
}

std::pair<std::vector<Field *>, std::vector<PageSlotID>> NodePage::PopHalf() {
  std::vector<Field *> iKeyVec{};
  std::vector<PageSlotID> iChildVec{};
  Size mid = _nCap / 2;
  for (auto it = _iKeyVec.begin() + mid; it != _iKeyVec.end();) {
    iKeyVec.push_back(*it);
    it = _iKeyVec.erase(it);
  }
  for (auto it = _iChildVec.begin() + mid; it != _iChildVec.end();) {
    iChildVec.push_back(*it);
    it = _iChildVec.erase(it);
  }
  return {iKeyVec, iChildVec};
}

void NodePage::InitFirst() {
  // TODO:
  // 当初始化一个空的中间结点时，默认为其分配一个空的叶子结点有利于简化实现
  // TODO:
  // 此处需要基于结点Key的类型，初始化一个叶子结点并为KeyVec和ChildVec添加第一个值
}

void NodePage::ResetFirst() {
  // TODO: 当中间结点清空时，为其保留一个空的叶子结点可以简化删除的合并过程
  // TODO: 此处需要基于结点Key的类型，重新设置KeyVec的第一个Key值
}

void NodePage::Load() {
  // TODO: 从格式化页面数据中导入结点信息
  // TODO: 自行设计，注意和Store匹配
}

void NodePage::Store() {
  // TODO: 格式化结点信息并保存到页面中
  // TODO: 自行设计，注意和Load匹配
}

Size NodePage::LowerBound(Field *pField) {
  // TODO: 二分查找找下界，此处给出实现
  // TODO: 边界的理解非常重要，可以自行重新测试一下
  Size nBegin = 0, nEnd = _iKeyVec.size();
  while (nBegin < nEnd) {
    Size nMid = (nBegin + nEnd) / 2;
    if (!Less(_iKeyVec[nMid], pField, _iKeyType)) {
      nEnd = nMid;
    } else {
      nBegin = nMid + 1;
    }
  }
  return nBegin;
}

Size NodePage::UpperBound(Field *pField) {
  // TODO: 二分查找找上界，此处给出实现
  // TODO: 边界的理解非常重要，可以自行重新测试一下
  Size nBegin = 0, nEnd = _iKeyVec.size();
  while (nBegin < nEnd) {
    Size nMid = (nBegin + nEnd) / 2;
    if (Greater(_iKeyVec[nMid], pField, _iKeyType)) {
      nEnd = nMid;
    } else {
      nBegin = nMid + 1;
    }
  }
  return nBegin;
}

Size NodePage::LessBound(Field *pField) {
  if (Empty()) return 0;
  if (Less(pField, _iKeyVec[0], _iKeyType)) return 0;
  return UpperBound(pField) - 1;
}

}  // namespace thdb
