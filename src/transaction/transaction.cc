#include "transaction/transaction.h"
#include <algorithm>

namespace thdb {

Transaction::Transaction(TxnID txn_id) : txn_id_(txn_id) {}

TxnID Transaction::GetTxnID() const { return txn_id_; }

void Transaction::setActiveTxns(const std::vector<TxnID> iActiveTxns) {
    activeTxns = iActiveTxns;
}

bool Transaction::visible(TxnID txnID) const {
    return (txnID == txn_id_) || (txnID < txn_id_ && find(activeTxns.begin(), activeTxns.end(), txnID) == activeTxns.end());
}

void Transaction::recordInsert(const PageSlotID iPair) {
    insertHistory.push_back(iPair);
}

void Transaction::setTableID(const PageID tableID) {
    nTableID = tableID;
}

PageID Transaction::getTableID() const { return nTableID; }

}  // namespace thdb
