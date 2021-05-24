#ifndef THDB_DEFINE_H_
#define THDB_DEFINE_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <map>
#include <string>
#include <vector>

namespace thdb {

typedef uint32_t PageID;
typedef uint16_t PageOffset;
typedef uint16_t SlotID;
typedef uint16_t FieldID;
typedef std::string String;
typedef uint32_t Size;
typedef int32_t Rank;

typedef std::pair<PageID, SlotID> PageSlotID;

typedef uint32_t TxnID;

enum class LogOperation {
    UNDEFINED = 0,
    BEGIN = 1,
    COMMIT = 2,
    ABORT = 3,
    INSERT = 4,
    UPDATE = 5,
    DELETE = 6
};

}  // namespace thdb

#endif
