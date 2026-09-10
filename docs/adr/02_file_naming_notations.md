# 2 文件命名规范

- Status: accepted

## 2.1 头文件名定义

头文件`.h`要有前缀，前缀代表其属性便于调用和解释依赖关系，常用缩写表示，不同前缀的定义如下：

| 前缀      | 前缀全称                | package | 说明               |
|---------|---------------------|---|------------------|
| c_      | common              | common | 公共方法             |
| hc_     | hard constraint     | problem | 硬约束              |
| sc_     | soft constraint     | problem | 软约束              |
| cc_     | cost constraint     | problem | 成本约束             |
| pdm_    | problem domain      | problem | 问题层的领域对象         |
| pr_     | problem             | problem | 问题               |
| visual_ | visual              | problem | 可视化(结果、过程等数据可视化) |
| bdm_    | base domain         | scenario | 基本的领域对象          |
| se_     | scenario            | scenario | 场景               |
| scr_    | standard csv reader | scenario | 标准的csv输入读取       |
| scp_    | standard csv parser | scenario | 内存转为标准的csv输入     |
| s_      | solver              | solver | solver           |

## 2.2 头文件和源文件关系

头文件`.h`和对应的源文件`.cpp`的名字要相同这样能快速找到对应关系，在`.cpp`文件承载信息过多丧失可读性、可维护性时应当按照一定的分类规则拆分为多个`.cpp`文件，且通过前缀关联`.h`和`.cpp`文件，特殊场景如下：

* `.h`文件中类超过3个；
* 成员方法超过10个；
* `.cpp`文件行数超过350行；
