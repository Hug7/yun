/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pr_pattern.h"

Load* PatternSPMD::create_load(LoadContext* context) const { return new LoadSPMD(context); }
