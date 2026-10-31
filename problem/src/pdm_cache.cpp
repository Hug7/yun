/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pdm_cache.h"

// ====== implement of CacheStoreBuffer ======
CacheStoreBuffer::CacheStoreBuffer() { this->node_time_window = new NodeTimeWindowStoreBuffer(); }

CacheStoreBuffer::~CacheStoreBuffer() { delete this->node_time_window; }

// ====== implement of Cache ======
Cache::Cache(const Parameter* parameter) {
  this->node_time_window = new NodeTimeWindowCache(parameter->plan_datetime_range);
}

Cache::~Cache() { delete this->node_time_window; }

void Cache::archive(const CacheStoreBuffer* cache_store_buffer) const {
  this->node_time_window->archive(cache_store_buffer->node_time_window);
}