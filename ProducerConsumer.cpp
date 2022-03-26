#include "ProducerConsumer.h"

#include <csignal>
#include <iostream>
#include <vector>

// можно было обойтись и без DECREMENT_MUTEX, но тогда код был бы менее читабельным
pthread_cond_t CV_CONSUMER = PTHREAD_COND_INITIALIZER;
pthread_cond_t CV_PRODUICER = PTHREAD_COND_INITIALIZER;
pthread_mutex_t TERM_MUTEX = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t SUM_MUTEX = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t DECREMENT_MUTEX = PTHREAD_MUTEX_INITIALIZER;
bool producerIsAlive = false;
uint aliveConsumersCount = 0;

int runThreads(uint consumerThreadsCount, uint maxSleepTime) {
  int term;
  int sum = 0;
  uint uRandomSleepTime;

  //Consumer creation
  std::vector<pthread_t> consumerThreads(consumerThreadsCount);
  for (int i = 0; i < consumerThreadsCount; ++i) {
    uRandomSleepTime = generateuRandomSleepTime(maxSleepTime);
    auto consumerArgs = ConsumerRoutineArgs{&uRandomSleepTime, &term, &sum};
    pthread_create(&consumerThreads[i], nullptr, consumerRoutine, &consumerArgs);
    pthread_detach(consumerThreads[i]);
    ++aliveConsumersCount;
  }
  sleep(1);

  //Produicer creation
  pthread_t producer;
  uRandomSleepTime = generateuRandomSleepTime(maxSleepTime);
  auto producerArgs = ProducerRoutineArgs{&term};
  pthread_create(&producer, nullptr, producerRoutine, &producerArgs);
  pthread_detach(producer);
  producerIsAlive = true;
  sleep(1);

  //Interrupter creation
  pthread_t interruptor;
  uRandomSleepTime = generateuRandomSleepTime(maxSleepTime);
  auto interruptorArgs = ConsumerInterruptorRoutineArgs{
      &uRandomSleepTime, consumerThreads.size(), consumerThreads.data()};
  pthread_create(&interruptor, nullptr, consumerInterruptorRoutine, &interruptorArgs);
  void** interruptorResult;
  pthread_join(interruptor, interruptorResult);

  pthread_cond_destroy(&CV_PRODUICER);
  pthread_cond_destroy(&CV_CONSUMER);
  pthread_mutex_destroy(&TERM_MUTEX);
  pthread_mutex_destroy(&SUM_MUTEX);
  return sum;
}

void* consumerRoutine(void* args) {
  auto* consumerRoutineArgs = static_cast<ConsumerRoutineArgs*>(args);
  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);

  while (true) {
    pthread_mutex_lock(&TERM_MUTEX);
    pthread_cond_wait(&CV_CONSUMER, &TERM_MUTEX);
    int& i = *(consumerRoutineArgs->term);
    int term = i;
    pthread_cond_signal(&CV_PRODUICER);
    pthread_mutex_unlock(&TERM_MUTEX);

    if (!producerIsAlive) {
      pthread_mutex_lock(&DECREMENT_MUTEX);
      --aliveConsumersCount;
      pthread_mutex_unlock(&DECREMENT_MUTEX);
      break;
    }

    pthread_mutex_lock(&SUM_MUTEX);
    (*consumerRoutineArgs->sum) += term;
    pthread_mutex_unlock(&SUM_MUTEX);

    usleep(*consumerRoutineArgs->uRandomSleepTime);
  }
  return nullptr;
}

void* producerRoutine(void* args) {
  auto producerRoutineArgs = static_cast<ProducerRoutineArgs*>(args);

  //гарантия, что consumer успеет встать в wait
  pthread_mutex_lock(&TERM_MUTEX);
  pthread_cond_wait(&CV_PRODUICER, &TERM_MUTEX);
  pthread_mutex_unlock(&TERM_MUTEX);

  bool result = true;
  while (result) {
    pthread_mutex_lock(&TERM_MUTEX);
    int& i = *(producerRoutineArgs->term);
    result = bool(std::cin >> i);
    pthread_cond_signal(&CV_CONSUMER);
    pthread_cond_wait(&CV_PRODUICER, &TERM_MUTEX);
    pthread_mutex_unlock(&TERM_MUTEX);
  }

  producerIsAlive = false;
  pthread_mutex_lock(&TERM_MUTEX);
  while (aliveConsumersCount > 0)
    pthread_cond_signal(&CV_CONSUMER);
  pthread_mutex_unlock(&TERM_MUTEX);
  return nullptr;
}

void* consumerInterruptorRoutine(void* args) {
  auto consumerInterruptorRoutineArgs =
      static_cast<ConsumerInterruptorRoutineArgs*>(args);

  while (aliveConsumersCount > 0) {
//    for (int i = 0; i < consumerInterruptorRoutineArgs->threadsCount; ++i)
//      pthread_cancel(consumerInterruptorRoutineArgs->threads[i]);
    usleep(*consumerInterruptorRoutineArgs->uRandomSleepTime);
  }
  return nullptr;
}

uint generateuRandomSleepTime(uint maxSleepTime) {
  srand(time(nullptr));
  return (rand() % maxSleepTime + 2 + 1) * 1000;
}

int getTid() {
  // 1 to 3+N thread ID
  thread_local static int* tid;
  return *tid;
}

