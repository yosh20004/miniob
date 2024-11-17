#include "sql/stmt/update_stmt.h"
#include "common/log/log.h"
#include "storage/db/db.h"
#include "storage/table/table.h"

/*我们对于整个UpdateStmt类都进行了一定程度的修改 
 *现在它拥有四个基本的私有成员: 表 字段 目标值 过滤条件 
 */


UpdateStmt::UpdateStmt(Table *table, const char *attribute_name, const Value &value, FilterStmt *filter_stmt)
    : table_(table), attribute_name_(attribute_name), value_(value), filter_stmt_(filter_stmt)
{}

UpdateStmt::~UpdateStmt()
{
  if (nullptr != filter_stmt_) {
    delete filter_stmt_;
    filter_stmt_ = nullptr;
  }
}

RC UpdateStmt::create(Db *db, const UpdateSqlNode &update_sql, Stmt *&stmt)
{
  const char *table_name = update_sql.relation_name.c_str();
  const char *attribute_name = update_sql.attribute_name.c_str();

  // std::cout << update_sql.relation_name << std::endl;
  // std::cout << update_sql.attribute_name << std::endl;
  
  if (nullptr == db || nullptr == table_name || nullptr == attribute_name) {
    LOG_WARN("invalid argument. db=%p, table_name=%p, attribute_name=%p",
        db, table_name, attribute_name);
    return RC::INVALID_ARGUMENT;
  }

  // 检查表是否存在
  Table *table = db->find_table(table_name);
  if (nullptr == table) {
    LOG_WARN("no such table. db=%s, table_name=%s", db->name(), table_name);
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  // 检查字段是否存在
  const TableMeta &table_meta = table->table_meta();
  const FieldMeta *field = table_meta.field(attribute_name);
  if (nullptr == field) {
    LOG_WARN("no such field in table. table=%s, field=%s", table_name, attribute_name);
    return RC::SCHEMA_FIELD_NOT_EXIST;
  }

  // 创建过滤条件
  std::unordered_map<std::string, Table *> table_map;
  table_map.insert(std::pair<std::string, Table *>(std::string(table_name), table));

  FilterStmt *filter_stmt = nullptr;
  RC rc = FilterStmt::create(
      db, table, &table_map, update_sql.conditions.data(), static_cast<int>(update_sql.conditions.size()), filter_stmt);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to create filter statement. rc=%d:%s", rc, strrc(rc));
    return rc;
  }

  // 创建更新语句
  stmt = new UpdateStmt(table, attribute_name, update_sql.value, filter_stmt);
  return RC::SUCCESS;
}