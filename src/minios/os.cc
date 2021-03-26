#include "minios/os.h"

#include <cstring>
#include <fstream>

#include "exception/exceptions.h"
#include "macros.h"
#include "minios/raw_page.h"
#include "settings.h"

namespace thdb {

MiniOS *MiniOS::os = nullptr;

// 单例模式，只返回1个操作系统实例
MiniOS *MiniOS::GetOS() {
  if (os == nullptr) os = new MiniOS();
  return os;
}

// 写回的时候os实例销毁
void MiniOS::WriteBack() {
  if (os != nullptr) {
    delete os;
    os = nullptr;
  }
}

// 构造函数中读取，析构函数写出的方式来进行持久化操作
MiniOS::MiniOS() {
  _pMemory = new RawPage *[MEM_PAGES]; // 指针的数组，每个指针指向一个 RawPage
  _pUsed = new Bitmap(MEM_PAGES);
  memset(_pMemory, 0, MEM_PAGES);
  _nClock = 0;
  LoadBitmap();
  LoadPages();
}

MiniOS::~MiniOS() {
  StoreBitmap();
  StorePages();
  for (size_t i = 0; i < MEM_PAGES; ++i)
    if (_pMemory[i]) delete _pMemory[i]; // 将已分配的页面空间释放（MiniOS对真实的OS释放）
  delete[] _pMemory;
  delete _pUsed;
}

// 分配一个新的页面，返回页面编号
PageID MiniOS::NewPage() {
  Size tmp = _nClock;
  // 从 _nClock 开始循环一圈，寻找第一个可以被分配的页面
  do {
    if (!_pUsed->Get(_nClock)) {
      _pMemory[_nClock] = new RawPage();
      _pUsed->Set(_nClock);
      return _nClock;
    } else {
      _nClock += 1;
      _nClock %= MEM_PAGES;
    }
  } while (_nClock != tmp);
  throw NewPageException();
}

// 删除指定编号的页面, 不存在写回
void MiniOS::DeletePage(PageID pid) {
  if (!_pUsed->Get(pid)) throw PageNotInitException(pid);
  delete _pMemory[pid];
  _pMemory[pid] = 0; // set null
  _pUsed->Unset(pid);
}

void MiniOS::ReadPage(PageID pid, uint8_t *dst, PageOffset nSize,
                      PageOffset nOffset) {
  if (!_pUsed->Get(pid)) throw PageNotInitException(pid);
  _pMemory[pid]->Read(dst, nSize, nOffset);
}

void MiniOS::WritePage(PageID pid, const uint8_t *src, PageOffset nSize,
                       PageOffset nOffset) {
  if (!_pUsed->Get(pid)) throw PageNotInitException(pid);
  _pMemory[pid]->Write(src, nSize, nOffset);
}

void MiniOS::LoadBitmap() {
  // bitmap序列化到磁盘文件，便于加载OS
  std::ifstream fin("THDB_BITMAP", std::ios::binary);
  if (!fin) return;
  uint8_t pTemp[MEM_PAGES / 8];
  fin.read((char *)pTemp, MEM_PAGES / 8);
  fin.close();
  _pUsed->Load(pTemp);
}

void MiniOS::LoadPages() {
  // 将Page从真实的磁盘中加载出来（之后利用内存模拟磁盘，只有析构的时候重新写入磁盘）
  std::ifstream fin("THDB_PAGE", std::ios::binary);
  if (!fin) return;
  uint8_t pTemp[PAGE_SIZE];
  for (uint32_t i = 0; i < MEM_PAGES; ++i) {
    if (_pUsed->Get(i)) {
      fin.read((char *)pTemp, PAGE_SIZE);
      _pMemory[i] = new RawPage();
      _pMemory[i]->Write(pTemp, PAGE_SIZE);
    }
  }
  fin.close();
}

void MiniOS::StoreBitmap() {
  std::ofstream fout("THDB_BITMAP", std::ios::binary);
  if (!fout) return;
  uint8_t pTemp[MEM_PAGES / 8];
  _pUsed->Store(pTemp);
  fout.write((char *)pTemp, MEM_PAGES / 8);
  fout.close();
}

// 将内存中的页面写入磁盘
void MiniOS::StorePages() {
  std::ofstream fout("THDB_PAGE", std::ios::binary);
  if (!fout) return;
  uint8_t pTemp[PAGE_SIZE];
  for (uint32_t i = 0; i < MEM_PAGES; ++i) {
    if (_pUsed->Get(i)) {
      _pMemory[i]->Read(pTemp, PAGE_SIZE);
      fout.write((char *)pTemp, PAGE_SIZE);
    }
  }
  fout.close();
}

Size MiniOS::GetUsedSize() const {
  if (!_pUsed) throw OsException();
  return _pUsed->GetSize();
}

}  // namespace thdb
