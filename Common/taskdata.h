#ifndef TASKDATA_H
#define TASKDATA_H
//任务

//任务状态
enum class TaskState
{
    TaskState_Pending = 0,  //未开始
    TaskState_Running,      //正在运行
    TaskState_Paused,       //暂停
    TaskState_Completed,    //已完成
    TaskState_Failed,       //失败
    TaskState_Canceled      //已取消
};


#endif // TASKDATA_H
