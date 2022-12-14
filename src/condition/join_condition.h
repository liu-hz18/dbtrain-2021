#ifndef THDB_JOIN_CONDITION_H_
#define THDB_JOIN_CONDITION_H_

#include "condition/condition.h"
#include "defines.h"

namespace thdb {

// 仅用于条件传递，暂时不会涉及
class JoinCondition : public Condition {
 public:
  JoinCondition(const String &sTableA, const String &sColA,
                const String &sTableB, const String &sColB);
  ~JoinCondition() = default;
  bool Match(const Record &iRecord) const override;
  ConditionType GetType() const override;
  String sTableA, sTableB;
  String sColA, sColB;
};

}  // namespace thdb

#endif
