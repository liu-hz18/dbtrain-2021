#ifndef THDB_LOG_PAGE_H_
#define THDB_LOG_PAGE_H_

#include "page/linked_page.h"
#include "macros.h"

namespace thdb {

class LogLine {
public:
    LogLine() {
        _tableID = 0;
        _txnID = 0;
        _logOp = LogOperation::UNDEFINED;
        _location = {0, 0};
    }
    LogLine(PageID tableID, TxnID txnID, LogOperation logOp, PageSlotID location) {
        _tableID = tableID;
        _txnID = txnID;
        _logOp = logOp;
        _location = location;
    }
    PageID _tableID;
    TxnID _txnID;
    LogOperation _logOp;
    PageSlotID _location;
};


class LogPage: public LinkedPage {
public:
    LogPage();
    LogPage(PageID nPageID);
    ~LogPage();
    void log(LogLine logline);
    std::vector<LogLine> getLogs() const;
    bool Full() const;

private:
    Size _nCap;
    PageOffset _nFixed;
    PageOffset _nextLogLineID;

};

} // namespace thdb

#endif
