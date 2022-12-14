#ifndef THDB_INDEX_EXCEPTION_H_
#define THDB_INDEX_EXCEPTION_H_

#include "defines.h"
#include "exception/exception.h"

namespace thdb {

class IndexException : public Exception {
  virtual const char* what() const throw() { return "Index Exception"; }
};

class IndexTypeException : public IndexException {
  virtual const char* what() const throw() { return "Unknown Index Type Exception"; }
};

}  // namespace thdb

#endif