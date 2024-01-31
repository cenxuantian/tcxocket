#ifndef __TMC_RESULT_HPP__
#define __TMC_RESULT_HPP__

#include <tuple>
#include <string>
#include <iostream>
#include <ctime>
#include <functional>
#include <sstream>

#define TMC_R_CALL_POS(ec) TMC::_R_CallInfo{TMC::_R_datetime(),__FUNCTION__,__FILE__,__LINE__,ec}

namespace TMC{

// retrive the first param from template params
template<typename _1st = void, typename ...T>
struct GetFirstParam{
typedef _1st type;
};
template<typename _1st>
struct GetFirstParam<_1st>{
typedef _1st type;
};

struct _R_CallInfo{
    std::string time = "";
    std::string func = "";
    std::string file = "";
    int line = 0;
    int ec = 0;
};

// if (_Tp == void)
//      type = bool
// else 
//      type = _Tp
template<typename _Tp>
struct IfVoid2Bool{
    typedef _Tp type;
};
template<>
struct IfVoid2Bool<void>{
    typedef bool type;
};



// can be out put to ostream
// for Result
template<typename T, typename = void>
constexpr bool _R_is_outputable = false;
template<typename T>
constexpr bool _R_is_outputable<T,std::void_t<decltype(std::declval<std::ostream&>() << std::declval<T&>())>> = true;

template<typename T>
using SimplePrint_InType_Fmt = std::conditional_t<std::is_pointer_v<std::decay<T>>,T,T const&>;

// this is a simple println func-like struct
// can be used for all type
// if type is not outputable, then ignore it
template<typename _First,typename ..._Args>
struct SimplePrint{
    std::ostream& os_;
    SimplePrint(std::ostream& os,_First const& first,_Args const& ...args):os_(os){
        if constexpr(_R_is_outputable<_First>){
            os << first;
        }else{
            os << "[Type not outputable]";
        }
        SimplePrint<_Args...>(os,args...);
    }
    void flush(){
        os_.flush();
    }
};
template<typename _First>
struct SimplePrint<_First>{
    std::ostream& os_;
    SimplePrint(std::ostream& os,_First const& first):os_(os){
        if constexpr(_R_is_outputable<_First>){
            os << first;
        }else{
            os << "[Type not outputable]";
        }
    }
    void flush(){
        os_.flush();
    }
};

inline std::string _R_datetime(){
    std::stringstream ss;
    time_t now = ::time(0);
    tm* ltm = ::localtime(&now);
    ss << (ltm->tm_year+1900)<<'-';
    if(ltm->tm_mon+1<10){
        ss << '0'<<(ltm->tm_mon+1)<<'-';
    }else{
        ss << (ltm->tm_mon+1)<<'-';
    }
    if(ltm->tm_mday<10){
        ss << '0'<<ltm->tm_mday <<' ';
    }else{
        ss <<ltm->tm_mday<<' ';
    }
    
    if(ltm->tm_hour<10){
        ss << '0'<<ltm->tm_hour <<':';
    }else{
        ss <<ltm->tm_hour <<':';
    }
    if(ltm->tm_min<10){
        ss << '0'<<ltm->tm_min <<':';
    }else{
        ss <<ltm->tm_min <<':';
    }
    if(ltm->tm_sec<10){
        ss << '0'<<ltm->tm_sec;
    }else{
        ss <<ltm->tm_sec;
    }
    return ss.str();
    
    //<<(ltm->tm_mon+1)<<(ltm->tm_year+1900);
};

// the type must be copied to construct for Result
template<typename T>
constexpr bool _R_must_copy = ((!std::is_move_assignable_v<std::decay_t<T>>)
                                    &&(std::is_copy_assignable_v<std::decay_t<T>>)) // can be copied and cannot be moved
                                || 
                                std::is_pointer_v<std::decay_t<T>>;

// this class holds values of returns
// if _Types are moveable and is not a pointer,
// you must pass a rvalue into constrcutor
// else you can pass lvalue into constructor
template<typename ..._Types>
class Result{
public:
    using DataType =  
    std::conditional_t<sizeof...(_Types) == 0,
        bool,                                       // when sizeof...(_Types) == 0
        std::conditional_t<sizeof...(_Types) == 1,  
            typename IfVoid2Bool<std::decay_t<typename GetFirstParam<_Types...>::type>>::type,
                                                    // when sizeof...(_Types) == 1 && this type is not void
            std::tuple<std::decay_t<_Types>...>>                  // when sizeof...(_Types) > 1
    >;

    using ConstructType = 
        std::conditional_t<_R_must_copy<DataType>,
            std::conditional_t<std::is_pointer_v<DataType>,
                DataType const,         // for pointer types                // copy constructor
                DataType const&>,       // for types that must be copied    // copy constructor
                DataType &&>;           // for types moveable               // move constructor
    
private:
    bool result_ = false;
    DataType data_;
    _R_CallInfo call_info_;

    template<typename ..._Args>
    void __print_err(_Args const& ...args){
        SimplePrint(std::cerr,
            "RESULT ERROR with error code: ",call_info_.ec,'\n',
            "\tMessage: ",args...,'\n',
            "\tTime: ",call_info_.time,'\n',
            "\tLast call position: ",call_info_.file,':',call_info_.line,'\n',
            "\tIn function: ",call_info_.func,'\n').flush();
    }

public:
    ~Result()noexcept(std::is_nothrow_destructible_v<DataType>){}
    // DataType data_ will call its default constructor
    Result(bool res)
        noexcept(std::is_nothrow_default_constructible_v<DataType>) 
        :result_(res)
        ,data_()
        ,call_info_(){}
    // DataType data_ will call its default constructor
    Result(bool res,_R_CallInfo&& _call_info)
        noexcept(std::is_nothrow_default_constructible_v<DataType>) 
        :result_(res)
        ,data_()
        ,call_info_(_call_info){}
    // DataType data_ will call its default move constructor
    Result(bool res,ConstructType data) 
        noexcept(std::is_nothrow_move_constructible_v<DataType>) 
        :result_(res)
        ,data_(data)
        ,call_info_(){}
    // DataType data_ will call its default move constructor
    Result(bool res,ConstructType data,_R_CallInfo&& _call_info) 
        noexcept(std::is_nothrow_move_constructible_v<DataType>) 
        :result_(res)
        ,data_(data)
        ,call_info_(_call_info){}


    // DataType data_ will call its default constructor
    static Result ok()
        noexcept(std::is_nothrow_default_constructible_v<DataType>)
    {
        return true;
    }
    // DataType data_ will call its default move constructor
    static Result ok(ConstructType data)
        noexcept(std::is_nothrow_move_constructible_v<DataType>) 
    {
        return Result<_Types...>(true,std::forward<DataType>(data));
    }
    // DataType data_ will call its default constructor
    static Result err(_R_CallInfo && err_info)
        noexcept(std::is_nothrow_default_constructible_v<DataType>)
    {
        return Result<_Types...>(false,std::forward<_R_CallInfo>(err_info)); 
    }
    // DataType data_ will call its default move constructor
    static Result err(ConstructType data,_R_CallInfo && err_info)
        noexcept(std::is_nothrow_move_constructible_v<DataType>) 
    {
        return Result<_Types...>(false,std::forward<DataType>(data)); 
    }
    
    
    // get the value,log errmsg into stderr no throw
    // get the value, if error throw first
    template<typename ..._Args>
    auto& except(_Args const& ...args){
        if(!result_){
            __print_err(args...);
            throw call_info_.ec;
        }
        return ignore();
    }
    // get the value,log errmsg into stderr no throw
    template<typename ..._Args>
    auto& no_except (_Args const& ...args) noexcept{
        if(!result_){
            __print_err(args...);
        }
        return ignore();
    }
    // get the value, throw -1
    auto& unwrap(){
        if(!result_)throw call_info_.ec;
        return ignore();
    }
    // get the value anyway
    auto& ignore() noexcept{
        const size_t pack_size = sizeof...(_Types);
        if constexpr (pack_size == 0){
            return result_;
        }
        else if constexpr (pack_size ==1){
            if constexpr(std::is_same<void,_Types...>::value){
                return result_;
            }else{
                return data_;
            }
        }
        else {
            return data_;
        }
    }
    
    
    // get the value,log errmsg into stderr no throw
    template<typename ..._Args>
    const auto& no_except (_Args const& ...args)const noexcept{
        if(!result_){
            __print_err(args...);
        }
        return ignore();
    }
    // get the value,log errmsg into stderr no throw
    // get the value, if error throw first
    template<typename ..._Args>
    const auto& except(_Args const& ...args) const{
        if(!result_){
            __print_err(args...);
            throw call_info_.ec;
        }
        return ignore();
    }
    // get the value, throw on error
    const auto& unwrap() const{
        if(!result_)throw call_info_.ec;
        return ignore();
    }
    // get the value anyway
    const auto& ignore() const noexcept{
        const size_t pack_size = sizeof...(_Types);
        if constexpr (pack_size == 0){
            return result_;
        }
        else if constexpr (pack_size ==1){
            if constexpr(std::is_same<void,_Types...>::value){
                return result_;
            }else{
                return data_;
            }
        }
        else {
            return data_;
        }
    }
    
    // check if there is an error
    bool check()const noexcept{
        return result_;
    }
    
    operator bool()const noexcept{
        return this->result_;
    }

    bool is_ok()const noexcept{
        return result_;
    }
    bool is_err()const noexcept{
        return !result_;
    }
    
    // change the result to error or success
    void change_res(bool _res) noexcept{
        result_ = _res;
    }

    void set_ec(int ec) noexcept{
        call_info_.ec = ec;
    }
    
    Result or_else(std::function<Result()> const&f){
        if(!result_) return f();
        return std::move(*this);
    }

    Result and_then(std::function<Result()> const&f){
        if(result_) return f();
        return std::move(*this);
    }

    template<typename _Fn>
    auto then(_Fn && f) ->decltype(f()){
        return f();
    }

};

// template<>
// Result<void>::Result(bool res,Result<void>::ConstructTypeLRef data)= delete;

}


#endif