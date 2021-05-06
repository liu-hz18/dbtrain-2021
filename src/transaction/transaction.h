#ifndef TRANSACTION_H_
#define TRANSACTION_H_

#include "defines.h"

namespace thdb {

class Transaction {
 public:
  explicit Transaction(TxnID txn_id);
  ~Transaction() = default;

 private:
  TxnID txn_id_;
};

}  // namespace thdb

#endif  // TRANSACTION_H_
