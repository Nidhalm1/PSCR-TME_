#pragma once

#include "Job.h"
#include "Queue.h"
#include <thread>
#include <vector>

namespace pr {

class Pool {
  Queue<Job> queue;
  std::vector<std::thread> threads;
  bool running_;

public:
  // create a pool with a queue of given size
  Pool(int qsize) : queue(qsize), running_(false) { /*TODO*/ }
  // start the pool with nbthread workers
  void start(int nbthread) {
    /*TODO*/
    running_ = true;
    for (int i = 0; i < nbthread; i++) {
      // syntaxe pour passer une methode membre au thread
      threads.emplace_back(&Pool::worker, this); //
    }
  }

  // submit a job to be executed by the pool
  void submit(Job *job) { /*TODO*/ queue.push(job); }

  // initiate shutdown, wait for threads to finish
  void stop() {
    running_ = false;
    queue.setbloquant(false);
    for (auto &t : threads) {
      t.join();
    }
  }

private:
  // worker thread function
  void worker() { // todo
    while (running_) {
      Job *b = queue.pop();
      if (b) {
        b->run();
        delete b;
      }
    }
  }
};

} // namespace pr
