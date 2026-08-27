# 时间窗

## 术语

### 时间窗定义

时间窗是指一个闭合的时间区间，包含最早时间(`early`)和最晚时间(`late`)，形如`[early,late]`，可以赋予订单的提\卸货时间段、仓库和门店等节点的作业时间段的业务属性。

### 多段时间窗

节点的时间窗是可以有多段的来应对午休和高峰期没有提卸货能力的问题。

### 超时

到达节点的时间超过了规定的`late`，且该`late`为最后一段可用的时间窗，则计为超时。

### 等待

到达节点的时间小于最近时间窗的`early`则视为等待，要等到`early`时间才能开始作业，期间耗费的时间定义为等待时间。

### 工作时间

节点的持续作业时间，常要求在时间窗内完成。

### 行驶时间

两个节点之间的行驶时长。

## 时间窗推导

时间窗推导是指在路由中基于每个节点的作业能力和路网信息考虑时间窗需求，以不超时少等待为标准推演各个节点的预计到达时间、开始作业时间、离开时间的范围。  
时间窗推导的python伪代码如下：

```python
class TimeWindow:
    """
    时间窗
    """
    early: int # 时间窗开始时间
    late: int # 时间窗结束时间

class TimeWindowPlan:
    """
    计划时间
    """
    early_arr: int # 最早到达时间
    early_dest: int # 最早离开时间
    late_arr: int # 最晚到达时间
    late_dest: int # 最晚离开时间
    work_time: int # 工作时间
    cost_time: int # 行驶时间

def infer(pre_node_tw_plan: TimeWindowPlan, 
          cur_node_tw: TimeWindow,
          cur_node_work_time: int,
          pre2cur_cost_time: int) -> Optional[TimeWindowPlan]:
    if cur_node_tw.late - cur_node_tw.early < cur_node_work_time:
        return None
    res_tw_plan: TimeWindowPlan = TimeWindowPlan()

    early_arr = pre_node_tw_plan.early_dest + pre2cur_cost_time
    late_arr = pre_node_tw_plan.late_dest + pre2cur_cost_time

    if early_arr <= cur_node_tw.late:
        if late_arr >= cur_node_tw.early:
            # normal
            res_tw_plan.early_arr = max(early_arr, cur_node_tw.early)
            res_tw_plan.late_arr = min(late_arr, cur_node_tw.late)
            res_tw_plan.early_dest = res_tw_plan.early_arr + cur_node_work_time
            res_tw_plan.late_dest = res_tw_plan.late_arr + cur_node_work_time
        else:
            # case of wait time
            res_tw_plan.early_arr = late_arr
            res_tw_plan.late_arr = late_arr
            res_tw_plan.early_dest = cur_node_tw.early_arr + cur_node_work_time
            res_tw_plan.late_dest = cur_node_tw.early_arr + cur_node_work_time
            res_tw_plan.wait_time = cur_node_tw.early_arr - late_arr
    else:
        # case of over time
        res_tw_plan.early_arr = early_arr
        res_tw_plan.late_arr = early_arr
        res_tw_plan.early_dest = early_arr + cur_node_work_time
        res_tw_plan.late_dest = early_arr + cur_node_work_time
        res_tw_plan.over_time = early_arr - cur_node_tw.late

    return res_tw_plan
```
