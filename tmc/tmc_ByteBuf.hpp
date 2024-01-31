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