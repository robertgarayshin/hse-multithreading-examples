#include "dfs.h"

#include <iostream>

Task Dfs(Scheduler &sched, const Graph &graph, std::vector<bool> &visited, int node) {
  if (visited[node]) {
    co_return;
  }

  visited[node] = true;
  std::cout << "[start]  node " << node << "\n";

  co_await Yield{sched};

  std::cout << "[resume] node " << node << " — spawning children\n";

  for (int neighbor : graph[node]) {
    if (!visited[neighbor]) {
      sched.Spawn(Dfs(sched, graph, visited, neighbor));
    }
  }

  std::cout << "[done]   node " << node << "\n";
}
