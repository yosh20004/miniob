// #include "sql/operator/update_physical_operator.h"
// #include "storage/table/table.h"
// #include "storage/trx/trx.h"
// #include "common/log/log.h"


// // 注意，update 的时候不会使用索引
// RC UpdatePhysicalOperator::open(Trx *trx)
// {
//   auto &child = children_[0];
//   RC    rc    = child->open(trx);
//   if (OB_FAIL(rc)) {
//     LOG_WARN("child operator open failed: %s", strrc(rc));
//     return rc;
//   }

//   unordered_map<string, Table *> base_table_map;

//   while (OB_SUCC(rc = child->next())) {
//     Tuple *tuple_ = child->current_tuple();  // 获得当前正在更新的 tuple
//     auto   tuple  = dynamic_cast<RowTuple *>(tuple_);
//     ASSERT(tuple != nullptr, "tuple cannot cast to RowTuple here!");

//     // 选择更新的表
//     if (table_->is_view()) {
//       if (tuple->cell_num() != tuple->rid_list_.size()) {
//         LOG_PANIC("update view: cell num %d is not equal to rid num %d", tuple->cell_num(), tuple->rid_list_.size());
//         return RC::INTERNAL;
//       }
//       for (size_t i = 0; i < tuple->rid_list_.size(); i++) {
//         // 在多表的情况下，rowtuple 中的 cell 可能来自不同表的 tuple，他们都有自己的 rid 和 table_name
//         auto tuple_table_name = tuple->table_name_list_[i];
//         auto update_rid       = tuple->rid_list_[i];

//         LOG_DEBUG("we are updating base table of view: %s, rid: %s", tuple_table_name.c_str(), update_rid.to_string().c_str());

//         if (tuple_table_name.empty()) {
//           LOG_PANIC("update view: raw table name is empty, we might got failed");
//           return RC::INTERNAL;
//         }
//         if (base_table_map.find(tuple_table_name) == base_table_map.end()) {
//           LOG_PANIC("update view: cannot find base table: %s", tuple_table_name.c_str());
//           return RC::INTERNAL;
//         }
//         update_table = base_table_map[tuple_table_name];
//         // 正式开始更新
//         rc = update_table->visit_record(update_rid, [this, tuple](Record &record) {
//           Record             old_record(record);
//           std::vector<Value> cells_to_update;  // 先存，防止有一个 field 更新异常导致部分写入。
//           // for (size_t i = 0; i < exprs_.size(); i++) {
//           std::vector<size_t> update_field_idx = selected_update_field_idx_[update_table->name()];
//           for (size_t i : update_field_idx) {
//             Value cell;
//             RC    rc = exprs_[i]->get_value(*tuple, cell);

//             // 子查询返回空值的情况
//             if (rc == RC::RECORD_EOF && field_metas_[i].nullable()) {
//               cell.set_null();
//             } else if (OB_FAIL(rc)) {
//               LOG_WARN("cannot get value from expression: %s", strrc(rc));
//             }

//             if (cell.is_null() && !field_metas_[i].nullable()) {
//               LOG_WARN("field %s is not nullable, but the value is null", field_metas_[i].name());
//               return RC::INVALID_ARGUMENT;
//             }

//             // we get the value again to check if the subquery is legal
//             if (exprs_[i]->type() == ExprType::SUB_QUERY) {
//               Value test_cell_;
//               RC    rc_ = exprs_[i]->get_value(*tuple, test_cell_);
//               if (rc_ != RC::RECORD_EOF) {
//                 LOG_WARN("update: subquery should return only one value");
//                 return RC::INVALID_ARGUMENT;
//               }
//             }

//             if (cell.attr_type() != field_metas_[i].type()) {
//               Value to_value;
//               rc = Value::cast_to(cell, field_metas_[i].type(), to_value);
//               if (OB_FAIL(rc)) {
//                 LOG_WARN("cannot cast from %s to %s", attr_type_to_string(cell.attr_type()), attr_type_to_string(field_metas_[i].type()));
//                 return RC::INVALID_ARGUMENT;
//               }
//               cell = to_value;
//             }
//             // 检查向量维度是否完全一致
//             if (cell.attr_type() == AttrType::VECTORS &&
//                 static_cast<size_t>(cell.length()) != field_metas_[i].vector_dim() * sizeof(float)) {
//               LOG_WARN("vector length exceeds limit: %d != %d", cell.data_length()/4, field_metas_[i].vector_dim());
//               return RC::INVALID_ARGUMENT;
//             }
//             // tuple->set_cell_at(field_metas_[i].field_id(), cell, record.data());
//             cells_to_update.push_back(cell);
//           }

//           for (size_t i = 0; i < cells_to_update.size(); ++i) {
//             size_t field_metas_idx = update_field_idx[i];
//             tuple->set_cell_at(field_metas_[field_metas_idx].field_id(), cells_to_update[i], record.data());
//           }

//           cells_to_update.clear();
//           RC rc = update_table->update_index(old_record, record, field_metas_);
//           if (OB_FAIL(rc)) {
//             LOG_WARN("update index failed: %s", strrc(rc));
//             return rc;
//           }
//           return RC::SUCCESS;
//         });
//       }

//     } else {
//       // 非视图更新情况
//       rc = table_->visit_record(tuple->record().rid(), [this, tuple](Record &record) {
//         Record             old_record(record);
//         std::vector<Value> cells_to_update;  // 先存，防止有一个 field 更新异常导致部分写入。
//         for (size_t i = 0; i < exprs_.size(); i++) {
//           Value cell;
//           RC    rc = exprs_[i]->get_value(*tuple, cell);

//           // 子查询返回空值的情况
//           if (rc == RC::RECORD_EOF && field_metas_[i].nullable()) {
//             cell.set_null();
//           } else if (OB_FAIL(rc)) {
//             LOG_WARN("cannot get value from expression: %s", strrc(rc));
//           }

//           if (cell.is_null() && !field_metas_[i].nullable()) {
//             LOG_WARN("field %s is not nullable, but the value is null", field_metas_[i].name());
//             return RC::INVALID_ARGUMENT;
//           }

//           // we get the value again to check if the subquery is legal
//           if (exprs_[i]->type() == ExprType::SUB_QUERY) {
//             Value test_cell_;
//             RC    rc_ = exprs_[i]->get_value(*tuple, test_cell_);
//             if (rc_ != RC::RECORD_EOF) {
//               LOG_WARN("update: subquery should return only one value");
//               return RC::INVALID_ARGUMENT;
//             }
//           }

//           if (cell.attr_type() != field_metas_[i].type()) {
//             Value to_value;
//             rc = Value::cast_to(cell, field_metas_[i].type(), to_value);
//             if (OB_FAIL(rc)) {
//               LOG_WARN("cannot cast from %s to %s", attr_type_to_string(cell.attr_type()), attr_type_to_string(field_metas_[i].type()));
//               return RC::INVALID_ARGUMENT;
//             }
//             cell = to_value;
//           }
//           // 检查向量维度是否完全一致
//           if (cell.attr_type() == AttrType::VECTORS &&
//               static_cast<size_t>(cell.length()) != field_metas_[i].vector_dim() * sizeof(float)) {
//             LOG_WARN("vector length exceeds limit: %d != %d", cell.data_length()/4, field_metas_[i].vector_dim());
//             return RC::INVALID_ARGUMENT;
//           }
//           // tuple->set_cell_at(field_metas_[i].field_id(), cell, record.data());
//           cells_to_update.push_back(cell);
//         }

//         for (size_t i = 0; i < cells_to_update.size(); ++i) {
//           tuple->set_cell_at(field_metas_[i].field_id(), cells_to_update[i], record.data());
//         }

//         cells_to_update.clear();
//         RC rc = table_->update_index(old_record, record, field_metas_);
//         if (OB_FAIL(rc)) {
//           LOG_WARN("update index failed: %s", strrc(rc));
//           return rc;
//         }
//         return RC::SUCCESS;
//       });
//     }

//     if (rc != RC::SUCCESS) {
//       return rc;
//     }
//   }

//   if (rc == RC::RECORD_EOF) {
//     rc = RC::SUCCESS;
//   }

//   child->close();
//   if (OB_FAIL(rc)) {
//     LOG_WARN("update index failed: %s", strrc(rc));
//     return rc;
//   }
//   return RC::SUCCESS;
// }

// RC UpdatePhysicalOperator::close() { return RC::SUCCESS; }

// RC UpdatePhysicalOperator::next() { return RC::RECORD_EOF; }



#include "sql/operator/update_physical_operator.h"
#include "storage/table/table.h"
#include "storage/trx/trx.h"
// #include "common/log/log.h"


// 注意，update 的时候不会使用索引
RC UpdatePhysicalOperator::open(Trx *trx)
{return RC::SUCCESS;}

RC UpdatePhysicalOperator::close() { return RC::SUCCESS; }

RC UpdatePhysicalOperator::next() { return RC::RECORD_EOF; }