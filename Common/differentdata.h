#ifndef DIFFERENTDATA_H
#define DIFFERENTDATA_H
//差异数据

//差异类型
enum class DiffType
{
    DiffType_Same = 0,  //两边都有
    DiffType_OnlyA,     //只有A有
    DiffType_OnlyB,     //只有B有
    DiffType_ContentDiff,   //两边都有但内容不同
    DiffType_Conflict       //冲突
};

#endif // DIFFERENTDATA_H
