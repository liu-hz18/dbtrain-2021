#include "transaction/transaction.h"

namespace thdb {

Transaction::Transaction(TxnID txn_id) : txn_id_(txn_id) {}

}  // namespace thdb
