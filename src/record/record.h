#ifndef THDB_RECORD_H_
#define THDB_RECORD_H_

#include "defines.h"
#include "field/field.h"

// 表示一条记录的抽象类
// 用于实现记录序列化和反序列化工作
namespace thdb {
class Record {
 public:
  Record();
  Record(Size nFieldSize);
  virtual ~Record();

  Field *GetField(FieldID nPos) const;
  void SetField(FieldID nPos, Field *pField);
  /**
   * @brief 获得记录中字段数量
   * @return Size 记录中字段的数量
   */
  Size GetSize() const;

  virtual Size Load(const uint8_t *src) = 0;
  virtual Size Store(uint8_t *dst) const = 0;
  virtual void Build(const std::vector<String> &iRawVec) = 0;

  void Clear(); // _iFields 置 nullptr
  String ToString();

 protected:
  std::vector<Field *> _iFields; // 各个字段的类型
};

}  // namespace thdb

#endif
