#ifndef THDB_NODE_PAGE_H_
#define THDB_NODE_PAGE_H_

#include "defines.h"
#include "field/field.h"
#include "page/page.h"

namespace thdb {

class Index;

/**
 * @brief 同时表示了中间结点和叶结点。
 * 中间结点每个Key为子结点中Key的最小值，Value为子结点{PageID,0}。
 * 叶结点每个Key为实际字段的Key，Value为对应记录的PageSlotID。
 */
class NodePage : public Page {
 public:
  /**
   * @brief 初始化结点页面
   * @param nKeyLen Key长度
   * @param iKeyType Key类型
   * @param bLeaf 是否为叶结点
   */
  NodePage(Size nKeyLen, FieldType iKeyType, bool bLeaf);
  /**
   * @brief 构建一个包含一定数量子结点的结点页面
   *
   * @param nKeyLen Key长度
   * @param iKeyType Key类型
   * @param bLeaf 是否为叶结点
   * @param iKeyVec 子结点的Key值
   * @param iChildVec 子结点的Value值（PageSlotID数组）
   */
  NodePage(Size nKeyLen, FieldType iKeyType, bool bLeaf,
           const std::vector<Field *> &iKeyVec,
           const std::vector<PageSlotID> &iChildVec);
  /**
   * @brief 导入一个已经存在的页面结点。
   *
   * @param nPageID 页面结点的页编号
   */
  NodePage(PageID nPageID);
  ~NodePage();

  /**
   * @brief 插入一条Key Value Pair
   * @param pKey 插入的Key
   * @param iPair 插入的Value
   * @return true 插入成功
   * @return false 插入失败
   */
  bool Insert(Field *pKey, const PageSlotID &iPair);
  /**
   * @brief 删除某个Key下所有的Key Value Pair
   * @param pKey 删除的Key
   * @return Size 删除的键值数量
   */
  Size Delete(Field *pKey);
  /**
   * @brief 删除某个Key Value Pair
   * @param pKey 删除的Key
   * @param iPair 删除的Value
   * @return true 删除成功
   * @return false 删除失败
   */
  bool Delete(Field *pKey, const PageSlotID &iPair);
  /**
   * @brief 更新某个Key Value Pair到新的Value
   * @param pKey 更新的Key
   * @param iOld 原始的Value
   * @param iNew 要更新成的新Value
   * @return true 更新成功
   * @return false 更新失败
   */
  bool Update(Field *pKey, const PageSlotID &iOld, const PageSlotID &iNew);
  /**
   * @brief 使用索引进行范围查找，左闭右开区间[pLow, pHigh)
   *
   * @param pLow
   * @param pHigh
   * @return std::vector<PageSlotID> 所有符合范围条件的Value数组
   */
  std::vector<PageSlotID> Range(Field *pLow, Field *pHigh);
  /**
   * @brief 清空当前结点和所有子结点所占用的所有空间
   */
  void Clear();

  /**
   * @brief 判断结点是否为满结点
   */
  bool Full() const;
  /**
   * @brief 判断结点是否为空结点
   */
  bool Empty() const;

  /**
   * @brief 获得结点保存的索引字段类型
   */
  FieldType GetType() const;

 private:
  /**
   * @brief 解析格式化的页面数据，初始化结点信息。
   */
  void Load();
  /**
   * @brief 将结点信息保存为格式化的页面数据。
   */
  void Store();

  /**
   * @brief 获得该结点第一个Key
   */
  Field *FirstKey() const;

  /**
   * @brief 不小于pKey的第一个Key在KeyVec中的位置
   */
  Size LowerBound(Field *pKey);
  /**
   * @brief 大于pKey的第一个Key在KeyVec中的位置
   */
  Size UpperBound(Field *pKey);
  /**
   * @brief 小于等于pKey的最后一个Key在KeyVec中的位置
   */
  Size LessBound(Field *pKey);

  /**
   * @brief 对于中间结点初始化第一个子叶结点
   */
  void InitFirst();
  /**
   * @brief 对于中间结点重置第一个子叶结点
   *
   */
  void ResetFirst();
  /**
   * @brief 分裂当前结点，清除当前结点中后一半的结点。
   * @return std::pair<std::vector<Field *>, std::vector<PageSlotID>>
   * 分裂出的后一半Key Value Pair数据
   */
  std::pair<std::vector<Field *>, std::vector<PageSlotID>> PopHalf();

  /**
   * @brief 页面对应结点是否为叶结点
   */
  bool _bLeaf;
  /**
   * @brief 结点页面一个Key占用的空间
   */
  Size _nKeyLen;
  /**
   * @brief 结点页面能存储的最大KeyValuePair容量
   */
  Size _nCap;
  /**
   * @brief 结点页面Key的类型
   */
  FieldType _iKeyType;

  /**
   * @brief Key数组，用于存储类型为Field*的Key
   */
  std::vector<Field *> _iKeyVec;
  /**
   * @brief Value数组，用于存储类型为PageSlotID的Value
   */
  std::vector<PageSlotID> _iChildVec;
  friend class Index;
};

}  // namespace thdb

#endif
