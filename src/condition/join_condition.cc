#include "condition/join_condition.h"

namespace thdb {

JoinCondition::JoinCondition(const String &sTableA, FieldID nPosA,
                             const String &sTableB, FieldID nPosB) {
  iTableColA = {sTableA, nPosA};
  iTableColB = {sTableB, nPosB};
}

bool JoinCondition::Match(const Record &iRecord) const { return true; }

ConditionType JoinCondition::GetType() const {
  return ConditionType::JOIN_TYPE;
}

}  // namespace thdb
