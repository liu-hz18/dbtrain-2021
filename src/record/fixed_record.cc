#include "record/fixed_record.h"

#include <assert.h>
#include <algorithm>

#include "exception/exceptions.h"
#include "field/fields.h"

namespace thdb {

FixedRecord::FixedRecord(Size nFieldSize,
                         const std::vector<FieldType> &iTypeVec,
                         const std::vector<Size> &iSizeVec)
    : Record(nFieldSize), _iTypeVec(iTypeVec), _iSizeVec(iSizeVec) {
  assert(_iTypeVec.size() == nFieldSize);
  assert(_iSizeVec.size() == nFieldSize);
}

// 定长记录反序列化
Size FixedRecord::Load(const uint8_t *src) {
  Clear();
  // LAB1 BEGIN
  // 反序列化，为定长记录导入各个字段数据
  // TIPS: 利用 Field 的抽象方法 SetData 导入数据
  // TIPS: 基于类型判断构建的指针类型
  // return: 反序列化使用的数据长度
  // 要填充的: _iFields(类型指针向量，指向一个具体的Field数据), iTypeVec(类型向量), iSizeVec(数据类型大小向量)
  // 如果有n个元素，就占用 4 + 4n + 4n + k 个空间
  Size offset = 0;
  for (FieldID i = 0; i < _iFields.size(); ++i) {
    FieldType iType = _iTypeVec[i];
    if (iType == FieldType::INT_TYPE) {
      _iFields[i] = new IntField(src + offset, 4);
      offset += 4;
    } else if (iType == FieldType::FLOAT_TYPE) {
      _iFields[i] = new FloatField(src + offset, 8);
      offset += 8;
    } else if (iType == FieldType::STRING_TYPE) {
      _iFields[i] = new StringField(src + offset, _iSizeVec[i]);
      offset += _iSizeVec[i];
    } else if (iType == FieldType::NONE_TYPE) {
      _iFields[i] = new NoneField();
    } else {
      throw RecordTypeException();
    }
  }
  return offset;
  // LAB1 END
}

// 定长记录序列化
Size FixedRecord::Store(uint8_t *dst) const {
  // LAB1 BEGIN
  // 序列化，将定长数据转化为特定格式
  // TIPS: 利用 Field 的抽象方法 GetData 写出数据
  // TIPS: 基于类型进行 dynamic_cast 进行指针转化
  // return: 序列化使用的数据长度
  Size offset = 0;
  // 存Fields数据
  for (FieldID i = 0; i < _iFields.size(); ++i) {
    FieldType iType = _iTypeVec[i];
    assert(_iFields[i] != nullptr);
    if (iType == FieldType::INT_TYPE) {
      IntField* field = dynamic_cast<IntField *>(_iFields[i]);
      field->GetData(dst + offset, 4);
      offset += 4;
    } else if (iType == FieldType::FLOAT_TYPE) {
      FloatField* field = dynamic_cast<FloatField *>(_iFields[i]);
      field->GetData(dst + offset, 8);
      offset += 8;
    } else if (iType == FieldType::STRING_TYPE) {
      StringField* field = dynamic_cast<StringField *>(_iFields[i]);
      field->GetData(dst + offset, _iSizeVec[i]);
      offset += _iSizeVec[i];
    } else if (iType == FieldType::NONE_TYPE) {
      // do nothing
    } else {
      throw RecordTypeException();
    }
  }
  return offset;
  // LAB1 END
}

void FixedRecord::Build(const std::vector<String> &iRawVec) {
  assert(iRawVec.size() == _iTypeVec.size());
  Clear(); // _iFields 置 nullptr
  for (FieldID i = 0; i < _iFields.size(); ++i) {
    FieldType iType = _iTypeVec[i];
    if (iRawVec[i] == "NULL") {
      SetField(i, new NoneField()); // 置_iFields[i] = Field*
      continue;
    }
    if (iType == FieldType::INT_TYPE) {
      int nVal = std::stoi(iRawVec[i]);
      SetField(i, new IntField(nVal));
    } else if (iType == FieldType::FLOAT_TYPE) {
      double fVal = std::stod(iRawVec[i]);
      SetField(i, new FloatField(fVal));
    } else if (iType == FieldType::STRING_TYPE) {
      SetField(i, new StringField(iRawVec[i].substr(1, iRawVec[i].size() - 2)));
    } else if (iType == FieldType::NONE_TYPE) {
      throw RecordTypeException();
    }
  }
}

Record *FixedRecord::Copy() const {
  Record *pRecord = new FixedRecord(GetSize(), _iTypeVec, _iSizeVec);
  for (Size i = 0; i < GetSize(); ++i)
    pRecord->SetField(i, GetField(i)->Copy());
  return pRecord;
}

void FixedRecord::Sub(const std::vector<Size> &iPos) {
  bool bInSub[GetSize()];
  memset(bInSub, 0, GetSize() * sizeof(bool));
  for (const auto nPos : iPos) bInSub[nPos] = 1;
  auto itField = _iFields.begin();
  auto itType = _iTypeVec.begin();
  auto itSize = _iSizeVec.begin();
  for (Size i = 0; i < GetSize(); ++i) {
    if (!bInSub[i]) {
      Field *pField = *itField;
      if (pField) delete pField;
      itField = _iFields.erase(itField);
      itType = _iTypeVec.erase(itType);
      itSize = _iSizeVec.erase(itSize);
    } else {
      ++itField;
      ++itType;
      ++itSize;
    }
  }
}

void FixedRecord::Add(Record *pRecord) {
  FixedRecord *pFixed = dynamic_cast<FixedRecord *>(pRecord);
  assert(pFixed != nullptr);
  for (Size i = 0; i < pFixed->GetSize(); ++i) {
    _iFields.push_back(pFixed->GetField(i)->Copy());
    _iTypeVec.push_back(pFixed->_iTypeVec[i]);
    _iSizeVec.push_back(pFixed->_iSizeVec[i]);
  }
}

void FixedRecord::Remove(FieldID nPos) {
  Record::Remove(nPos);
  auto itType = _iTypeVec.begin() + nPos;
  auto itSize = _iSizeVec.begin() + nPos;
  _iTypeVec.erase(itType);
  _iSizeVec.erase(itSize);
}

}  // namespace thdb
