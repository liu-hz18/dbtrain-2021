#include "manager/transaction_manager.h"

namespace thdb {

Transaction *TransactionManager::Begin() { return nullptr; }

void TransactionManager::Commit(Transaction *txn) { return; }

void TransactionManager::Abort(Transaction *txn) { return; }

}  // namespace thdb
