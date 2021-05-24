#ifndef RECOVERY_MANAGER_H_
#define RECOVERY_MANAGER_H_

#include "defines.h"

namespace thdb {

class RecoveryManager {
 public:
  RecoveryManager() = default;
  ~RecoveryManager() = default;

  void Redo();
  void Undo();
  void setLogPageID(PageID);

 private:
  PageID _logPageID;
};

}  // namespace thdb

#endif  // RECOVERY_MANAGER_H_
