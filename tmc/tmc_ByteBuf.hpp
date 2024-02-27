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

#ifndef __TMC_BYTEBUFFER_HPP__
#define __TMC_BYTEBUFFER_HPP__

#include "tmc_ArrBuf.hpp"

#include <string>
#include <cstring>

namespace TMC
{
typedef unsigned char Byte;
inline constexpr size_t npos = ~0ULL;

class ByteBuf : public ArrBuf<Byte>{
public:
    ByteBuf()noexcept :ArrBuf<Byte>() {}
    ByteBuf(std::string const& str):ArrBuf<Byte>(){
        push_back((Byte*)str.data());
    }
    ByteBuf(Byte const* str):ArrBuf<Byte>(){
        push_back(str);
    }
    ByteBuf(ByteBuf const& other):ArrBuf<Byte>(other){}
    ByteBuf(ByteBuf && other)noexcept :ArrBuf<Byte>(other){}

    ByteBuf& operator=(ByteBuf const& other){
        this->ArrBuf<Byte>::operator=(other);
        return *this;
    }
    ByteBuf& operator=(ByteBuf && other){
        this->ArrBuf<Byte>::operator=(other);
        return *this;
    }
    ByteBuf& operator=(std::string const& str){
        _reset();
        push_back((Byte*)str.data());
        return *this;
    }
    ByteBuf& operator=(Byte const* str){
        _reset();
        push_back(str);
        return *this;
    }
    
    Byte const* bytes()const{
        const_cast<ByteBuf*>(this)->ArrBuf::push_back(0);
        const_cast<ByteBuf*>(this)->ArrBuf::pop_back(1);
        return view();
    }
    const char* c_str()const {
        return (const char*)bytes();
    }
    void push_back(Byte const* _data){
        ArrBuf::push_back(_data,strlen((char const*)_data));
    }
    void push_front(Byte const* _data){
        ArrBuf::push_front(_data,strlen((char const*)_data));
    }
    ByteBuf slice(size_t start_pos,size_t count = npos){
        ByteBuf res = *this;
        res.pop_front(start_pos);
        if(count!=npos){
            size_t res_size = res.size();
            if(res_size>count){
                res.pop_back(res_size-count);
            }
        }
        return res;
    }
};
} // namespace TMC



#endif