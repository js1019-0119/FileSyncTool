#ifndef SYNCDATA_H
#define SYNCDATA_H
//同步数据

//建议同步的动作
enum class SyncSuggDirection
{
    SyncSuggDirection_None = 0,  //无需同步
    SyncSuggDirection_AtoB,         //AtoB
    SyncSuggDirection_BtoA,         //BtoA
    SyncSuggDirection_RemoveInB,    //删除B内容
    SyncSuggDirection_RemoveInA     //删除A内容
};

//同步模式
enum class SyncMode
{
    SyncMode_TwoWay = 0,    //双向同步
    SyncMode_AtoB,          //AtoB
    SyncMode_BtoA           //BtoA
};

#endif // SYNCDATA_H
