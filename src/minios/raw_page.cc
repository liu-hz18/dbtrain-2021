#include "minios/raw_page.h"

#include <cstring>

#include "exception/exceptions.h"
#include "macros.h"

namespace thdb {

RawPage::RawPage() {
  _pData = new uint8_t[PAGE_SIZE]; // PAGE_SIZE == 4096 Byte
  memset(_pData, 0, PAGE_SIZE); // 内存中的页面，初始化全0
}

RawPage::~RawPage() { delete[] _pData; }

// 将page中数据读取到dst
void RawPage::Read(uint8_t* dst, PageOffset nSize, PageOffset nOffset) {
  if ((nSize + nOffset) > PAGE_SIZE) throw PageOutOfSizeException();
  memcpy(dst, _pData + nOffset, nSize);
}

// 将src内容存到page中
void RawPage::Write(const uint8_t* src, PageOffset nSize, PageOffset nOffset) {
  if ((nSize + nOffset) > PAGE_SIZE) throw PageOutOfSizeException();
  memcpy(_pData + nOffset, src, nSize);
}

}  // namespace thdb
