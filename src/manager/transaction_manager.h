#ifndef TRANSACTION_MANAGER_H_
#define TRANSACTION_MANAGER_H_

#include <unordered_map>

#include "defines.h"
#include "transaction/transaction.h"

namespace thdb {

class TransactionManager {
public:
  TransactionManager() = default;
  ~TransactionManager() = default;

  Transaction *Begin();
  void Commit(Transaction *txn);
  void Abort(Transaction *txn);

private:
  std::vector<TxnID> activeTxns;
  TxnID nextTxnID = 0;

};

}  // namespace thdb

#endif  // TRANSACTION_MANAGER_H_
