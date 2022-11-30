//
//  threadpool.hpp
//  WebServer
//
//  Created by 蒋艺 on 2022/11/28.
//

#ifndef threadpool_hpp
#define threadpool_hpp

#include <stdio.h>
#include <pthread.h>
#include <list>
#include "locker.hpp"
#include <exception>

template<typename T>
class Threadpool {
public:
    Threadpool(int thread_number=8, int max_requests=10000);
    ~Threadpool();
    bool append(T* request);
private:
    int thread_number;
    pthread_t * m_threads;
    int m_max_requests;
    std::list<T*> m_workqueue;
    Locker m_queuelocker;
    Sem m_queuestat;
    bool m_stop;
    /**
     * @brief call back function of thread
     * 
     * @param[in] arg arguments of the call back function
     * @return void* 
     */
    static void* worker(void *arg);
    void run();
};

template<typename T>
Threadpool<T>::Threadpool(int thread_number, int max_requests) :
thread_number(thread_number), m_max_requests(max_requests),
m_stop(false), m_threads(NULL) {
    
    if (this->thread_number <= 0 or m_max_requests <=0 ) {
        throw std::exception();
    }
    
    // initialized threads
    m_threads = new pthread_t[thread_number];
    if (!m_threads) {
        throw std::exception();
    }
    
    // create threads and set attribute
    for (int i=0; i<thread_number; i++) {
        printf("create the %dth thread\n", i);
        if (pthread_create(m_threads+i, NULL, worker, this) !=0) {
            delete [] m_threads;
            throw std::exception();
        }
        
        // set detach to avoid the need for parent thread to release resources
        if ( pthread_detach(m_threads[i]) ) {
            delete [] m_threads;
            throw  std::exception();
        }
        
    }
    
}

template <typename T>
Threadpool<T>::~Threadpool(){
    delete[] m_threads;
    m_stop = true;
}

template <typename T>
bool Threadpool<T>::append(T* request) {
    m_queuelocker.lock();
    if (m_workqueue.size() > m_max_requests ) {
        m_queuelocker.unlock();
        return false;
    }
    
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();
    return true;
}

template <typename T>
void* Threadpool<T>::worker(void* arg) {
    Threadpool* pool = (Threadpool*) arg;
    pool->run();
    return pool;
}

template <typename T>
void Threadpool<T>::run(){
    while(!m_stop) {
        m_queuestat.wait();
        m_queuelocker.lock();
        if ( m_workqueue.empty() ) {
            m_queuelocker.unlock();
            continue;
        }
        
        T* request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();
        
        if(!request) {
            continue;
        }
        
        request->process();
    }
}
#endif /* threadpool_hpp */
