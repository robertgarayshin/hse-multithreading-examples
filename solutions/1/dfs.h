#pragma once

#include "scheduler.h"
#include <vector>

using Graph = std::vector<std::vector<int>>;

Task Dfs(Scheduler &sched, const Graph &graph, std::vector<bool> &visited, int node);
