#ifndef THDB_INSTANCE_H_
#define THDB_INSTANCE_H_

#include "defines.h"
#include "field/fields.h"
#include "index/index.h"
#include "manager/index_manager.h"
#include "manager/table_manager.h"
#include "record/transform.h"
#include "result/results.h"
#include "table/schema.h"

namespace thdb {

// 单个数据库实例
class Instance {
 public:
  Instance();
  ~Instance();

  bool CreateTable(const String &sTableName, const Schema &iSchema);
  bool DropTable(const String &sTableName);
  FieldID GetColID(const String &sTableName, const String &sColName) const;
  FieldType GetColType(const String &sTableName, const String &sColName) const;
  Size GetColSize(const String &sTableName, const String &sColName) const;

  std::vector<PageSlotID> Search(const String &sTableName, Condition *pCond,
                                 const std::vector<Condition *> &iIndexCond);
  uint32_t Delete(const String &sTableName, Condition *pCond,
                  const std::vector<Condition *> &iIndexCond);
  uint32_t Update(const String &sTableName, Condition *pCond,
                  const std::vector<Condition *> &iIndexCond,
                  const std::vector<Transform> &iTrans);
  PageSlotID Insert(const String &sTableName,
                    const std::vector<String> &iRawVec);

  Record *GetRecord(const String &sTableName, const PageSlotID &iPair) const;
  std::vector<Record *> GetTableInfos(const String &sTableName) const;
  std::vector<String> GetTableNames() const;
  std::vector<String> GetColumnNames(const String &sTableName) const;
  Table *GetTable(const String &sTableName) const;

  bool IsIndex(const String &sTableName, const String &sColName) const;
  Index *GetIndex(const String &sTableName, const String &sColName) const;
  std::vector<Record *> GetIndexInfos() const;
  bool CreateIndex(const String &sTableName, const String &sColName,
                   FieldType iType);
  bool DropIndex(const String &sTableName, const String &sColName);

 private:
  TableManager *_pTableManager;
  IndexManager *_pIndexManager;
};

}  // namespace thdb

#endif