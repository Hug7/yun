/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <vector>

#include "algo_parallel_executor.h"
#include "pdm_load.h"
#include "pdm_segment.h"
#include "pdm_solution.h"

class LoadEditor;
class SolverContext;

/**
 * @brief repair matrix 中一个候选插入方案的轻量视图
 * @details 由 unfold_* 产出, 只携带定位与目标值增量; 提交时用 (segment_ind, load_ind)
 * 回到 matrix 中取真实数据
 */
struct RepairCellView {
  /**
   * @brief 插入后的目标值增量
   */
  double delta{0.0};
  /**
   * @brief segment 下标
   */
  int segment_ind{-1};
  /**
   * @brief load 下标
   */
  int load_ind{-1};
};

/**
 * @brief RepairCellView 的全序
 * @details delta 升序, 同 delta 按 segment_ind、load_ind 升序; 严格全序, 无并列
 */
struct RepairCellViewLess {
  bool operator()(const RepairCellView& a, const RepairCellView& b) const {
    if (a.delta != b.delta) {
      return a.delta < b.delta;
    }
    if (a.segment_ind != b.segment_ind) {
      return a.segment_ind < b.segment_ind;
    }
    return a.load_ind < b.load_ind;
  }
};

/**
 * @brief repair matrix 中一格 (segment, load) 的描述
 * @details 不可行格保留占位(is_feasible=false, delta=+inf), 不留 nullptr;
 * 行列下标由所在行/列隐式给出, 因此视图只带 (segment_ind, load_ind)。
 * POD: 只保存重做该插入所需的最小信息, 不保存 Load 副本。
 */
struct RepairCell {
  /**
   * @brief 插入后的目标值增量; 不可行格统一 +inf
   */
  double delta{std::numeric_limits<double>::infinity()};
  /**
   * @brief 插入槽位下标
   */
  std::int32_t slot_ind{-1};
  /**
   * @brief 车型在 vehicle_usages 中的下标
   */
  std::int32_t vehicle_ind{-1};
  /**
   * @brief 评估时的列版本
   */
  std::int32_t version{-1};
  /**
   * @brief 是否有可行插入方案
   */
  bool is_feasible{false};
  /**
   * @brief 评估时"可行但因被占满而跳过"的车型位图
   * @details 第 i 位为 1 表示: 除该格记录的方案外, 车型 i 当时也装得下但没有空位,
   * 车型 i 一旦释放, 这一格的结论可能变好。可行格与不可行格都记录(可行格用于发现更便宜
   * 的车型), 由 RepairMatrix 在对应车型释放时定点重评估。位图按下标 % 64 分桶,
   * 车型数超过 64 时同一桶的车型共用一位, 释放时命中的格子会偏多
   */
  std::uint64_t watched_vehicles{0};
};

/**
 * @brief 行内可行候选的排序项(P2 候选索引的最小单元)
 */
struct RankedCell {
  /**
   * @brief 插入代价增量
   */
  double delta{0.0};
  /**
   * @brief 目标 load 的列下标
   */
  int load_ind{-1};
};

/**
 * @brief RankedCell 的升序比较: delta 优先, load_ind 兜底, 与全序一致
 */
struct RankedCellLess {
  [[nodiscard]] bool operator()(const RankedCell& lhs, const RankedCell& rhs) const;
};

/**
 * @brief repair matrix 的一行, 对应一个 segment
 */
struct RepairRow {
  /**
   * @brief 该 segment 是否仍在待指派集合中
   */
  bool active{false};
  /**
   * @brief 列信息, 长度 = 当前 load 数量
   */
  std::vector<RepairCell> cells;
  /**
   * @brief 本行可行格的有序视图(升序, 与 cells 同步维护)
   * @details 行内计数只由该行在并行阶段的唯一写者修改; 矩阵总数在串行屏障后对各行求和
   */
  std::vector<RankedCell> ranked;
};

/**
 * @brief 未指派 segment × 现有 load 的插入代价矩阵
 * @details 行按 segment 下标稠密存储, 列随 solution 中 load 数量增长;
 * 并行阶段每格只有一个写者, 不允许任何插入/扩容。
 */
class RepairMatrix {
 public:
  /**
   * @brief 并行执行器
   * @details 由策略层持有, 本类不负责释放
   */
  ParallelExecutor* parallel_executor;
  /**
   * @brief 插入方案评估器
   * @details 由 SolverContext 持有, 本类不负责释放
   */
  LoadEditor* load_editor;
  /**
   * @brief load 上下文
   * @details 并行任务用它取时间窗写缓冲; 由 Problem 持有, 本类不负责释放
   */
  LoadContext* load_context;
  /**
   * @brief solver 上下文; 仅生产路径的构造器会赋值
   */
  const SolverContext* context{nullptr};
  /**
   * @brief solution
   */
  Solution* sol;
  /**
   * @brief segment 列表
   */
  const std::vector<Segment> segments;
  /**
   * @brief 待分配的 segment 索引列表
   * @details 保持插入顺序(std::erase 删除), 不使用 swap-erase
   */
  std::vector<int> idle_segment_indices;
  /**
   * @brief matrix 的行, rows[segment_ind]
   */
  std::vector<RepairRow> rows;

  /**
   * @brief 从 solver 上下文构造(生产路径)
   */
  explicit RepairMatrix(ParallelExecutor* _executor, const SolverContext* _context, Solution* _sol,
                        std::vector<Segment> _segments);

  /**
   * @brief 直接从依赖构造
   * @details 供不依赖 SolverContext(整套 CSV 场景)的单元测试复用同一实现;
   * 生产路径走上面的上下文重载。
   */
  RepairMatrix(ParallelExecutor* _executor, LoadEditor* _load_editor, LoadContext* _load_context,
               Solution* _sol, std::vector<Segment> _segments);

  RepairMatrix(const RepairMatrix&) = delete;
  RepairMatrix& operator=(const RepairMatrix&) = delete;
  RepairMatrix(RepairMatrix&&) = delete;
  RepairMatrix& operator=(RepairMatrix&&) = delete;

  /**
   * @brief 预建全部行并并行评估整张矩阵
   */
  void init();

  /**
   * @brief 并行入口: 提交一批任务并阻塞等待
   * @details 任务体拿到的写缓冲是任务私有的时间窗写缓冲; 屏障返回前本函数会把所有写缓冲的新增
   * 条目归档进请求内共享的冻结表。调用方拿不到"不归档"的路径。
   * 任务体抛出的异常在此收敛, 不会穿越并行边界。
   */
  void run_parallel(int task_count, const std::function<void(int, CacheStoreBuffer*)>& task) const;

  /**
   * @brief 计算一格 (segment, load) 的插入结果
   * @details 不可行(含抛异常)时返回 is_feasible=false、delta=+inf 的占位, 保证不留 nullptr
   */
  [[nodiscard]] RepairCell eval_cell(int segment_ind, int load_ind,
                                     CacheStoreBuffer* store_buffer) const;

  void remove_segment(int segment_ind);

  /**
   * @brief 落实一个候选插入方案
   * @details 只按候选记录的 (slot_ind, vehicle_ind) 重做插入, 不设运行期资源守卫:
   * 车型一被占满就立即重评估受影响的格子(见 refresh_saturated_vehicle), 因此候选记录的
   * 车型要么就是该 load 的当前车型(Solution::update_load 先释放后占用, 净零),
   * 要么仍能再占用一次。版本闸门与资源不变式都通过后重做仍失败, 说明矩阵与解已经不同构
   * (内部不变式被破坏): 记录 error 日志并抛 std::logic_error, 不再静默降级
   * @param view 由 unfold_* 产出、且所在列未被重算过的候选
   * @exception std::logic_error 候选与当前解不再同构(不应发生, 出现即为缺陷)
   */
  void commit(const RepairCellView& view);

  /**
   * @brief 把一个已失效的候选标记为不可行
   * @details 下一次该列重算会覆盖它; 保证算子循环单调收敛。commit 不再以返回 false
   * 表示失效, 本方法保留给需要主动否决候选的调用方
   * @param view 由 unfold_* 产出、当前仍可行的候选
   */
  void mark_infeasible(const RepairCellView& view);

  /**
   * @brief 可行格数量
   * @details P2-A 起为 O(1) 增量计数(不再逐格扫描); Debug 构建中会断言其与全表重算一致
   */
  [[nodiscard]] int unfold_matrix_cell_count() const;

  /**
   * @brief 取全表 top-k 可行候选
   * @return 全序升序(front() 最优)
   */
  [[nodiscard]] std::vector<RepairCellView> unfold_matrix_top_k(int k) const;

  /**
   * @brief 取指定 segment 行的 top-k 可行候选
   * @return 全序升序(front() 最优); 行不存在或不可行候选不足时返回实际数量
   */
  [[nodiscard]] std::vector<RepairCellView> unfold_segment_top_k(int segment_ind, int k) const;

  /**
   * @brief load_ind 列变化后重算整列
   * @details 串行补列(禁止并行扩容)后并行重算每个 idle 行的该列; 版本次数 +1
   */
  void update_load(int load_ind);

  /**
   * @brief 用一个 idle segment 新建车次
   * @return 成功新建返回 true; 所有 idle segment 都无可行车型时返回 false
   */
  bool idle_segment_create_load();

  /**
   * @brief 测试钩子: 非空时按提交顺序记录候选
   * @details 仅用于确定性回归测试比对 (segment_ind, load_ind) 提交序列,
   * 生产路径恒为 nullptr, 不参与任何决策
   */
  std::vector<RepairCellView>* commit_trace{nullptr};

 private:
  /**
   * @brief 每列的版本号, 长度 = 当前列数
   * @details 列被重算后 +1, 用于把"矩阵不得静默过期"变成可执行契约
   */
  std::vector<std::int32_t> load_version_;
  /**
   * @brief 版本号生成器
   */
  std::int32_t version_counter_{0};
  /**
   * @brief 全表可行格总数(增量维护)
   * @details 各行的 ranked 由并行写者维护; 本字段只在串行阶段(init/remove_segment/
   * mark_infeasible/update_load 屏障后)按各行 ranked.size() 求和更新
   */
  int feasible_count_{0};
  /**
   * @brief 各车型的满仓快照(1 = 已无空位)
   */
  std::vector<char> vehicle_full_;
  /**
   * @brief 与 solution 的车辆资源比对快照, 对刚被占满/释放的车型刷新矩阵
   */
  void sync_vehicle_fullness();
  /**
   * @brief 重评估"记录用该车型、且该车型不是本 load 当前车型"的全部可行格
   */
  void refresh_saturated_vehicle(int vehicle_ind);
  /**
   * @brief 车型重新空出后, 重评估把它列为待跟踪车型的格子
   */
  void retry_released_vehicle(int vehicle_ind);

  /**
   * @brief 收集若干行内的可行候选视图
   */
  void collect_row_views(int segment_ind, std::vector<RepairCellView>& out) const;
};
