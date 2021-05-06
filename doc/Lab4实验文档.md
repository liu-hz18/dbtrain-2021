# Lab4 实验文档

## 框架主要变更

- 新增类：TransactionManager, Transaction
- 接口修改：Table 类的 InsertRecord, DeleteRecord, UpdateRecord, SearchRecord 接口增加一个传入参数 Transaction *txn
