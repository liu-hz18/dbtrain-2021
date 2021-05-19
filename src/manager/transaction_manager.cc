#include "manager/transaction_manager.h"
#include <algorithm>
#include <cassert>

#include "table/table.h"

namespace thdb {

Transaction *TransactionManager::Begin() {
    Transaction* txn = new Transaction(nextTxnID);
    activeTxns.push_back(nextTxnID);
    nextTxnID++;
    txn->setActiveTxns(activeTxns);
    return txn; 
}

void TransactionManager::Commit(Transaction *txn) { 
    auto _iter = find(activeTxns.begin(), activeTxns.end(), txn->GetTxnID());
    assert(_iter != activeTxns.end());
    activeTxns.erase(_iter);
}

void TransactionManager::Abort(Transaction *txn) { 
    auto _iter = find(activeTxns.begin(), activeTxns.end(), txn->GetTxnID());
    assert(_iter != activeTxns.end());
    activeTxns.erase(_iter);
    size_t insertNumber = txn->insertHistory.size();
    Table* pTable = new Table(txn->getTableID());
    for (int i = insertNumber-1; i >= 0; i--) {
        // TODO: get pTable
        auto iPair = txn->insertHistory[i];
        pTable->DeleteRecord(iPair.first, iPair.second);
    }
    delete pTable;
    txn->insertHistory.clear();
}

}  // namespace thdb
