#ifndef __TMC_ARRBUFFER_HPP__
#define __TMC_ARRBUFFER_HPP__

// #if __cplusplus < 201703L
// #error "please use c++17 standard"
// #endif

#include <cstdint>
#include <atomic>

namespace TMC{
template<typename _Type> class ArrBuf;
template<typename _Type> class ArrBuf_Iter;
template<typename _Type> class ArrBuf_Iter_Const;

constexpr size_t arrbuf_max_size = ~0ULL - 1;

template<typename _Type>
class ArrBuf_Iter{
    friend class ArrBuf<_Type>;
private:
    size_t cur_pos;
    ArrBuf<_Type>& owner;
    ArrBuf_Iter(ArrBuf<_Type>& _owner):cur_pos(~0ULL),owner(_owner){}
    ArrBuf_Iter(ArrBuf<_Type>& _owner,size_t pos):cur_pos(pos),owner(_owner){}
public:
    ArrBuf_Iter& operator++(){
        cur_pos++;
        if(cur_pos>=owner.size_){
            cur_pos = ~0ULL;
        }
        return *this;
    }
    ArrBuf_Iter& operator++(int){
        cur_pos++;
        if(cur_pos>=owner.size_){
            cur_pos = ~0ULL;
        }
        return *this;
    }
    ArrBuf_Iter operator+(int _size){
        cur_pos+=_size;
        if(cur_pos>=owner.size_){
            cur_pos = ~0ULL;
        }
        return *this;
    }
    ArrBuf_Iter& operator+=(int _size){
        cur_pos+=_size;
        if(cur_pos>=owner.size_){
            cur_pos = ~0ULL;
        }
        return *this;
    }
    bool operator==(ArrBuf_Iter const& other){
        return cur_pos==other.cur_pos;
    }
    bool operator!=(ArrBuf_Iter const& other){
        return cur_pos!=other.cur_pos;
    }
    _Type& operator*(){
        return owner[cur_pos];
    }
};

template<typename _Type>
class ArrBuf_Iter_Const{
    friend class ArrBuf<_Type>;
private:
    size_t cur_pos;
    ArrBuf<_Type> const& owner;
    ArrBuf_Iter_Const(ArrBuf<_Type> const& _owner)noexcept :cur_pos(~0ULL),owner(_owner) {}
    ArrBuf_Iter_Const(ArrBuf<_Type> const& _owner,size_t pos)noexcept :cur_pos(pos),owner(_owner){}
public:
    ArrBuf_Iter_Const& operator++() noexcept{
        cur_pos++;
        if(cur_pos>=owner.size_){
            cur_pos = ~0ULL;
        }
        return *this;
    }
    ArrBuf_Iter_Const& operator++(int) noexcept{
        cur_pos++;
        if(cur_pos>=owner.size_){
            cur_pos = ~0ULL;
        }
        return *this;
    }
    ArrBuf_Iter_Const operator+(int _size) noexcept{
        cur_pos+=_size;
        if(cur_pos>=owner.size_){
            cur_pos = ~0ULL;
        }
        return *this;
    }
    ArrBuf_Iter_Const& operator+=(int _size) noexcept{
        cur_pos+=_size;
        if(cur_pos>=owner.size_){
            cur_pos = ~0ULL;
        }
        return *this;
    }
    bool operator==(ArrBuf_Iter_Const const& other) noexcept{
        return cur_pos==other.cur_pos;
    }
    bool operator!=(ArrBuf_Iter_Const const& other) noexcept{
        return cur_pos!=other.cur_pos;
    }
    _Type const& operator*(){
        return owner[cur_pos];
    }
};

template<typename _Type>
class ArrBuf{
    friend class ArrBuf_Iter<_Type>;
private:
    _Type* data = nullptr;  // aways the same as capacity
    size_t size_ = 0;
    size_t capacity = 0;

    size_t _pre_grow_capacity(size_t _capacity) noexcept{
        if(_capacity==0){
            _capacity = 8;
        }else{
            _capacity +=_capacity/2;
        }
        _capacity = _capacity>arrbuf_max_size? arrbuf_max_size:_capacity;
        return _capacity;
    }
    void _apply_grow_capacity(size_t _capacity){
        _Type* new_data  = new _Type[_capacity];
        if(data && size_){
            memcpy(new_data,data,size_);
            delete[] data;
            data = nullptr;
        }
        data = new_data;
        capacity = _capacity;
    }
protected:
    void _reset(){
        if(data){
            delete[] data;
            data =nullptr;
        }
        size_ = 0;
        capacity =0;
    }
public:
    ArrBuf() noexcept {}
    ArrBuf(ArrBuf const& other){
        this->size_ = other.size_;
        this->capacity = other.capacity;
        if(other.capacity){
            this->data = new _Type[other.capacity];
        }
        if(other.size_){
            memcpy(this->data,other.data,size_);
        }
    }
    ArrBuf(ArrBuf&& other) noexcept {
        std::swap(this->capacity,other.capacity);
        std::swap(this->size_,other.size_);
        this->data = other.data;
        other.data = nullptr;
    }
    ArrBuf(_Type const* _data,size_t _size){
        push_back(_data,_size);
    }
    ArrBuf(_Type const& _data){
        push_back(_data);
    }
    ~ArrBuf(){
        _reset();
    }
    ArrBuf& operator=(ArrBuf const& other){
        _reset();
        this->size_ = other.size_;
        this->capacity = other.capacity;
        if(other.capacity){
            this->data = new _Type[other.capacity];
        }
        if(other.size_){
            memcpy(this->data,other.data,size_);
        }
        return *this;
    }
    ArrBuf& operator=(ArrBuf && other) noexcept{
        std::swap(this->capacity,other.capacity);
        std::swap(this->size_,other.size_);
        char* temp = this->data;
        this->data = other.data;
        other.data = temp;
        return *this;
    }
    _Type& operator[](size_t pos){
        return data[pos];
    }
    _Type const& operator[](size_t pos) const{
        return data[pos];
    }
    void push_back(_Type const* _data, size_t _size){
        size_t after_size = _size+size_;

        size_t _capacity = capacity;
        while(_capacity<after_size){
            _capacity = _pre_grow_capacity(_capacity);
        }
        if(_capacity!=capacity){
            _apply_grow_capacity(_capacity);
        }
        memcpy(data+size_,_data,_size);
        size_ = after_size;
    }
    void push_back(_Type const& _data){
        push_back(&_data,1);
    }
    void pop_back(size_t _size){
        size_ = _size>size_?0:size_-_size;
    }
    void push_front(_Type const* _data, size_t _size){
        size_t after_size = _size+size_;
        size_t _capacity = capacity;
        while(_capacity<after_size){
            _capacity = _pre_grow_capacity(_capacity);
        }
        if(_capacity!=capacity){
            _apply_grow_capacity(_capacity);
        }
        memcpy(data+_size,data,size_);
        memcpy(data,_data,_size);
        size_ = after_size;
    }
    void push_front(_Type const& _data){
        push_front(&_data,1);
    }
    void pop_front(size_t _size){
        size_t after_size = _size>size_?0:size_-_size;
        if(after_size){
            memcpy(data,data+_size,after_size);
        }
        size_ = after_size;
    }
    _Type const* view()const noexcept{
        return data;
    }
    size_t size()const noexcept{
        return size_;
    }
    void clear(){
        _reset();
    }
    ArrBuf_Iter<_Type> begin() noexcept{
        return ArrBuf_Iter<_Type>(*this,0);
    }
    ArrBuf_Iter<_Type> end() noexcept{
        return ArrBuf_Iter<_Type>(*this);
    }
    ArrBuf_Iter_Const<_Type> begin() const noexcept{
        return ArrBuf_Iter<_Type>(*this,0);
    }
    ArrBuf_Iter_Const<_Type> end() const noexcept{
        return ArrBuf_Iter<_Type>(*this);
    }
};


}
#endif