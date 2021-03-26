#include "page/linked_page.h"

#include "macros.h"
#include "minios/os.h"

namespace thdb {

const PageOffset NEXT_PAGE_OFFSET = 4;
const PageOffset PREV_PAGE_OFFSET = 8;

// NOTE: 本构造对象 会 向OS请求新的页面
LinkedPage::LinkedPage() : Page() {
  this->_bModified = true;
  this->_nNextID = NULL_PAGE;
  this->_nPrevID = NULL_PAGE;
}

// NOTE: 本构造对象 并不会 向OS请求新的页面
// 初始化对象的时候，读出页面的header元数据
// _nNextID 放在页面header中 offset = 4 处
// _nPrevID 放在页面header中 offset = 8 处
LinkedPage::LinkedPage(PageID nPageID) : Page(nPageID) {
  this->_bModified = false;
  GetHeader((uint8_t *)&_nNextID, 4, NEXT_PAGE_OFFSET);  // 32bit, 解析为一个PageID(uint32_t)
  GetHeader((uint8_t *)&_nPrevID, 4, PREV_PAGE_OFFSET);
}

LinkedPage::~LinkedPage() {
  if (_bModified) {
    // Dirty Page Condition
    // RAII: 对象析构的时候写回dirty页面，保证正确性
    SetHeader((uint8_t *)&_nNextID, 4, NEXT_PAGE_OFFSET);
    SetHeader((uint8_t *)&_nPrevID, 4, PREV_PAGE_OFFSET);
  }
}

uint32_t LinkedPage::GetNextID() const { return _nNextID; }

uint32_t LinkedPage::GetPrevID() const { return _nPrevID; }

void LinkedPage::SetNextID(PageID nNextID) {
  this->_nNextID = nNextID;
  this->_bModified = true;
}

void LinkedPage::SetPrevID(PageID nPrevID) {
  this->_nPrevID = nPrevID;
  this->_bModified = true;
}

bool LinkedPage::PushBack(LinkedPage *pPage) {
  if (!pPage) return false;
  // LAB1 BEGIN
  // 链表结点后增加一个新的链表页面结点
  // ALERT: ！！！一定要注意！！！
  // 不要同时建立两个指向相同磁盘位置的且可变对象，否则会出现一致性问题
  // ALERT:
  // 页面对象完成修改后一定要及时析构，析构会自动调用写回。以方便其他函数重新基于这一页面建立页面对象
  // TIPS: 需要判断当前页面是否存在后续页面
  // TIPS：正确设置当前页面和pPage的PrevID以及NextID
  PageID insertPageID = pPage->GetPageID();
  if (_nNextID != NULL_PAGE) {
    LinkedPage* next_page = new LinkedPage(_nNextID);
    next_page->SetPrevID(insertPageID);
    pPage->SetNextID(next_page->GetPageID());
    delete next_page;
  }
  SetNextID(insertPageID);
  pPage->SetPrevID(_nPageID);
  // LAB1 END
  return true;
}

PageID LinkedPage::PopBack() {
  // LAB1 BEGIN
  // 删除下一个链表结点，返回删除结点的PageID
  // TIPS: 需要判断当前页面是否存在后续页面
  // TIPS:
  // 正确设置当前页面的NextID，如果后一个结点不是空页，需要讨论是否存在后两页的PrevID问题
  // TIPS: 可以使用MiniOS::DeletePage在最后释放被删除的页面
  Size temp_next_id = _nNextID;
  if (temp_next_id != NULL_PAGE) {
    LinkedPage* next_page = new LinkedPage(temp_next_id);
    // 后一个节点之后还有页面
    if (next_page->GetNextID() != NULL_PAGE) {
      PageID next_next_id = next_page->GetNextID();
      SetNextID(next_next_id);
      LinkedPage* next_next_page = new LinkedPage(next_next_id);
      next_next_page->SetPrevID(_nPageID);
      delete next_next_page;
    } else {
      SetNextID(NULL_PAGE);
    }
    delete next_page;
    MiniOS::GetOS()->DeletePage(temp_next_id);
  }
  return temp_next_id;
  // LAB1 END
}

}  // namespace thdb
