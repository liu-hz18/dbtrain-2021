#include "display.h"

namespace thdb {

// 用于打印查询结果
void PrintTable(std::vector<Result *> &results) {
  for (auto result : results) {
    result->Display();
    delete result;
  }
}

}  // namespace thdb
