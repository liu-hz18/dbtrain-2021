#include "record_page.h"

#include <cassert>
#include <cstring>

#include "exception/exceptions.h"
#include "macros.h"

namespace thdb {

const PageOffset FIXED_SIZE_OFFSET = 12;
const PageOffset BITMAP_OFFSET = 0;
const PageOffset BITMAP_SIZE = 128; // bitmap 占 128 * 8 = 1024 bit, 可以表示1024个slot
// BITMAP_SIZE 之后才是 _nCap 个 _nFixed 大小的 slot

// 先调用父类 LinkedPage 构造函数，向页面header写入链表中 下一个 和 上一个
// 的PageID。然后向header中写入_nFixed数据，表示 表示支持的定长记录长度
// bool参数只是为了重载
RecordPage::RecordPage(PageOffset nFixed, bool) : LinkedPage() {
  _nFixed = nFixed;
  _pUsed = new Bitmap((DATA_SIZE - BITMAP_SIZE) / nFixed);
  _nCap = (DATA_SIZE - BITMAP_SIZE) / _nFixed;  // 表示页面能容纳的记录数量
  SetHeader((uint8_t *)&_nFixed, 2, FIXED_SIZE_OFFSET);
}

RecordPage::RecordPage(PageID nPageID) : LinkedPage(nPageID) {
  GetHeader((uint8_t *)&_nFixed, 2, FIXED_SIZE_OFFSET);
  _pUsed = new Bitmap((DATA_SIZE - BITMAP_SIZE) / _nFixed);
  _nCap = (DATA_SIZE - BITMAP_SIZE) / _nFixed; // 注意bitmap不维护其自身的占用状态
  LoadBitmap();
}

// 之后会调用父类析构函数，保存header元数据
RecordPage::~RecordPage() { StoreBitmap(); }

void RecordPage::LoadBitmap() {
  uint8_t pTemp[BITMAP_SIZE];
  memset(pTemp, 0, BITMAP_SIZE);
  // Bitmap序列化后的内容，存在Page的data段，data段内偏移为0（也就是起始）
  GetData(pTemp, BITMAP_SIZE, BITMAP_OFFSET);
  _pUsed->Load(pTemp); // 反序列化
}

void RecordPage::StoreBitmap() {
  uint8_t pTemp[BITMAP_SIZE];
  memset(pTemp, 0, BITMAP_SIZE);
  _pUsed->Store(pTemp);  // 序列化到uint8_t*
  SetData(pTemp, BITMAP_SIZE, BITMAP_OFFSET); // 写入页面的data段
  delete _pUsed;
}

// 表示页面能容纳的记录数量
Size RecordPage::GetCap() const { return _nCap; }

Size RecordPage::GetUsed() const { return _pUsed->GetUsed(); }

bool RecordPage::Full() const { return _pUsed->Full(); }

PageOffset RecordPage::GetFixedSize() const { return _nFixed; }

void RecordPage::Clear() {
  for (SlotID i = 0; i < _nCap; ++i)
    if (HasRecord(i)) DeleteRecord(i);
}

SlotID RecordPage::InsertRecord(const uint8_t *src) {
  // LAB1 BEGIN
  // 寻找空的槽位，插入数据
  // TIPS: 合理抛出异常的方式可以帮助DEBUG工作
  // TIPS: 利用_pUsed位图判断槽位是否使用，插入后需要更新_pUsed
  // TIPS: 使用SetData实现写数据
  // 寻找空的槽位
  SlotID nSlotID = -1;
  for (SlotID i = 0; i < _nCap; ++i) {
    if (!HasRecord(i)) {
      nSlotID = i;
      break;
    }
  }
  // 找到了一个空槽位
  if (nSlotID < _nCap) {
    SetData(src, _nFixed, BITMAP_OFFSET + BITMAP_SIZE + nSlotID * _nFixed);
    _pUsed->Set(nSlotID);
  }
  return nSlotID;
  // LAB1 END
}

uint8_t *RecordPage::GetRecord(SlotID nSlotID) {
  // LAB1 BEGIN
  // 获得nSlotID槽位置的数据
  // TIPS: 使用GetData实现读数据
  // TIPS: 注意需要使用new分配_nFixed大小的空间
  // 判断是否已经有page
  if (!_pUsed->Get(nSlotID)) throw RecordPageException(nSlotID);
  uint8_t* dst = new uint8_t[_nFixed];
  GetData(dst, _nFixed, BITMAP_OFFSET + BITMAP_SIZE + nSlotID * _nFixed);
  return dst;
  // LAB1 END
}

bool RecordPage::HasRecord(SlotID nSlotID) { return _pUsed->Get(nSlotID); }

void RecordPage::DeleteRecord(SlotID nSlotID) {
  // LAB1 BEGIN
  // 删除一条记录
  // TIPS: 需要设置_pUsed
  if (!_pUsed->Get(nSlotID)) throw RecordPageException(nSlotID);
  _pUsed->Unset(nSlotID);
  this->_bModified = true;
  // LAB1 END
}

void RecordPage::UpdateRecord(SlotID nSlotID, const uint8_t *src) {
  if (!_pUsed->Get(nSlotID)) throw RecordPageException(nSlotID);
  SetData(src, _nFixed, BITMAP_OFFSET + BITMAP_SIZE + nSlotID * _nFixed);
}

}  // namespace thdb
