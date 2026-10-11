/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

// libfork 3.8 与 Apple libc++ 的适配层
//
// 背景: libfork 在 `__cpp_lib_atomic_ref` 存在时, 会用 `std::atomic_ref` 读取 const 上下文中
// 的 bool 成员(core/impl/frame.hpp 的 atomic_has_exception)。而 Apple 的 libc++ 未提供
// `std::atomic_ref<const T>`, 该分支无法编译。这里在引入 libfork 之前取消该特性宏, 让它
// 退回 `std::atomic_bool` 分支 —— 语义等价, 代价是每个协程帧多一个 atomic 成员。
//
// 约定: 所有使用 libfork 的 TU 必须包含本头文件, 不得直接写 #include "libfork.hpp",
// 否则同一个 TU 内可能出现两套 frame 布局(ODR 违规)。

#include <atomic>

#ifdef __cpp_lib_atomic_ref
#undef __cpp_lib_atomic_ref
#endif

#include "libfork.hpp"
