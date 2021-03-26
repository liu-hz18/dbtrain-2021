#ifndef THDB_TRANSFORM_H_
#define THDB_TRANSFORM_H_

#include "defines.h"
#include "field/fields.h"

namespace thdb {

// 把 sRaw 从 String 类型转换为 iType 类型的 Field
class Transform {
 public:
  Transform(FieldID nFieldID, FieldType iType, const String &sRaw);
  ~Transform() = default;

  Field *GetField() const;
  FieldID GetPos() const;

 private:
  FieldID _nFieldID;
  FieldType _iType;
  String _sRaw;
};

bool operator==(const Transform &a, const Transform &b);

bool operator<(const Transform &a, const Transform &b);

bool operator<=(const Transform &a, const Transform &b);

bool operator>(const Transform &a, const Transform &b);

bool operator>=(const Transform &a, const Transform &b);

bool operator!=(const Transform &a, const Transform &b);

}  // namespace thdb

#endif
