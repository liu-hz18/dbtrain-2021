#include "system/instance.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <algorithm>
#include <iostream>

#include "condition/conditions.h"
#include "exception/exceptions.h"
#include "manager/table_manager.h"
#include "record/fixed_record.h"

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

}  // namespace thdb
