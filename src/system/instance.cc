#include "system/instance.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>

#include "exception/exceptions.h"
#include "manager/table_manager.h"
#include "record/fixed_record.h"

namespace thdb {

// 创建单个数据库实例
Instance::Instance() { _pTableManager = new TableManager(); }

Instance::~Instance() { delete _pTableManager; }

Table *Instance::GetTable(const String &sTableName) const {
  return _pTableManager->GetTable(sTableName);
}

bool Instance::CreateTable(const String &sTableName, const Schema &iSchema) {
  _pTableManager->AddTable(sTableName, iSchema);
  return true;
}

bool Instance::DropTable(const String &sTableName) {
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

std::vector<PageSlotID> Instance::Search(const String &sTableName,
                                         Condition *pCond) {
  Table *pTable = GetTable(sTableName);
  if (pTable == nullptr) throw TableException();
  return pTable->SearchRecord(pCond);
}

PageSlotID Instance::Insert(const String &sTableName,
                            const std::vector<String> &iRawVec) {
  Table *pTable = GetTable(sTableName);
  if (pTable == nullptr) throw TableException();
  Record *pRecord = pTable->EmptyRecord();
  pRecord->Build(iRawVec);
  PageSlotID iPair = pTable->InsertRecord(pRecord);
  delete pRecord;
  return iPair;
}

uint32_t Instance::Delete(const String &sTableName, Condition *pCond) {
  auto iResVec = Search(sTableName, pCond);
  Table *pTable = GetTable(sTableName);
  for (const auto &iPair : iResVec)
    pTable->DeleteRecord(iPair.first, iPair.second);
  return iResVec.size();
}

uint32_t Instance::Update(const String &sTableName, Condition *pCond,
                          const std::vector<Transform> &iTrans) {
  auto iResVec = Search(sTableName, pCond);
  Table *pTable = GetTable(sTableName);
  for (const auto &iPair : iResVec)
    pTable->UpdateRecord(iPair.first, iPair.second, iTrans);
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

}  // namespace thdb
