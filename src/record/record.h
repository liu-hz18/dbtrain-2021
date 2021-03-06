#ifndef THDB_RECORD_H_
#define THDB_RECORD_H_

#include "defines.h"
#include "field/field.h"

namespace thdb {
class Record {
 public:
  Record();
  Record(Size nFieldSize);
  virtual ~Record();

  Field *GetField(FieldID nPos) const;
  void SetField(FieldID nPos, Field *pField);
  Size GetSize() const;

  virtual Size Load(const uint8_t *src) = 0;
  virtual Size Store(uint8_t *dst) const = 0;
  virtual void Build(const std::vector<String> &iRawVec) = 0;

  void Clear();
  String ToString();

 protected:
  std::vector<Field *> _iFields;
};

}  // namespace thdb

#endif
