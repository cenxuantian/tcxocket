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


#ifndef __TMC_LOCKFREEQ_HPP__
#define __TMC_LOCKFREEQ_HPP__

// #if __cplusplus < 201703L
// #error "please use c++17 standard"
// #endif

#include <cstdint>
#include <type_traits>
#include <utility>
#include <atomic>

namespace TMC
{
template<typename _Type,size_t _Size>
class LockfreeQ{
private:
    struct _LockfreeQImpl{
        size_t cur_start = 0;
        size_t cur_end = 0;
        size_t cur_size = 0;
    };
    _Type data[_Size];  // ring buffer
    volatile std::atomic<_LockfreeQImpl> impl;
    
    void _update_data_copy(_Type const& _in_data,size_t index){
        if constexpr(std::is_move_assignable_v<_Type>) 
            data[index] = _in_data;
        else memcpy(data+index,&_in_data,sizeof(_Type));
    }
    
    // return 0 the impl after push
    // return 1 the pos should be modified
    static std::pair<_LockfreeQImpl,size_t> _next_push(_LockfreeQImpl const& _before_impl) noexcept{
        std::pair<_LockfreeQImpl,size_t> res{_before_impl,0};

        if(res.first.cur_end==res.first.cur_start && res.first.cur_size!=0){
            res.first.cur_size--;
            res.first.cur_start++;
            res.first.cur_start = res.first.cur_start==_Size?0:res.first.cur_start;
        }
        res.second = res.first.cur_end;
        res.first.cur_end++;
        res.first.cur_end = res.first.cur_end==_Size?0:res.first.cur_end;
        res.first.cur_size++;
        return res;
    }
    // return 0 the impl after pop
    // return 1 the pos should be returned
    static std::pair<_LockfreeQImpl,size_t> _next_pop(_LockfreeQImpl const& _before_impl) noexcept{
        if(!_before_impl.cur_size){
            return {_LockfreeQImpl(),~(0ULL)};
        }
        std::pair<_LockfreeQImpl,size_t> res{_before_impl,0};
        res.second = res.first.cur_start;
        res.first.cur_start++;
        res.first.cur_start = res.first.cur_start==_Size?0:res.first.cur_start;
        res.first.cur_size--;
        return res;
    }

    // get the pos that needed to be pushed
    // and update the impl
    size_t _atomic_push_pos() noexcept {
        _LockfreeQImpl save_impl;
        std::pair<_LockfreeQImpl,size_t> _pair_res;
        while(1){
            save_impl= impl.load();
            _pair_res= _next_push(save_impl);
            if(impl.compare_exchange_strong(save_impl,_pair_res.first)){
                break;
            }
        }
        return _pair_res.second;
    }

    // get the pos that needed to be returned
    // and update the impl
    size_t _atomic_pop_pos() noexcept{
        _LockfreeQImpl save_impl;
        std::pair<_LockfreeQImpl,size_t> _pair_res;
        while(1){
            save_impl= impl.load();
            _pair_res= _next_pop(save_impl);
            if(impl.compare_exchange_strong(save_impl,_pair_res.first)){
                break;
            }
        }
        return _pair_res.second;
    }

public:
    void push(_Type const& _in_ldata){
        _update_data_copy(_in_ldata,_atomic_push_pos());
    }
    // if return ~0ULL if not safe
    size_t safe_pop_pos() noexcept {
        return _atomic_pop_pos();
    }
    _Type safe_pop(size_t pos) noexcept {
        return std::move(data[pos]);
    }
    size_t size()const noexcept{
        return impl.load().cur_size;
    }
    size_t capacity()const noexcept{
        return _Size;
    }
    void clear() noexcept{
        impl.store(_LockfreeQImpl());
    }
    
};

}


#endif


