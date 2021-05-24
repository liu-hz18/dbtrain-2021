#include <algorithm>
#include <cassert>

#include "macros.h"
#include "manager/transaction_manager.h"
#include "table/table.h"
#include "page/log_page.h"

namespace thdb {

const PageOffset NEXT_TXN_ID_OFFSET = 4;
const PageOffset LOG_PAGE_ID_OFFSET = 8;

TransactionManager::TransactionManager() {
    Page* pPage = new Page(TXN_MANAGER_PAGEID);
    pPage->GetData((uint8_t *)&nextTxnID, 4, NEXT_TXN_ID_OFFSET);
    pPage->GetData((uint8_t *)&logPageID, 4, LOG_PAGE_ID_OFFSET);
    delete pPage;
    if (logPageID != 0) pPage = new LogPage(logPageID);
    else pPage = new LogPage();
    logPageID = pPage->GetPageID();
    delete pPage;
};

TransactionManager::~TransactionManager() {
    Page* pPage = new Page(TXN_MANAGER_PAGEID);
    pPage->SetData((uint8_t *)&nextTxnID, 4, NEXT_TXN_ID_OFFSET);
    pPage->SetData((uint8_t *)&logPageID, 4, LOG_PAGE_ID_OFFSET);
    delete pPage;
}

PageID TransactionManager::getLogPageID() const {
    return logPageID;
}

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
    // wirte-ahead log
    LogPage* pPage = new LogPage(logPageID);
    pPage->log(LogLine(txn->getTableID(), txn->GetTxnID(), LogOperation::COMMIT, {0, 0}));
    delete pPage;
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

void TransactionManager::LogInsert(Transaction *txn, PageSlotID location) {
    // wirte-ahead log
    LogPage* pPage = new LogPage(logPageID);
    pPage->log(LogLine(txn->getTableID(), txn->GetTxnID(), LogOperation::INSERT, location));
    delete pPage;
}

}  // namespace thdb
