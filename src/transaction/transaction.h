#ifndef TRANSACTION_H_
#define TRANSACTION_H_

#include "defines.h"

namespace thdb {

class Transaction {
 public:
  explicit Transaction(TxnID txn_id);
  ~Transaction() = default;

  TxnID GetTxnID() const;
  void setActiveTxns(const std::vector<TxnID> iActiveTxns);
  bool visible(TxnID txnID) const;
  void recordInsert(const PageSlotID iPair);
  void setTableID(const PageID tableID);
  PageID getTableID() const;

  std::vector<PageSlotID> insertHistory;

 private:
  TxnID txn_id_;
  std::vector<TxnID> activeTxns;
  PageID nTableID;
  
};

}  // namespace thdb

#endif  // TRANSACTION_H_
