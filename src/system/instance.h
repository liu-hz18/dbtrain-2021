#ifndef THDB_INSTANCE_H_
#define THDB_INSTANCE_H_

#include "defines.h"
#include "field/fields.h"
#include "index/index.h"
#include "manager/index_manager.h"
#include "manager/table_manager.h"
#include "manager/transaction_manager.h"
#include "record/transform.h"
#include "result/results.h"
#include "table/schema.h"

namespace thdb {

class Instance {
 public:
  Instance();
  ~Instance();

  bool CreateTable(const String &sTableName, const Schema &iSchema);
  bool DropTable(const String &sTableName);
  /**
   * @brief 获得列在表中的位置信息
   */
  FieldID GetColID(const String &sTableName, const String &sColName) const;
  /**
   * @brief 获得列的类型
   */
  FieldType GetColType(const String &sTableName, const String &sColName) const;
  /**
   * @brief 获得列的长度
   */
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
  /**
   * @brief 获取一个Table *指针，要求存在表，否则报错
   */
  Table *GetTable(const String &sTableName) const;

  /**
   * @brief 判断列是否为索引列
   */
  bool IsIndex(const String &sTableName, const String &sColName) const;
  /**
   * @brief 获取一个Index*指针，要求存在索引，否则报错
   */
  Index *GetIndex(const String &sTableName, const String &sColName) const;
  std::vector<Record *> GetIndexInfos() const;
  bool CreateIndex(const String &sTableName, const String &sColName,
                   FieldType iType);
  bool DropIndex(const String &sTableName, const String &sColName);

  TransactionManager *GetTransactionManager() const {
    return _pTransactionManager;
  }

  /**
   * @brief 实现多个表的JOIN操作
   *
   * @param iResultMap 表名和对应的Filter过程后PageSlotID结果的Map
   * @param iJoinConds 所有表示Join条件的Condition的Vector
   * @return std::pair<std::vector<String>, std::vector<Record *>>
   * Pair第一项为JOIN结果对应的Table列的列名，列的顺序自行定义；
   * Pair第二项为JOIN结果，结果数据Record*中的字段顺序需要和列名一致。
   */
  std::pair<std::vector<String>, std::vector<Record *>> Join(
      std::map<String, std::vector<PageSlotID>> &iResultMap,
      std::vector<Condition *> &iJoinConds);

 private:
  TableManager *_pTableManager;
  IndexManager *_pIndexManager;
  TransactionManager *_pTransactionManager;
};

}  // namespace thdb

#endif
