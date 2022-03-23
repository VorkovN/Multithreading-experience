#include <iostream>
#include "producer_consumer.h"

int main(int argc, char** argv) {
  uint threadsCount = atoi(argv[1]);
  uint maxSleepTime = atoi(argv[2]);
  int sum = run_threads(threadsCount, maxSleepTime);
  std::cout << sum << std::endl;
  return 0;
}
