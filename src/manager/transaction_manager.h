#ifndef TRANSACTION_MANAGER_H_
#define TRANSACTION_MANAGER_H_

#include <unordered_map>

#include "defines.h"
#include "transaction/transaction.h"
#include "page/page.h"
#include "minios/os.h"

namespace thdb {

class TransactionManager {
public:
  TransactionManager();
  ~TransactionManager();
  Transaction *Begin();
  void Commit(Transaction *txn);
  void Abort(Transaction *txn);
  void LogInsert(Transaction *txn, PageSlotID location);
  PageID getLogPageID() const;

private:
  std::vector<TxnID> activeTxns;
  TxnID nextTxnID = 0;
  PageID logPageID;

};

}  // namespace thdb

#endif  // TRANSACTION_MANAGER_H_
