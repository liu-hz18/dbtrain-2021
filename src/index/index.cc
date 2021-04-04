#include "index/index.h"

#include "page/node_page.h"

namespace thdb {
Index::Index(FieldType iType) {
  // TODO: 建立一个新的根结点，注意需要基于类型判断根结点的属性
  // TODO: 根结点需要设为中间结点
  // TODO: 注意记录RootID
}

Index::Index(PageID nPageID) {
  // TODO: 记录RootID即可
}

Index::~Index() {
  // TODO: 如果不添加额外的指针，理论上不用额外写回内容
}

void Index::Clear() {
  // TODO: 利用RootID获得根结点
  // TODO: 利用根结点的Clear函数清除全部索引占用页面
}

PageID Index::GetRootID() const { return _nRootID; }

bool Index::Insert(Field *pKey, const PageSlotID &iPair) {
  // TODO: 利用RootID获得根结点
  // TODO: 利用根结点的Insert执行插入
  // TODO: 根结点满时，需要进行分裂操作，同时更新RootID
}

Size Index::Delete(Field *pKey) {
  // ALERT: 结点合并实现难度较高，不作为必要要求
  // TODO: 利用RootID获得根结点
  // TODO: 利用根结点的Delete执行删除
}

bool Index::Delete(Field *pKey, const PageSlotID &iPair) {
  // ALERT: 结点合并实现难度较高，不作为必要要求
  // TODO: 利用RootID获得根结点
  // TODO: 利用根结点的Delete执行删除
}

bool Index::Update(Field *pKey, const PageSlotID &iOld,
                   const PageSlotID &iNew) {
  // TODO: 利用RootID获得根结点
  // TODO: 利用根结点的Update执行删除
}

std::vector<PageSlotID> Index::Range(Field *pLow, Field *pHigh) {
  // TODO: 利用RootID获得根结点
  // TODO: 利用根结点的Range执行范围查找
}

}  // namespace thdb
