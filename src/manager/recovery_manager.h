#ifndef RECOVERY_MANAGER_H_
#define RECOVERY_MANAGER_H_

namespace thdb {

class RecoveryManager {
 public:
  RecoveryManager() = default;
  ~RecoveryManager() = default;

  void Redo();
  void Undo();

 private:
};

}  // namespace thdb

#endif  // RECOVERY_MANAGER_H_
