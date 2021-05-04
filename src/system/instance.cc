#include "system/instance.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>

#include <vector>
#include <unordered_map>
#include <algorithm>
#include <iostream>

#include "condition/conditions.h"
#include "exception/exceptions.h"
#include "manager/table_manager.h"
#include "page/record_page.h"
#include "record/record.h"
#include "record/fixed_record.h"

using namespace std;

namespace thdb {

Instance::Instance() {
  _pTableManager = new TableManager();
  _pIndexManager = new IndexManager();
}

Instance::~Instance() {
  delete _pTableManager;
  delete _pIndexManager;
}

Table *Instance::GetTable(const String &sTableName) const {
  return _pTableManager->GetTable(sTableName);
}

bool Instance::CreateTable(const String &sTableName, const Schema &iSchema) {
  _pTableManager->AddTable(sTableName, iSchema);
  return true;
}

bool Instance::DropTable(const String &sTableName) {
  for (const auto &sColName : _pIndexManager->GetTableIndexes(sTableName))
    _pIndexManager->DropIndex(sTableName, sColName);
  _pTableManager->DropTable(sTableName);
  return true;
}

FieldID Instance::GetColID(const String &sTableName,
                           const String &sColName) const {
  Table *pTable = GetTable(sTableName);
  if (pTable == nullptr) throw TableException();
  return pTable->GetPos(sColName);
}

FieldType Instance::GetColType(const String &sTableName,
                               const String &sColName) const {
  Table *pTable = GetTable(sTableName);
  if (pTable == nullptr) throw TableException();
  return pTable->GetType(sColName);
}

Size Instance::GetColSize(const String &sTableName,
                          const String &sColName) const {
  Table *pTable = GetTable(sTableName);
  if (pTable == nullptr) throw TableException();
  return pTable->GetSize(sColName);
}

bool CmpPageSlotID(const PageSlotID &iA, const PageSlotID &iB) {
  if (iA.first == iB.first) return iA.second < iB.second;
  return iA.first < iB.first;
}

std::vector<PageSlotID> Intersection(std::vector<PageSlotID> iA,
                                     std::vector<PageSlotID> iB) {
  std::sort(iA.begin(), iA.end(), CmpPageSlotID);
  std::sort(iB.begin(), iB.end(), CmpPageSlotID);
  std::vector<PageSlotID> iRes{};
  std::set_intersection(iA.begin(), iA.end(), iB.begin(), iB.end(),
                        std::back_inserter(iRes));
  return iRes;
}

std::vector<PageSlotID> Instance::Search(
    const String &sTableName, Condition *pCond,
    const std::vector<Condition *> &iIndexCond) {
  Table *pTable = GetTable(sTableName);
  if (pTable == nullptr) throw TableException();
  if (iIndexCond.size() > 0) {
    IndexCondition *pIndexCond = dynamic_cast<IndexCondition *>(iIndexCond[0]);
    assert(pIndexCond != nullptr);
    auto iName = pIndexCond->GetIndexName();
    auto iRange = pIndexCond->GetIndexRange();
    std::vector<PageSlotID> iRes =
        GetIndex(iName.first, iName.second)->Range(iRange.first, iRange.second);
    for (Size i = 1; i < iIndexCond.size(); ++i) {
      IndexCondition *pIndexCond =
          dynamic_cast<IndexCondition *>(iIndexCond[i]);
      auto iName = pIndexCond->GetIndexName();
      auto iRange = pIndexCond->GetIndexRange();
      iRes = Intersection(iRes, GetIndex(iName.first, iName.second)
                                    ->Range(iRange.first, iRange.second));
    }
    return iRes;
  } else
    return pTable->SearchRecord(pCond);
}

PageSlotID Instance::Insert(const String &sTableName,
                            const std::vector<String> &iRawVec) {
  Table *pTable = GetTable(sTableName);
  if (pTable == nullptr) throw TableException();
  Record *pRecord = pTable->EmptyRecord();
  pRecord->Build(iRawVec);
  PageSlotID iPair = pTable->InsertRecord(pRecord);
  // Handle Insert on Index
  if (_pIndexManager->HasIndex(sTableName)) {
    auto iColNames = _pIndexManager->GetTableIndexes(sTableName);
    for (const auto &sCol : iColNames) {
      FieldID nPos = pTable->GetPos(sCol);
      Field *pKey = pRecord->GetField(nPos);
      _pIndexManager->GetIndex(sTableName, sCol)->Insert(pKey, iPair);
    }
  }

  delete pRecord;
  return iPair;
}

uint32_t Instance::Delete(const String &sTableName, Condition *pCond,
                          const std::vector<Condition *> &iIndexCond) {
  auto iResVec = Search(sTableName, pCond, iIndexCond);
  Table *pTable = GetTable(sTableName);
  bool bHasIndex = _pIndexManager->HasIndex(sTableName);
  for (const auto &iPair : iResVec) {
    // Handle Delete on Index
    if (bHasIndex) {
      Record *pRecord = pTable->GetRecord(iPair.first, iPair.second);
      auto iColNames = _pIndexManager->GetTableIndexes(sTableName);
      for (const auto &sCol : iColNames) {
        FieldID nPos = pTable->GetPos(sCol);
        Field *pKey = pRecord->GetField(nPos);
        _pIndexManager->GetIndex(sTableName, sCol)->Delete(pKey, iPair);
      }
      delete pRecord;
    }

    pTable->DeleteRecord(iPair.first, iPair.second);
  }
  return iResVec.size();
}

uint32_t Instance::Update(const String &sTableName, Condition *pCond,
                          const std::vector<Condition *> &iIndexCond,
                          const std::vector<Transform> &iTrans) {
  auto iResVec = Search(sTableName, pCond, iIndexCond);
  Table *pTable = GetTable(sTableName);
  bool bHasIndex = _pIndexManager->HasIndex(sTableName);
  for (const auto &iPair : iResVec) {
    // Handle Delete on Index
    if (bHasIndex) {
      Record *pRecord = pTable->GetRecord(iPair.first, iPair.second);
      auto iColNames = _pIndexManager->GetTableIndexes(sTableName);
      for (const auto &sCol : iColNames) {
        FieldID nPos = pTable->GetPos(sCol);
        Field *pKey = pRecord->GetField(nPos);
        _pIndexManager->GetIndex(sTableName, sCol)->Delete(pKey, iPair);
      }
      delete pRecord;
    }

    pTable->UpdateRecord(iPair.first, iPair.second, iTrans);

    // Handle Delete on Index
    if (bHasIndex) {
      Record *pRecord = pTable->GetRecord(iPair.first, iPair.second);
      auto iColNames = _pIndexManager->GetTableIndexes(sTableName);
      for (const auto &sCol : iColNames) {
        FieldID nPos = pTable->GetPos(sCol);
        Field *pKey = pRecord->GetField(nPos);
        _pIndexManager->GetIndex(sTableName, sCol)->Insert(pKey, iPair);
      }
      delete pRecord;
    }
  }
  return iResVec.size();
}

Record *Instance::GetRecord(const String &sTableName,
                            const PageSlotID &iPair) const {
  Table *pTable = GetTable(sTableName);
  return pTable->GetRecord(iPair.first, iPair.second);
}

std::vector<Record *> Instance::GetTableInfos(const String &sTableName) const {
  std::vector<Record *> iVec{};
  for (const auto &sName : GetColumnNames(sTableName)) {
    FixedRecord *pDesc = new FixedRecord(
        3,
        {FieldType::STRING_TYPE, FieldType::STRING_TYPE, FieldType::INT_TYPE},
        {COLUMN_NAME_SIZE, 10, 4});
    pDesc->SetField(0, new StringField(sName));
    pDesc->SetField(1,
                    new StringField(toString(GetColType(sTableName, sName))));
    pDesc->SetField(2, new IntField(GetColSize(sTableName, sName)));
    iVec.push_back(pDesc);
  }
  return iVec;
}
std::vector<String> Instance::GetTableNames() const {
  return _pTableManager->GetTableNames();
}
std::vector<String> Instance::GetColumnNames(const String &sTableName) const {
  return _pTableManager->GetColumnNames(sTableName);
}

bool Instance::IsIndex(const String &sTableName, const String &sColName) const {
  return _pIndexManager->IsIndex(sTableName, sColName);
}

Index *Instance::GetIndex(const String &sTableName,
                          const String &sColName) const {
  return _pIndexManager->GetIndex(sTableName, sColName);
}

std::vector<Record *> Instance::GetIndexInfos() const {
  std::vector<Record *> iVec{};
  for (const auto &iPair : _pIndexManager->GetIndexInfos()) {
    FixedRecord *pInfo =
        new FixedRecord(4,
                        {FieldType::STRING_TYPE, FieldType::STRING_TYPE,
                         FieldType::STRING_TYPE, FieldType::INT_TYPE},
                        {TABLE_NAME_SIZE, COLUMN_NAME_SIZE, 10, 4});
    pInfo->SetField(0, new StringField(iPair.first));
    pInfo->SetField(1, new StringField(iPair.second));
    pInfo->SetField(
        2, new StringField(toString(GetColType(iPair.first, iPair.second))));
    pInfo->SetField(3, new IntField(GetColSize(iPair.first, iPair.second)));
    iVec.push_back(pInfo);
  }
  return iVec;
}

bool Instance::CreateIndex(const String &sTableName, const String &sColName,
                           FieldType iType) {
  auto iAll = Search(sTableName, nullptr, {});
  _pIndexManager->AddIndex(sTableName, sColName, iType);
  Table *pTable = GetTable(sTableName);
  // Handle Exists Data
  for (const auto &iPair : iAll) {
    FieldID nPos = pTable->GetPos(sColName);
    Record *pRecord = pTable->GetRecord(iPair.first, iPair.second);
    Field *pKey = pRecord->GetField(nPos);
    _pIndexManager->GetIndex(sTableName, sColName)->Insert(pKey, iPair);
    delete pRecord;
  }
  return true;
}

bool Instance::DropIndex(const String &sTableName, const String &sColName) {
  auto iAll = Search(sTableName, nullptr, {});
  Table *pTable = GetTable(sTableName);
  for (const auto &iPair : iAll) {
    FieldID nPos = pTable->GetPos(sColName);
    Record *pRecord = pTable->GetRecord(iPair.first, iPair.second);
    Field *pKey = pRecord->GetField(nPos);
    _pIndexManager->GetIndex(sTableName, sColName)->Delete(pKey, iPair);
    delete pRecord;
  }
  _pIndexManager->DropIndex(sTableName, sColName);
  return true;
}

std::pair<std::vector<String>, std::vector<Record *>> Instance::Join(
    std::map<String, std::vector<PageSlotID>> &iResultMap,
    std::vector<Condition *> &iJoinConds) {
  // LAB3 BEGIN
  // TODO:实现正确且高效的表之间JOIN过程

  // ALERT:由于实现临时表存储具有一定难度，所以允许JOIN过程中将中间结果保留在内存中，不需要存入临时表
  // ALERT:一定要注意，存在JOIN字段值相同的情况，需要特别重视
  // ALERT:针对于不同的JOIN情况（此处只需要考虑数据量和是否为索引列），可以选择使用不同的JOIN算法
  // ALERT:JOIN前已经经过了Filter过程
  // ALERT:建议不要使用不经过优化的NestedLoopJoin算法

  // TIPS:JoinCondition中保存了JOIN两方的表名和列名
  // TIPS:利用GetTable(TableName)的方式可以获得Table*指针，之后利用lab1中的Table::GetRecord获得初始Record*数据
  // TIPs:利用Table::GetColumnNames可以获得Table初始的列名，与初始Record*顺序一致
  // TIPS:Record对象添加了Copy,Sub,Add,Remove函数，方便同学们对于Record进行处理
  // TIPS:利用GetColID/Type/Size三个函数可以基于表名和列名获得列的信息
  // TIPS:利用IsIndex可以判断列是否存在索引
  // TIPS:利用GetIndex可以获得索引Index*指针

  // EXTRA:JOIN的表的数量超过2时，所以需要先计算一个JOIN执行计划（不要求复杂算法）,有兴趣的同学可以自行实现
  // EXTRA:在多表JOIN时，可以采用并查集或执行树来确定执行JOIN的数据内容

  // implemented Sort-Merge Algorithm.
  // 初始化两个表的Record*向量
  // cout << "begin" << endl;
  const String tableName1 = iResultMap.cbegin()->first;
  std::vector<PageSlotID> pageslots1 = iResultMap.cbegin()->second;
  Table *pTable1 = GetTable(tableName1);
  std::vector<String> columnNames1 = pTable1->GetColumnNames();
  std::vector<Record *> records1;
  // record page cache
  RecordPage* last_record_page = nullptr;
  PageID last_page_id = -1;
  for (const auto& pageslot: pageslots1) {
    Record* fixed_record = pTable1->EmptyRecord();
    uint8_t* raw_slot_data;
    if (pageslot.first != last_page_id) {
      if (last_record_page != nullptr) delete last_record_page;
      last_record_page = new RecordPage(pageslot.first);
      last_page_id = pageslot.first;
    }
    raw_slot_data = last_record_page->GetRecord(pageslot.second);
    fixed_record->Load(raw_slot_data);
    delete[] raw_slot_data;
    records1.emplace_back(fixed_record);
  }

  const String tableName2 = iResultMap.crbegin()->first;
  std::vector<PageSlotID> pageslots2 = iResultMap.crbegin()->second;
  Table *pTable2 = GetTable(tableName2);
  std::vector<String> columnNames2 = pTable2->GetColumnNames();
  std::vector<Record *> records2;
  // record page cache
  last_record_page = nullptr;
  last_page_id = -1;
  for (const auto& pageslot: pageslots2) {
    Record* fixed_record = pTable2->EmptyRecord();
    uint8_t* raw_slot_data;
    if (pageslot.first != last_page_id) {
      if (last_record_page != nullptr) delete last_record_page;
      last_record_page = new RecordPage(pageslot.first);
      last_page_id = pageslot.first;
    }
    raw_slot_data = last_record_page->GetRecord(pageslot.second);
    fixed_record->Load(raw_slot_data);
    delete[] raw_slot_data;
    records2.emplace_back(fixed_record);
  }

  // get join condition
  JoinCondition* condition = dynamic_cast<JoinCondition*>(iJoinConds[0]); // we only select one for simplification.
  assert(condition != nullptr);
  String joinCol1, joinCol2;
  if (condition->sTableA == tableName1) {
    joinCol1 = condition->sColA; joinCol2 = condition->sColB;
  } else {
    joinCol1 = condition->sColB; joinCol2 = condition->sColA;
  }
  uint32_t joinColRank1 = pTable1->GetPos(joinCol1);
  uint32_t joinColRank2 = pTable2->GetPos(joinCol2);
  FieldType type = pTable1->GetType(joinCol1);
  assert(type == pTable2->GetType(joinCol2));

  std::vector<Record *> resultRecords;
  std::vector<String> resultColNames = columnNames1;
  resultColNames.insert(resultColNames.end(), columnNames2.begin(), columnNames2.end());

  // use dynamic join algo.
  if (pageslots1.size() < 1024 && pageslots2.size() < 1024) { // use sort merge join, O(m + n + mlogm + nlogn)
    // sort
    sort(records1.begin(), records1.end(), [=](Record* left, Record* right){
      return Less(left->_iFields[joinColRank1], right->_iFields[joinColRank1], type);
    });
    sort(records2.begin(), records2.end(), [=](Record* left, Record* right){
      return Less(left->_iFields[joinColRank2], right->_iFields[joinColRank2], type);
    });
    // merge
    size_t last_j = 0;
    for (size_t i = 0; i < records1.size(); ++i) {
      size_t j;
      for (j = last_j; j < records2.size(); ++j) {
        if (Equal(records1[i]->_iFields[joinColRank1], records2[j]->_iFields[joinColRank2], type)) {
          Record* hitRecord = records1[i]->Copy();
          hitRecord->Add(records2[j]);
          resultRecords.emplace_back(hitRecord);
        } else {
          break;
        }
      }
      if (i < records1.size()-1 && !Equal(records1[i]->_iFields[joinColRank1], records1[i+1]->_iFields[joinColRank1], type)) {
        last_j = j;
      }
    }
  } else { // use hash join O(m+n)
    // hash O(m)
    std::unordered_map<int, std::vector<Record*>> hashmap;
    for (auto* record: records1) {
      IntField* field = dynamic_cast<IntField*>(record->_iFields[joinColRank1]);
      if (hashmap.count(field->GetIntData()) > 0) {
        hashmap[field->GetIntData()].emplace_back(record);
      } else {
        hashmap.insert({field->GetIntData(), std::vector<Record*>{record}});
      }
    }
    // iteration, O(n)
    for (auto* record: records2) {
      IntField* field = dynamic_cast<IntField*>(record->_iFields[joinColRank2]);
      if (hashmap.count(field->GetIntData()) > 0) {
        for (auto* record1: hashmap[field->GetIntData()]) {
          Record* hitRecord = record1->Copy();
          hitRecord->Add(record);
          resultRecords.emplace_back(hitRecord);
        }
      }
    }
  }

  for (size_t i = 0; i < records1.size(); ++i) {
    delete records1[i];
  }
  for (size_t i = 0; i < records2.size(); ++i) {
    delete records2[i];
  }

  return std::pair<std::vector<String>, std::vector<Record*> >(resultColNames, resultRecords);
  // LAB3 END
}

}  // namespace thdb
