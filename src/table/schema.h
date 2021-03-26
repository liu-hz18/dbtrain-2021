#ifndef THDB_SCHEMA_H_
#define THDB_SCHEMA_H_

#include "defines.h"
#include "table/column.h"

namespace thdb {

// 模式: 是数据库的组织和结构. 它显示了数据库对象及其相互之间的关系
// 可以是表(table)、列(column)、数据类型(data type)、视图(view)、存储过程(stored procedures)

// 在这里就是 std::vector<Column> 的封装
class Schema {
 public:
  Schema(const std::vector<Column> &iColVec);
  ~Schema() = default;

  Size GetSize() const;
  Column GetColumn(Size nPos) const;

 private:
  std::vector<Column> _iColVec;
};

}  // namespace thdb

#endif