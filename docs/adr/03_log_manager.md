# 3 日志管理

- Status: accepted

## 3.1 三个输出

日志同时写到三处：控制台、`log_dir`、`output_dir`。后两个入参是**目录**，文件名统一为 `{request_id}.log`。文件 sink 用追加模式，同一 `request_id` 重复求解时不会丢掉上一次的日志。

格式为 `[%Y-%m-%d %H:%M:%S.%e] [%t] [%l] %v`，带毫秒和**线程号**：封装给 Python 之后调用方会开多线程，没有线程号的交织日志无法归因。控制台 sink 全进程共享同一个对象，多个请求并发写控制台时由 sink 内部的锁串行化。

## 3.2 配置从入参进来，不读 Config.yaml

`Config.yaml` 属于外层封装（Python 包），C++ 包发布后既不能依赖它也不能修改它，所以日志目录和日志等级都是 `Solver` 的构造入参，由调用方解析 `Config.yaml` 后传进来；三个输出共用同一个等级。

被否决的方案是 C++ 内部读 `Config.yaml`：那会把「配置文件在哪」变成 C++ 包自己的问题（旧 `common/include/c_log.h` 里就留着一个硬编码绝对路径和 `fix me: 路径问题`），而且发布后的包无法保证 `Config.yaml` 存在。

## 3.3 每个请求一个 logger，不用全局默认 logger

`LogManager::create(request_id, log_dir, output_dir, level)`（`common/include/c_log.h`，header-only，因为 `yun_common` 是 INTERFACE 库）返回该请求专属的 `std::shared_ptr<spdlog::logger>`，由 `SolverContext` 持有，需要打日志的模块拿到句柄后自己调：`StandardCsvReader`、`VisualManager`、`StrategyManager` 存成成员，`SolverPrecheck` 走 `context->logger`。logger 不注册进 spdlog 的 registry，因此 `request_id` 撞车也不会抛 `logger already exists`。

全程不调用 `spdlog::set_default_logger`。原因：`spdlog::info(...)` 内部通过 `default_logger_raw()` 拿到的是**不加锁的裸指针**（`registry.h` 第 41-44 行注释、`spdlog.h` 第 128-131 行的 IMPORTANT 都写明了不能与 `set_default_logger` 并发），一旦有人边写日志边换默认 logger，旧 logger 会在别人的 `sink_it_()` 底下析构，是 use-after-free。封装给 Python 后调用方开多线程，这个前提保不住。

被否决的方案是进程级单例 + 设为 spdlog 默认 logger：改动最小（30+ 处 `spdlog::info` 一行不用改），但只在「一个进程一次求解、且求解串行」的前提下成立。

代价是 logger 句柄必须显式传递。拿不到句柄的代码（例如 `common/include/c_file_utils.h` 里的 `file_exists`）继续调 `spdlog::error` 时会落到 spdlog 内置的默认 logger：只上控制台、级别固定 info，不进文件，但也不会崩。

并发验证：8 个线程同时 `create` 并各写 1000 条日志，8 个文件各 1001 行、零串档。

## 3.4 创建时机在 SolverContext

两个日志文件的名字都取决于 `request_id`，而 `request_id` 和 `output_dir` 都是在 `SolverContext` 构造时才生成的，所以 `LogManager::create` 由 `SolverContext` 在拼装完 `output_dir` 之后、`load_scenario()` 之前调用，`log_dir` 与等级由 `Solver` 透传进来。只有这样，场景加载这段全仓最密的日志才能进 `output_dir`；`output_dir` 本身也由文件 sink 顺带创建（spdlog 会建父目录），不再依赖排在后面的 `VisualManager`。

`logger` 成员声明在 `scenario`、`parameter` 等之前，因此析构时最后释放；`~SolverContext` 里显式 `flush()`，调用方不需要再管刷盘。

## 3.5 日志失败不中断求解

目录不可写时只跳过对应的文件 sink，并往 `cerr` 打一行，控制台照常输出；等级字符串无法识别时退化为 `info`——spdlog 的 `level::from_str` 对未知名字返回 `off`，等于静默关掉全部日志，这个坑必须在包内部堵住。

不调用 `spdlog::shutdown()`：它会 `drop_all()` 把默认 logger 置空，而 `spdlog::info` 直接解引用该裸指针，之后任何一条日志都是空指针崩溃；封装成包后同一进程里可能反复求解。

## 3.6 request_id 的唯一性

`request_id` 的格式是 `%Y%m%d%H%M%S_{进程标识}_{序号}`，例如 `20260912010513_6b0f_1`。只有时间戳（精确到秒）是不够的：并发求解时同一秒内的两个请求会拿到相同的 `request_id`，于是共用同一个 `output_dir`，两个 logger 也会追加写同一个日志文件。

进程标识取 uuid 的前 4 位，进程启动后固定，负责区分「同一秒内的不同进程」；序号是进程内的原子自增，负责区分「同一进程内的并发请求」，这一半是确定性的，不靠概率。

被否决的方案是拼完整 uuid（`%Y%m%d%H%M%S_{uuid}`，`generate_uuid()` 已经实现）：51 个字符的目录名太长。也没采用纯随机短后缀，因为同秒并发存在生日碰撞，而且 fork 出的子进程会继承随机数发生器状态。

遗留约束：fork 型多进程（Linux 上 Python `multiprocessing` 的默认方式）会让子进程继承父进程的进程标识，若父子在同一秒内各自发起求解且序号撞上，仍会重名。真用到这种部署方式时，应改为由调用方把 `request_id` 作为入参传进来。
