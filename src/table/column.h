#ifndef THDB_COLUMN_H_
#define THDB_COLUMN_H_

#include "defines.h"
#include "field/field.h"

namespace thdb {

// Table中的一列，只存储元信息，包括：列名、数据类型、单个数据的大小
class Column {
 public:
  Column(const String &sName, FieldType iType);
  Column(const String &sName, FieldType iType, Size nSize);
  ~Column() = default;

  String GetName() const;
  FieldType GetType() const;
  Size GetSize() const;

 private:
  String _sName;
  FieldType _iType;
  Size _nSize; // 单个数据的大小
};

}  // namespace thdb

#endif