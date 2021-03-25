#include "record/record.h"

#include "exception/exceptions.h"

namespace thdb {

Record::Record() { _iFields = std::vector<Field *>(); }

Record::Record(Size nFieldSize) {
  _iFields = std::vector<Field *>();
  for (Size i = 0; i < nFieldSize; ++i) _iFields.push_back(nullptr);
}

Record::~Record() { Clear(); }

Field *Record::GetField(FieldID nPos) const { return _iFields[nPos]; }

void Record::SetField(FieldID nPos, Field *pField) {
  if (_iFields[nPos]) delete _iFields[nPos];
  _iFields[nPos] = pField;
}

Size Record::GetSize() const { return _iFields.size(); }

void Record::Clear() {
  for (const auto &pField : _iFields)
    if (pField) delete pField;
  for (int i = 0; i < _iFields.size(); ++i) _iFields[i] = nullptr;
}

String Record::ToString() {
  String result;
  for (const auto &pField : _iFields) {
    if (pField) {
      result += pField->ToString() + " ";
    } else {
      throw Exception();
    }
  }
  return result;
}

}  // namespace thdb
