/*这是我们为了实现src/observer/sql/optimizer/logical_plan_generator.cpp中的create_plan(UpdateStmt)函数准备的类
 *这个文件是新建的 若编译工作大体正常 将被提交
 */


#pragma once

#include "common/value.h"
#include "sql/operator/logical_operator.h"
#include "storage/table/table.h"

/**
 * @brief 更新逻辑算子
 * @ingroup LogicalOperator
 */
class UpdateLogicalOperator : public LogicalOperator 
{
public:
  UpdateLogicalOperator(Table *table, const char *attribute_name, const Value &value) : 
    table_(table), attribute_name_(attribute_name), value_(value)
  {}
  virtual ~UpdateLogicalOperator() = default;

  LogicalOperatorType type() const override { return LogicalOperatorType::UPDATE; }

  Table *table() const { return table_; }
  const char *attribute_name() const { return attribute_name_; }
  const Value &value() const { return value_; }

private:
  Table *table_ = nullptr;                // 表名
  const char *attribute_name_ = nullptr;  // 要更新的字段名
  Value value_;                           // update值
};