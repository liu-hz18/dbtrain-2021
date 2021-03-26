#ifndef THDB_JOIN_CONDITION_H_
#define THDB_JOIN_CONDITION_H_

#include "condition/condition.h"
#include "defines.h"

namespace thdb {

// 仅用于条件传递，暂时不会涉及
class JoinCondition : public Condition {
 public:
  JoinCondition(const String &sTableA, FieldID nPosA, const String &sTableB,
                FieldID nPosB);
  ~JoinCondition() = default;
  bool Match(const Record &iRecord) const override;
  ConditionType GetType() const override;
  std::pair<String, FieldID> iTableColA;
  std::pair<String, FieldID> iTableColB;
};

}  // namespace thdb

#endif
