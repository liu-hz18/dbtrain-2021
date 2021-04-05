#include "table/table.h"

#include <assert.h>
#include <iostream>
using namespace std;
#include <algorithm>

#include "macros.h"
#include "minios/os.h"
#include "page/record_page.h"
#include "record/fixed_record.h"

namespace thdb {

PageID NextPageID(PageID nCur) {
  // 并不会向OS请求一个页面，只是从已有页面 nCur 中读出元数据存到内存数据结构中
  LinkedPage *pPage = new LinkedPage(nCur);
  PageID nNext = pPage->GetNextID();
  delete pPage;
  return nNext;
}

Table::Table(PageID nTableID) {
  pTable = new TablePage(nTableID); // 不会 向OS请求新的页面

  _nHeadID = pTable->GetHeadID();
  _nTailID = pTable->GetTailID();
  _nNotFull = _nHeadID;
  NextNotFull();
}

Table::~Table() { delete pTable; }

Record *Table::GetRecord(PageID nPageID, SlotID nSlotID) {
  // LAB1 BEGIN
  // 获得一条记录
  // TIPS: 利用 RecordPage::GetRecord 获取无格式记录数据
  // TIPS: 利用 pTable的TablePage::GetFieldSize, GetTypeVec,
  // GetSizeVec三个函数可以构建空的 FixedRecord 对象 TIPS:
  // 利用 Record::Load 导入数据 
  // ALERT: 需要注意析构所有不会返回的内容
  // 1. 新建RecordPage管理对象RecordPage
  RecordPage* record_page = new RecordPage(nPageID);
  // 2. RecordPage::GetRecord获取无格式记录数据
  uint8_t* raw_slot_data = record_page->GetRecord(nSlotID);
  delete record_page;
  // 3. 构建空的FixedRecord对象
  Record* fixed_record = EmptyRecord();
  // 4. fix_record->Load() 从buffer来初始化 fixed_record 的数据成员
  Size size = fixed_record->Load(raw_slot_data);
  // 5. 释放buffer & record_page, 返回 fixed_record
  delete[] raw_slot_data;
  return fixed_record;
  // LAB1 END
}

PageSlotID Table::InsertRecord(Record *pRecord) {
  // LAB1 BEGIN
  // 插入一条记录
  // TIPS: 利用 _nNotFull 来获取有空间的页面
  // TIPS: 利用 Record::Store 获得序列化数据
  // TIPS: 利用 RecordPage::InsertRecord 插入数据
  // TIPS: 注意页满时更新_nNotFull
  RecordPage* record_page = new RecordPage(_nNotFull);
  PageOffset _nFixed = record_page->GetFixedSize();
  uint8_t* raw_slot_data = new uint8_t[_nFixed];
  Size size = pRecord->Store(raw_slot_data);
  assert(size <= _nFixed);
  SlotID nSlotID = record_page->InsertRecord(raw_slot_data);
  PageID nPageID = _nNotFull;
  delete[] raw_slot_data;
  // 页满时更新_nNotFull
  bool full = record_page->Full();
  delete record_page; // 先delete, 强制写回
  if (full) {
    NextNotFull();
  }
  return std::pair<PageID, SlotID>(nPageID, nSlotID);
  // LAB1 END
}

void Table::DeleteRecord(PageID nPageID, SlotID nSlotID) {
  // LAB1 BEGIN
  // TIPS: 利用 RecordPage::DeleteRecord 插入数据
  // TIPS: 注意更新 _nNotFull 来保证较高的页面空间利用效率
  RecordPage* record_page = new RecordPage(nPageID);
  record_page->DeleteRecord(nSlotID);
  _nNotFull = (nPageID < _nNotFull) ? nPageID : _nNotFull;
  delete record_page;
  // LAB1 END
}

void Table::UpdateRecord(PageID nPageID, SlotID nSlotID,
                         const std::vector<Transform> &iTrans) {
  // LAB1 BEGIN
  // TIPS: 仿照InsertRecord从无格式数据导入原始记录
  // TIPS: 构建Record对象，利用Record::SetField更新Record对象
  // TIPS: Trasform::GetPos表示更新位置，GetField表示更新后的字段
  // TIPS: 将新的记录序列化
  // TIPS: 利用RecordPage::UpdateRecord更新一条数据
  // 1. 构建RecordPage来管理页面
  RecordPage* record_page = new RecordPage(nPageID);
  // 2. RecordPage::GetRecord获取无格式记录数据
  uint8_t* raw_slot_data = record_page->GetRecord(nSlotID);
  // 3. 构建空的FixedRecord对象
  Record* fixed_record = EmptyRecord();
  // 4. fix_record->Load() 从buffer来初始化 fixed_record 的数据成员
  Size size = fixed_record->Load(raw_slot_data);
  for (auto transform: iTrans) {
    FieldID field_id = transform.GetPos();
    Field* field = transform.GetField();
    fixed_record->SetField(field_id, field);
  }
  // Record 序列化
  Size _size = fixed_record->Store(raw_slot_data);
  record_page->UpdateRecord(nSlotID, raw_slot_data);
  delete[] raw_slot_data;
  delete fixed_record;
  delete record_page;
  // LAB1 END
}

std::vector<PageSlotID> Table::SearchRecord(Condition *pCond) {
  // LAB1 BEGIN
  // 对记录的条件检索
  // TIPS: 仿照InsertRecord从无格式数据导入原始记录
  // TIPS: 依次导入各条记录进行条件判断
  // TIPS: Condition的抽象方法Match可以判断Record是否满足检索条件
  // TIPS: 返回所有符合条件的结果的pair<PageID,SlotID>
  // loop
  std::vector<PageSlotID> result;
  PageID nBegin = _nHeadID;
  while(nBegin != NULL_PAGE) {
    PageID nPageID = nBegin;
    RecordPage* record_page = new RecordPage(nPageID);
    Size total_slots = record_page->GetCap();
    for (Size i = 0; i < total_slots; ++i) {
      if (record_page->HasRecord(i)) {
        uint8_t* raw_slot_data = record_page->GetRecord(i);
        Record* fixed_record = EmptyRecord();
        Size size = fixed_record->Load(raw_slot_data);
        delete[] raw_slot_data;
        // nullptr, 表示查找该表的所有记录
        if (pCond == nullptr || pCond->Match(*fixed_record)) {
          result.push_back(std::pair<PageID, SlotID>(nPageID, i));
        }
        delete fixed_record;
      }
    }
    delete record_page;
    nBegin = NextPageID(nBegin);
  }
  return result;
  // LAB1 END
}

void Table::SearchRecord(std::vector<PageSlotID> &iPairs, Condition *pCond) {
  if (!pCond) return;
  auto it = iPairs.begin();
  while (it != iPairs.end()) {
    Record *pRecord = GetRecord(it->first, it->second);
    if (!pCond->Match(*pRecord)) {
      it = iPairs.erase(it);
    } else
      ++it;
    delete pRecord;
  }
}

void Table::Clear() {
  PageID nBegin = _nHeadID;
  while (nBegin != NULL_PAGE) {
    PageID nTemp = nBegin;
    nBegin = NextPageID(nBegin);
    MiniOS::GetOS()->DeletePage(nTemp);
  }
}

void Table::NextNotFull() {
  // LAB1 BEGIN
  // 实现一个快速查找非满记录页面的算法
  // ALERT: ！！！一定要注意！！！
  // 不要同时建立两个指向相同磁盘位置的且可变对象，否则会出现一致性问题
  // ALERT: 可以适当增加传入参数，本接口不会被外部函数调用，例如额外传入Page*指针
  // TIPS:
  // 充分利用链表性质，注意全满时需要在结尾_pTable->GetTailID对应结点后插入新的结点，并更新_pTable的TailID
  // TIPS: 只需要保证均摊复杂度较低即可
  PageID nBegin = _nNotFull;
  while(nBegin != NULL_PAGE) {
    PageID nCur = nBegin;
    RecordPage* record_page = new RecordPage(nCur);
    bool full = record_page->Full();
    delete record_page;
    if (full) {
      nBegin = NextPageID(nBegin);
    } else {
      _nNotFull = nCur;
      break;
    }
  }
  // 创建新页面, 加到链表尾部
  if (nBegin == NULL_PAGE) {
    RecordPage* record_page = new RecordPage(pTable->GetTotalSize(), true);
    RecordPage* prev_tail_page = new RecordPage(_nTailID);
    prev_tail_page->PushBack(record_page);
    pTable->SetTailID(record_page->GetPageID());
    _nNotFull = _nTailID = pTable->GetTailID();
    delete prev_tail_page;
    delete record_page;
  }
  // LAB1 END
}

FieldID Table::GetPos(const String &sCol) const { return pTable->GetPos(sCol); }

FieldType Table::GetType(const String &sCol) const {
  return pTable->GetType(sCol);
}

Size Table::GetSize(const String &sCol) const { return pTable->GetSize(sCol); }

Record *Table::EmptyRecord() const {
  FixedRecord *pRecord = new FixedRecord(
      pTable->GetFieldSize(), pTable->GetTypeVec(), pTable->GetSizeVec());
  return pRecord;
}

bool CmpByFieldID(const std::pair<String, FieldID> &a,
                  const std::pair<String, FieldID> &b) {
  return a.second < b.second;
}

std::vector<String> Table::GetColumnNames() const {
  std::vector<String> iVec{};
  std::vector<std::pair<String, FieldID>> iPairVec(pTable->_iColMap.begin(),
                                                   pTable->_iColMap.end());
  std::sort(iPairVec.begin(), iPairVec.end(), CmpByFieldID);
  for (const auto &it : iPairVec) iVec.push_back(it.first);
  return iVec;
}

}  // namespace thdb
