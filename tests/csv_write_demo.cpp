/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

// rapidcsv 写 CSV 示例：三个常见场景，运行后在 /tmp/rapidcsv_demo 下生成文件并读回校验。
// 编译：clang++ -std=c++20 -I common/include tests/csv_write_demo.cpp -o /tmp/csv_write_demo

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "c_rapidcsv.h"

namespace {

const char* kOutputDir = "/tmp/rapidcsv_demo";

// 场景 1：带表头的业务数据。SetColumnName 写表头，SetRow 写整行，SetCell 写单个格子。
// 两个坑：新建文件必须用空路径构造 Document（Document("不存在的路径") 会抛 ios 异常）；
//         表头要先用 SetColumnName 占位，否则第一行数据会顶到表头位置、留下一行空表头。
void demo_header_row_cell(const std::string& path) {
  // LabelParams(列名行下标, 行名列下标)，行名列为 -1 表示不用行名
  rapidcsv::Document doc("", rapidcsv::LabelParams(0, -1));

  doc.SetColumnName(0, "order_id");
  doc.SetColumnName(1, "customer");
  doc.SetColumnName(2, "pieces");
  doc.SetColumnName(3, "weight_kg");

  // SetRow 的行号是数据行下标，从 0 开始，不含表头行；整行同类型时最省事。
  // 值里带分隔符（这里的逗号）会被自动加引号。
  doc.SetRow<std::string>(0, {"SO-1001", "华东仓, 上海", "8", "12.5"});

  // 混合类型时用 SetCell，参数顺序是（列, 行）
  doc.SetCell<std::string>(0, 1, "SO-1002");
  doc.SetCell<std::string>(1, 1, "华南仓");
  doc.SetCell<int>(2, 1, 3);
  doc.SetCell<double>(3, 1, 3.75);

  // 追加一行：行号取 GetRowCount()。
  // double 按 17 位精度写出，28.0 会变成 "28"；要固定小数位就自己格式化成 string 再写。
  const size_t next_row = doc.GetRowCount();
  doc.SetCell<std::string>(0, next_row, "SO-1003");
  doc.SetCell<std::string>(1, next_row, "华北仓");
  doc.SetCell<int>(2, next_row, 15);
  doc.SetCell<double>(3, next_row, 28.0);

  doc.Save(path);  // Save() 不传参数就是写回构造时的路径
}

// 场景 2：无表头的矩阵（距离矩阵、成本矩阵这类）。
// LabelParams(-1, -1) 之后，行号列号与文件里的物理位置一一对应。
void demo_matrix_without_header(const std::string& path) {
  rapidcsv::Document doc("", rapidcsv::LabelParams(-1, -1));

  // 用 SetRow 把 3x3 矩阵逐行写满。SetColumn 只负责它自己那一列，
  // 不会补齐同一行里其它没写过的格子（没写过的格子输出为空字符串）。
  doc.SetRow<int>(0, {0, 12, 35});
  doc.SetRow<int>(1, {12, 0, 27});
  doc.SetRow<int>(2, {35, 27, 0});

  // 整列覆盖也是支持的，常用来修正或重算某一列
  doc.SetColumn<int>(2, {35, 27, 0});

  // 新增一个网点，矩阵扩成 4x4：SetColumn 会顺手把行数补齐到列的长度，
  // 但补出来的新行其它格子是空的，所以最后要用 SetRow 把新行整行写满。
  doc.SetColumn<int>(3, {48, 41, 19, 0});
  doc.SetRow<int>(3, {48, 41, 19, 0});

  doc.Save(path);
}

// 场景 3：自定义分隔符 + 写到内存流 + 打开已有文件插行追加 + 读回校验。
void demo_separator_stream_append(const std::string& path) {
  // SeparatorParams(分隔符, trim, 是否用 \r\n, 引号内允许换行, 自动加引号, 引号字符)
  const rapidcsv::SeparatorParams sep(';');

  rapidcsv::Document doc("", rapidcsv::LabelParams(0, -1), sep);
  doc.SetColumnName(0, "location_code");
  doc.SetColumnName(1, "location_name");
  doc.SetColumnName(2, "capacity");
  doc.SetRow<std::string>(0, {"L001", "上海分拨中心", "120"});

  // Save 也能写进任意输出流，适合拼日志或塞进 HTTP 响应
  std::ostringstream buffer;
  doc.Save(buffer);
  std::cout << "--- Save(std::ostream) 的内容 ---\n" << buffer.str();

  doc.Save(path);

  // 重新打开时 LabelParams / SeparatorParams 要和写入时一致，否则表头会被当成数据
  rapidcsv::Document reopened(path, rapidcsv::LabelParams(0, -1), sep);

  // InsertRow 在指定位置插行，后面的行整体下移；这里插到第一条数据行之前
  reopened.InsertRow<std::string>(0, {"L000", "北京总部", "200"});

  // SetCell + GetRowCount() 则是追加到末尾
  const size_t next_row = reopened.GetRowCount();
  reopened.SetCell<std::string>(0, next_row, "L002");
  reopened.SetCell<std::string>(1, next_row, "广州分拨中心");
  reopened.SetCell<int>(2, next_row, 80);
  reopened.Save();

  // 读回校验：按列名取整列，按（列名, 行号）取单格
  const std::vector<std::string> codes = reopened.GetColumn<std::string>("location_code");
  for (size_t i = 0; i < codes.size(); i++) {
    std::cout << codes[i] << " -> " << reopened.GetCell<std::string>("location_name", i)
              << ", capacity=" << reopened.GetCell<int>("capacity", i) << "\n";
  }
  std::cout << "rows=" << reopened.GetRowCount() << ", cols=" << reopened.GetColumnCount() << "\n";
}

void print_file(const std::string& path) {
  std::cout << "\n=== " << path << " ===\n";
  std::ifstream in(path);
  std::cout << in.rdbuf() << "\n";
}

}  // namespace

int main() {
  std::filesystem::create_directories(kOutputDir);
  const std::string dir(kOutputDir);

  demo_header_row_cell(dir + "/orders.csv");
  demo_matrix_without_header(dir + "/dist_matrix.csv");
  demo_separator_stream_append(dir + "/locations_semicolon.csv");

  print_file(dir + "/orders.csv");
  print_file(dir + "/dist_matrix.csv");
  print_file(dir + "/locations_semicolon.csv");
  return 0;
}
