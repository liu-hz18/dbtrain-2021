#include <cassert>
#include <vector>
#include "log_page.h"

namespace thdb {

const PageOffset FIXED_SIZE_OFFSET = 12;
const PageOffset NEXT_LOGLINE_OFFSET = 14;
const PageOffset DATA_BEGIN_OFFSET = 0;

LogPage::LogPage(): LinkedPage() {
    // only support INSERT. (txnid, op_type, record)
    _nFixed = 8 + 4 + 4 + 4; // location size + txnid size(4 bytes) + logOp size(4 bytes) + table id
    _nCap = (PAGE_SIZE - DATA_BEGIN_OFFSET) / _nFixed;
    _nextLogLineID = 0;
}

LogPage::LogPage(PageID nPageID): LinkedPage(nPageID) {
    GetHeader((uint8_t *)&_nFixed, 2, FIXED_SIZE_OFFSET);
    GetHeader((uint8_t *)&_nextLogLineID, 2, NEXT_LOGLINE_OFFSET);
}

LogPage::~LogPage() {
    SetHeader((uint8_t *)&_nFixed, 2, FIXED_SIZE_OFFSET);
    SetHeader((uint8_t *)&_nextLogLineID, 2, NEXT_LOGLINE_OFFSET);
}

void LogPage::log(LogLine logline) {
    assert(!Full());
    // 直接写到持久存储
    SetData((uint8_t *)&(logline._tableID), 4, DATA_BEGIN_OFFSET + _nextLogLineID * _nFixed);
    SetData((uint8_t *)&(logline._txnID), 4, DATA_BEGIN_OFFSET + _nextLogLineID * _nFixed + 4);
    SetData((uint8_t *)&(logline._logOp), 4, DATA_BEGIN_OFFSET + _nextLogLineID * _nFixed + 8);
    SetData((uint8_t *)&(logline._location.first), 4, DATA_BEGIN_OFFSET + _nextLogLineID * _nFixed + 12);
    SetData((uint8_t *)&(logline._location.second), 4, DATA_BEGIN_OFFSET + _nextLogLineID * _nFixed + 16);
    _nextLogLineID ++;
}

std::vector<LogLine> LogPage::getLogs() const {
    std::vector<LogLine> logs;
    for (int i = 0; i < _nextLogLineID; i++) {
        LogLine logline;
        GetData((uint8_t *)&(logline._tableID), 4, DATA_BEGIN_OFFSET + i * _nFixed);
        GetData((uint8_t *)&(logline._txnID), 4, DATA_BEGIN_OFFSET + i * _nFixed + 4);
        GetData((uint8_t *)&(logline._logOp), 4, DATA_BEGIN_OFFSET + i * _nFixed + 8);
        GetData((uint8_t *)&(logline._location.first), 4, DATA_BEGIN_OFFSET + i * _nFixed + 12);
        GetData((uint8_t *)&(logline._location.second), 4, DATA_BEGIN_OFFSET + i * _nFixed + 16);
        logs.push_back(logline);
    }
    return logs;
}

bool LogPage::Full() const { 
    return _nextLogLineID >= _nFixed; 
}

} // namespace thdb
