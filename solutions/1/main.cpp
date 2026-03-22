#include "dfs.h"

#include <iostream>

int main() {
  constexpr int kNodes = 6;
  Graph graph(kNodes);

  graph[0] = {1, 2};
  graph[1] = {0, 3, 4};
  graph[2] = {0, 5};
  graph[3] = {1};
  graph[4] = {1};
  graph[5] = {2};

  std::vector visited(kNodes, false);

  Scheduler sched;

  std::cout << "=== Обход графа в глубину с кооперативной многозадачностью ===" << std::endl << std::endl;

  sched.Spawn(Dfs(sched, graph, visited, 0));

  sched.Run();

  std::cout << std::endl << "=== Все вершины посещены ===" << std::endl;
  return 0;
}
