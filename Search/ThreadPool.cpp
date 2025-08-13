/*
 * Copyright ©2025 Travis McGaha.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Pennsylvania
 * CIT 5950 for use solely during Spring Semester 2025 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include <unistd.h>
#include <iostream>

#include "./ThreadPool.hpp"

namespace searchserver {

// This is the thread start routine, i.e., the function that threads
// are born into.
void *thread_loop(void *t_pool);

ThreadPool::ThreadPool(size_t num_threads) : q_lock_(), q_cond_(), work_queue_(), killthreads_(false), num_threads_(num_threads), thread_vec_(num_threads) {
  // Initialize lock and condition
  pthread_mutex_init(&q_lock_, nullptr);
  pthread_cond_init(&q_cond_, nullptr);
  
  // Create the worker threads
  for (size_t i = 0; i < num_threads; i++) {
    int result = pthread_create(&thread_vec_[i], nullptr, thread_loop, this);
    if (result != 0) {
      std::cerr << "Failed to create thread: " << result << std::endl;
    }
  }
}

ThreadPool:: ~ThreadPool() {
  // Signal all threads to exit
  pthread_mutex_lock(&q_lock_);
  killthreads_ = true;
  
  // Wake up all threads so they can check the killthreads_ flag
  pthread_cond_broadcast(&q_cond_);
  pthread_mutex_unlock(&q_lock_);
  
  // Wait for all threads to finish
  for (size_t i = 0; i < num_threads_; i++) {
    pthread_join(thread_vec_[i], nullptr);
  }
  
  // Do any remaining tasks in queue
  while (!work_queue_.empty()) {
    Task task = work_queue_.front();
    work_queue_.pop_front();
    task.func_(task.arg_);
  }
  
  // Destroy the mutex and condition variable
  pthread_mutex_destroy(&q_lock_);
  pthread_cond_destroy(&q_cond_);
}

// Enqueue a Task for dispatch.
void ThreadPool::dispatch(Task t) {
  // Lock before accessing the queue
  pthread_mutex_lock(&q_lock_);
  
  // Add task to queue
  work_queue_.push_back(t);
  
  // Signal all waiting threads there is work
  pthread_cond_broadcast(&q_cond_);
  
  // Unlock
  pthread_mutex_unlock(&q_lock_);
}

// This is the main loop that all worker threads are born into.  They
// wait for a signal on the work queue condition variable, then they
// grab work off the queue.  Threads return (i.e., kill themselves)
// when they notice that killthreads_ is true.
void *thread_loop(void *t_pool) {
    ThreadPool *pool = static_cast<ThreadPool*>(t_pool);
    
    pthread_mutex_lock(&pool->q_lock_);

    while (true) {
        // Check if should exit
        if (pool->killthreads_ && pool->work_queue_.empty()) {
            pthread_mutex_unlock(&pool->q_lock_);
            return nullptr;
        }

        // Wait for work if queue is empty
        while (pool->work_queue_.empty() && !pool->killthreads_) {
            pthread_cond_wait(&pool->q_cond_, &pool->q_lock_);
        }
        
        // Check again if need to exit
        if (pool->killthreads_ && pool->work_queue_.empty()) {
            pthread_mutex_unlock(&pool->q_lock_);
            return nullptr;
        }
        
        // If no work and exit is true
        if (pool->work_queue_.empty()) {
            pthread_mutex_unlock(&pool->q_lock_);
            return nullptr;
        }
        
        // Work to do so get task from front of queue
        ThreadPool::Task task = pool->work_queue_.front();
        pool->work_queue_.pop_front();
        
        // Unlock before executing task
        pthread_mutex_unlock(&pool->q_lock_);
        
        // Execute the task
        task.func_(task.arg_);
        
        // Lock again for next iteration
        pthread_mutex_lock(&pool->q_lock_);
    }

    
    pthread_mutex_unlock(&pool->q_lock_);
    return nullptr;
}

}  // namespace searchserver
