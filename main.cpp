#include <iostream>
#include "ProducerConsumer.h"

//8000 потоков обрабытывают 256000 чисел +- за 43сек
int main(int argc, char** argv) {
  uint threadsCount = atoi(argv[1]);
  uint maxSleepTime = atoi(argv[2]);
  int sum = runThreads(threadsCount, maxSleepTime);
  std::cout << sum << std::endl;
  return 0;
}
