/*本类(本文件)是为了src/observer/sql/stmt/stmt.cpp::create_stmt里分支而实现
 */

#pragma once

#include "sql/stmt/stmt.h"

class DropTableStmt :public Stmt {
public:
    explicit DropTableStmt(std::string table_name_): table_name_(std::move(table_name_)) {
    }

    ~DropTableStmt() override = default;

    StmtType type() const override { //实现基类定义的纯虚函数
        return StmtType::DROP_TABLE;
    }

    [[nodiscard]] const std::string &table_name() const { //返回要drop的表名字
        return table_name_;
    }

    static RC create(const Db *db, const DropTableSqlNode &sql_node, Stmt *&stmt);

private:
    std::string table_name_;
};
