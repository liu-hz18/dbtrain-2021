#include <vector>
#include <algorithm>
#include "manager/recovery_manager.h"
#include "page/log_page.h"
#include "table/table.h"

namespace thdb {

void RecoveryManager::setLogPageID(PageID logPageID) {
    _logPageID = logPageID;
};

void RecoveryManager::Redo() {
    LogPage* page = new LogPage(_logPageID);
    std::vector<LogLine> logs = page->getLogs();
    delete page;
    // 遍历logs得到redo
    std::vector<TxnID> commited;
    int length = logs.size();
    for (int i = 0; i < length; i++) {
        if (logs[i]._logOp == LogOperation::COMMIT) {
            commited.push_back(logs[i]._txnID);
        }
    }
    std::vector<LogLine> redos;
    for (int i = length-1; i >= 0; i--) {
        if (find(commited.begin(), commited.end(), logs[i]._txnID) != commited.end()) {
            redos.push_back(logs[i]);
        }
    }
    // redo, actually do nothing
}

void RecoveryManager::Undo() {
    LogPage* page = new LogPage(_logPageID);
    std::vector<LogLine> logs = page->getLogs();
    delete page;
    // 遍历logs得到undo
    std::vector<TxnID> commited;
    int length = logs.size();
    for (int i = 0; i < length; i++) {
        if (logs[i]._logOp == LogOperation::COMMIT) {
            commited.push_back(logs[i]._txnID);
        }
    }
    std::vector<LogLine> undos;
    for (int i = length-1; i >= 0; i--) {
        if (find(commited.begin(), commited.end(), logs[i]._txnID) == commited.end()) {
            undos.push_back(logs[i]);
        }
    }
    // undo, delete
    for (auto log: undos) {
        Table* pTable = new Table(log._tableID);
        pTable->DeleteRecord(log._location.first, log._location.second);
    }
}

}  // namespace thdb
