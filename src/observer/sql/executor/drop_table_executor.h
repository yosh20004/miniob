/*本类为了实现drop table而添加，是为了删除某一张表*/

#pragma once
#include "common/rc.h"

class SQLStageEvent;


class DropTableExecutor {
public:
    RC execute(SQLStageEvent* sql_event);
};
