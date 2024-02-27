/*
MIT License

Copyright (c) 2024 Cenxuan

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.


*/

#ifndef __TMC_THREADPOOL_HPP__
#define __TMC_THREADPOOL_HPP__

#include "tmc_LockfreeQ.hpp"

#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <future>

#define __CUR_TYPENAME ThreadPool
#define __DEL_COPY_CONS    __CUR_TYPENAME(const __CUR_TYPENAME&)=delete;
#define __DEL_MOVE_CONS    __CUR_TYPENAME(__CUR_TYPENAME&&)=delete;
#define __DEL_COPY_ASIGN   __CUR_TYPENAME& operator=(__CUR_TYPENAME const&)=delete;
#define __DEL_MOVE_ASIGN   __CUR_TYPENAME& operator=(__CUR_TYPENAME &&)=delete;

namespace TMC
{
template<size_t _PoolSize,size_t _TaskQueueSize>
class ThreadPool
{
    __DEL_COPY_CONS
    __DEL_MOVE_CONS
    __DEL_COPY_ASIGN
    __DEL_MOVE_ASIGN
private:
    std::thread* threads[_PoolSize];// pointers of threads
    LockfreeQ<std::function<void()>,_TaskQueueSize> task_queue;
    bool need_stop = false;
    size_t rest_size = 0;
    std::mutex condition_mtx;
    std::condition_variable condition_var;
    void _dispatch(){
        while (!need_stop)
        {
            std::unique_lock<std::mutex> ul(condition_mtx);
            rest_size++;
            condition_var.wait(ul);
            ul.unlock();
            rest_size--;
            std::function<void(void)> f;

            pop_again:
            size_t pop_pos = task_queue.safe_pop_pos();
            if (pop_pos != ~0ULL) {
                f = task_queue.safe_pop(pop_pos);
            }
            if(f)f();
            if(rest_size<task_queue.size()){
                goto pop_again;
            }
        }
    }
public:
    ThreadPool(){
        for(size_t i=0;i<_PoolSize;i++){
            threads[i] = new std::thread([](ThreadPool* _this){_this->_dispatch();},this);
        }
        while(rest_size != _PoolSize);
        condition_var.notify_all();
    }

    void stop(){
        need_stop = true;
        condition_var.notify_all();
        for(size_t i=0;i<_PoolSize;i++){
            threads[i]->join();
            delete threads[i];
        }
    }
    template<typename _Fn,typename ... _Args>
    auto submit(_Fn &&f, _Args &&...args)->std::future<decltype(f(args...))>{
        std::function<decltype(f(args...))()> func = std::bind(std::forward<_Fn>(f), std::forward<_Args>(args)...);
        auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);
        task_queue.push([task_ptr]() {(*task_ptr)();});
        size_t qsize = task_queue.size();
        if(qsize>0&&qsize<=_PoolSize){
            for(size_t i=0;i<qsize;i++){
                this->condition_var.notify_one();
            }
        }
        return task_ptr->get_future();
    }

};


} // namespace TMC


#undef CUR_TYPENAME
#undef DEL_COPY_CONS
#undef DEL_MOVE_CONS
#undef DEL_COPY_ASIGN
#undef DEL_MOVE_ASIGN

#endif