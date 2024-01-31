#ifndef __TMC_SOCKET_HPP__
#define __TMC_SOCKET_HPP__


#include "tmc_ByteBuf.hpp"
#include "tmc_Result.hpp"

#ifdef _WIN32
#define MSG_NOSIGNAL 0
#define be64toh _bswap64
#define htobe64 _bswap64
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <windows.h>
#elif defined(__linux__)
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#define SD_BOTH SHUT_RDWR
#define SOCKET int
#define closesocket(_sock) close(_sock)
#include <arpa/inet.h>
#include <errno.h>
#endif

#include <utility>
#include <tuple>
#include <chrono>
#include <memory>

#define ROUTE_SOCK_OPT(_optname,_level,_type)\
template<> struct GetSockOptDetails<_optname>{\
    typedef _type type;\
    static const int level = _level;\
    static const int optname = _optname;}

namespace TMC{

class IPAddr;
class Socket;

template<int _opt> struct GetSockOptDetails;

inline int sys_errno();




inline int sys_errno(){
#ifdef __linux__
    return errno;
#elif defined(_WIN32)
    return GetLastError();
#endif
}

class IPAddr{
    friend class Socket;
public:
    enum class Type: int;
public:
    enum class Type: int{
        V4 = 0,
        V6 = 1,
    };
private:
    sockaddr_in addr_in_;
    Type type_ = Type::V4;
public:
    static IPAddr v4(const char* host,u_short port){
        IPAddr res;
        res.addr_in_.sin_family = AF_INET;
        res.addr_in_.sin_port = port;
#ifdef _WIN32
        res.addr_in_.sin_addr.S_un.S_addr = inet_addr(host);
#elif defined(__linux__)
        res.addr_in_.sin_addr.s_addr =inet_addr(host);
#endif
        return res;
    }
    static IPAddr v4(sockaddr_in const& addr_in){
        IPAddr res;
        ::memcpy(&res.addr_in_,&addr_in,sizeof(sockaddr_in));
        return res;
    }
    Type type()const noexcept{
        return type_;
    }
    std::string host()const{
        return ::inet_ntoa(addr_in_.sin_addr);
    }
    u_short port()const noexcept{
        return addr_in_.sin_port;
    }
};


template<int _opt> struct GetSockOptDetails{
    typedef void type;
    static const int level = -1;
    static const int optname = -1;
};
ROUTE_SOCK_OPT(SO_BROADCAST,SOL_SOCKET,int);
ROUTE_SOCK_OPT(SO_DONTROUTE,SOL_SOCKET,int);
ROUTE_SOCK_OPT(SO_ERROR,SOL_SOCKET,int);
ROUTE_SOCK_OPT(SO_KEEPALIVE,SOL_SOCKET,int);
ROUTE_SOCK_OPT(SO_LINGER,SOL_SOCKET,linger);
ROUTE_SOCK_OPT(SO_OOBINLINE,SOL_SOCKET,int);
ROUTE_SOCK_OPT(SO_RCVBUF,SOL_SOCKET,int);
ROUTE_SOCK_OPT(SO_SNDBUF,SOL_SOCKET,int);
ROUTE_SOCK_OPT(SO_RCVLOWAT,SOL_SOCKET,int);
ROUTE_SOCK_OPT(SO_SNDLOWAT,SOL_SOCKET,int);
ROUTE_SOCK_OPT(SO_RCVTIMEO,SOL_SOCKET,timeval);
ROUTE_SOCK_OPT(SO_SNDTIMEO,SOL_SOCKET,timeval);
ROUTE_SOCK_OPT(SO_REUSEADDR,SOL_SOCKET,int);
ROUTE_SOCK_OPT(SO_TYPE,SOL_SOCKET,int);

// ROUTE_SOCK_OPT(IP_HDRINCL,IPPROTO_IP,int);
// ROUTE_SOCK_OPT(IP_OPTINOS,IPPROTO_IP,int);
// ROUTE_SOCK_OPT(IP_TOS,IPPROTO_IP,int);
// ROUTE_SOCK_OPT(IP_TTL,IPPROTO_IP,int);

// ROUTE_SOCK_OPT(TCP_MAXSEG,IPPROTO_TCP,int);
ROUTE_SOCK_OPT(TCP_NODELAY,IPPROTO_TCP,int);

class Socket{
public:
    enum class Protocol: int;
    enum class ShutdownType :int;
public:
    enum class Protocol: int{
        P_OTHER = -1,
        TCP = 0x00,
        UDP = 0x01,
    };
    enum class ShutdownType :int{
        SDT_RECV = 0,
        SDT_SEND = 1,
        SDT_BOTH = 2,
    };
private:
    bool valid_ = false;
    SOCKET h_sock_ = INVALID_SOCKET;
    Protocol protocol_;
    IPAddr addr_;

    Socket()noexcept {}//hide
    Socket(Protocol _protocol):protocol_(_protocol) {//hide
        switch (_protocol)
        {
        case Protocol::TCP:
            h_sock_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            break;
        case Protocol::UDP:
            h_sock_ = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            break;
        }
        if(h_sock_ != INVALID_SOCKET){
            this->valid_ = true;
        }
    }
    Socket(int af,int type, int _protocol):protocol_(Protocol::P_OTHER){
        h_sock_ = ::socket(af, type, _protocol);
        if(h_sock_ != INVALID_SOCKET){
            this->valid_ = true;
        }
    }

    // this func will call ::send or ::sendto
    // param buf content to send
    // param _fn function to call (send / sendto)
    // param args if sendto, the target
    template<typename _Fn,typename ..._Args>
    Result<int> __write(ByteBuf const& buf,_Fn &&_fn, _Args &&...args){
        int ret = _fn(h_sock_,(const char*)buf.view(),(int)buf.size(),MSG_NOSIGNAL,std::forward<_Args>(args)...);
        if(ret==SOCKET_ERROR){
            return Result<int>(false,{0});
        }
        return Result<int>(true,std::move(ret),TMC_R_CALL_POS(exact_err().ignore()));
    }

    // this func will call ::send or ::sendto
    // param buf content to send
    // param _fn function to call (send / sendto)
    // param args if sendto, the target
    // make sure all buf has been write
    template<typename _Fn,typename ..._Args>
    Result<void> __write_all(std::chrono::milliseconds const& _timeout,ByteBuf const& buf,_Fn &&_fn, _Args &&...args){
        auto write_buf_size_res = get_write_bufsize();
        if(!write_buf_size_res.check()){
            return {false,TMC_R_CALL_POS(exact_err().ignore())};
        }
        size_t write_buf_size = write_buf_size_res.ignore();
        const char* content = (const char*)buf.view();
        size_t offset = 0;
        size_t left_size = buf.size();

        try_send:
        if(left_size<write_buf_size){
            while (!writeable().ignore());
            int ret = ret = _fn(h_sock_,content+offset,(int)left_size,MSG_NOSIGNAL,std::forward<_Args>(args)...);
            if(ret == SOCKET_ERROR)return false;
            else if(left_size == ret){
                return true;
            }else{
                offset+=ret;
                left_size-=ret;
                goto try_send;
            }
        }else{
            while (!writeable().ignore());
            int ret = _fn(h_sock_,content+offset,(int)write_buf_size,MSG_NOSIGNAL,std::forward<_Args>(args)...);
            if(ret == SOCKET_ERROR)return {false,TMC_R_CALL_POS(exact_err().ignore())};
            offset+=ret;
            left_size-=ret;
            goto try_send;
        }
        return true;
    }

    // this func will call ::recv or ::recvfrom
    // param buf content to send
    // param _fn function to call (recv / recvfrom)
    // param args if recvfrom, the target
    template<typename _Fn,typename ..._Args>
    Result<ByteBuf> __readsome(int _expect_size,_Fn &&_fn, _Args &&...args){
        std::unique_ptr<char> buf(new char[_expect_size]{0});
        int ret = _fn(h_sock_,buf.get(),_expect_size,0,std::forward<_Args>(args)...);
        if(ret == SOCKET_ERROR){
            return {false,TMC_R_CALL_POS(exact_err().ignore())};
        }

        Result<ByteBuf> res(true);
        res.ignore().ArrBuf<Byte>::push_back((Byte const*)buf.get(),ret);
        return res;
    }

    // this func will call ::recv or ::recvfrom
    // param buf content to send
    // param _fn function to call (recv / recvfrom)
    // param args if recvfrom, the target
    // make sure all buf has been read
    // will block if not enough
    template<typename _Fn,typename ..._Args>
    Result<ByteBuf> __readall(std::chrono::milliseconds const& _timeout,int _size,_Fn &&_fn, _Args &&...args){
        auto start = std::chrono::system_clock::now();

        auto read_buf_size_res = get_read_bufsize();
        if(!read_buf_size_res.check()){
            return {false,TMC_R_CALL_POS(exact_err().ignore())};
        }
        int read_buf_size = read_buf_size_res.ignore();
        Result<ByteBuf> final_res(true);

        bool wait_forever;
        if (_timeout == std::chrono::milliseconds(0)){
            wait_forever = true;
        }else{
            wait_forever = false;
        }
        
        try_read:
        if(_size<read_buf_size){
            if(_size<=0){
                return final_res;
            }
            if(wait_forever){
                if(!await_readable(_timeout).check()) return {false,TMC_R_CALL_POS(exact_err().ignore())};
            }else{
                auto past_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);
                auto left_time = _timeout<=past_time?std::chrono::milliseconds(0):_timeout-past_time;
                if(left_time != std::chrono::milliseconds(0)){
                    auto wait_res = await_readable(left_time);
                    if(!wait_res.check()){
                        return {false,TMC_R_CALL_POS(exact_err().ignore())};// await error
                    }else if(!wait_res.ignore()){// await time out
                        return final_res;
                    }
                }else{// await time out
                    return final_res;
                }
            }
            std::unique_ptr<char> buf(new char[_size]{0});
            int ret =  _fn(h_sock_,buf.get(),_size,0,std::forward<_Args>(args)...);
            if(ret == SOCKET_ERROR){
                return false;
            }else{
                final_res.ignore().ArrBuf<Byte>::push_back((Byte const*)buf.get(),ret);
            }
            if(ret <_size){
                _size-=ret;
                goto try_read;
            }else{
                return final_res;
            }
        }else{
            if(wait_forever){
                if(!await_readable(_timeout).check()) return {false,TMC_R_CALL_POS(exact_err().ignore())};
            }else{
                auto past_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);
                auto left_time = _timeout<=past_time?std::chrono::milliseconds(0):_timeout-past_time;
                if(left_time != std::chrono::milliseconds(0)){
                    auto wait_res = await_readable(left_time);
                    if(!wait_res.check()){
                        return false;// await error
                    }else if(!wait_res.ignore()){// await time out
                        return final_res;
                    }
                }else{// await time out
                    return final_res;
                }
            }
            std::unique_ptr<char> buf(new char[_size]{0});
            int ret =  _fn(h_sock_,buf.get(),read_buf_size,0,std::forward<_Args>(args)...);
            if(ret == SOCKET_ERROR){
                return {false,TMC_R_CALL_POS(exact_err().ignore())};
            }else{
                final_res.ignore().ArrBuf<Byte>::push_back((Byte const*)buf.get(),ret);
            }
            _size-=ret;
            goto try_read;
        }

        return {false,TMC_R_CALL_POS(fast_err())};
    }


public:
    // start the environment of running a socket
    // if on windows, you need to run this function on each thread
    static Result<void> env_start(){
#ifdef _WIN32
        WSADATA wsadata;
        if(WSAStartup(MAKEWORD(2,2), &wsadata)!= 0){
            return {false,TMC_R_CALL_POS(fast_err())};
        }
#endif
        return true;
    }
    
    // stop the environment of running a socket
    // if on windows, you need to run this function on each thread
    static Result<void> env_stop(){
#ifdef _WIN32
        if(WSACleanup()){
            return {false,TMC_R_CALL_POS(fast_err())};
        }
#endif
        return true;
    }
    
    // create a socket
    static Result<Socket> create(Protocol _protocol){
        Socket res(_protocol);
        return Result<Socket>(res.valid_,std::move(res),TMC_R_CALL_POS(fast_err()));
    }
    
    // create a socket in native style
    static Result<Socket> create(int af,int type, int _protocol){
        Socket res(af,type,_protocol);
        return Result<Socket>(res.valid_,std::move(res),TMC_R_CALL_POS(fast_err()));
    }

    // check if the socket is valid currently
    bool valid()const noexcept{
        return this->valid_;
    }
    
    // get the errno on linux or the result of GetLastError on windows
    // this number is always the last error of the system
    // please use exact_err instead when use multi-thread scheme
    static int fast_err(){
        return sys_errno();
    }
    
    // get the native socket handle 
    SOCKET native_handle()const noexcept{
        return h_sock_;
    }
    
    // get the recorded address if called bind
    IPAddr const& address()const noexcept{
        return this->addr_;
    }
    
    // get the protocol
    Protocol protocol()const noexcept{
        return this->protocol_;
    }

    // socet connect function
    // only valid for tcp
    Result<void> connect(IPAddr const& addr){
        int ret = ::connect(h_sock_,(sockaddr*)&addr.addr_in_,sizeof(sockaddr_in));
        
        if(ret!= SOCKET_ERROR){
            sockaddr_storage storage;
#ifdef __linux__
            socklen_t sock_len = sizeof(sockaddr_storage);
#elif defined(_WIN32)
            int sock_len = sizeof(sockaddr_storage);
#endif
            ret = ::getsockname(h_sock_,(sockaddr*)&storage,&sock_len);
            if(ret == SOCKET_ERROR){
                return {false,TMC_R_CALL_POS(exact_err().ignore())};
            }else{
                this->addr_ = IPAddr::v4(*((sockaddr_in*)&storage));
                return true;
            }
        }else{
            return {false,TMC_R_CALL_POS(fast_err())};
        }
    }
    
    // socet sut down function
    Result<void> shutdown(ShutdownType type = ShutdownType::SDT_BOTH){
        bool res = ::shutdown(h_sock_,(int)type)!=SOCKET_ERROR;
        return {res,TMC_R_CALL_POS(res?0:fast_err())};
    }
    
    // socet close function
    Result<void> close(){
        int ret = ::closesocket(h_sock_);
        if(ret!=SOCKET_ERROR){
            this->valid_ = false;
        }
        return {ret!=SOCKET_ERROR,TMC_R_CALL_POS(exact_err().ignore())};
    }
    
    // socket bind function
    Result<void> bind(IPAddr const& addr){
        bool success = ::bind(h_sock_,(sockaddr*)&addr.addr_in_,sizeof(sockaddr_in)) != SOCKET_ERROR;
        this->addr_ = addr;
        return {success,TMC_R_CALL_POS(exact_err().ignore())};
    }
    
    // socket listen function
    Result<void> listen(){
        return {::listen(h_sock_,1)!=SOCKET_ERROR,TMC_R_CALL_POS(exact_err().ignore())};
    }
    
    // socket accept function
    Result<Socket> accept(){
        Socket res(this->protocol_);
        sockaddr_in addr;
#ifdef __linux__
        socklen_t addr_len = sizeof(sockaddr_in);
#elif defined(_WIN32)
        int addr_len = sizeof(sockaddr_in);
#endif
        res.h_sock_ =  ::accept(h_sock_,(sockaddr*)&addr,&addr_len);
        if(res.h_sock_ != INVALID_SOCKET){
            res.valid_ = true;
        }
        memcpy(&(res.addr_.addr_in_),&addr,sizeof(sockaddr_in));
        if(res.addr_.addr_in_.sin_family == AF_INET){
            res.addr_.type_ = IPAddr::Type::V4;
        }else if(res.addr_.addr_in_.sin_family == AF_INET6){
            res.addr_.type_ = IPAddr::Type::V6;
        }
        return Result<Socket>(res.valid_,std::move(res),TMC_R_CALL_POS(exact_err().ignore()));
    }
    
    // socket send funtion
    // return size writen
    // param 0 message to write    
    Result<int> write(ByteBuf const& buf){
        return __write(buf,::send);
    }

    // make sure write all the buffer content
    Result<void> write_all(ByteBuf const& buf,std::chrono::milliseconds const& _timeout = std::chrono::milliseconds(0)){
        return __write_all(_timeout,buf,::send);
    }

    // socket sendto funtion
    // return size writen
    // param 0 message to write
    // param 1 target of udp
    Result<int> write_to(ByteBuf const& buf,IPAddr const& tar){
        return __write(buf,::sendto,(sockaddr*)&tar.addr_in_,(int)sizeof(sockaddr_in));
    }
    
    // make sure write all the buffer content
    Result<void> write_all_to(ByteBuf const& buf, IPAddr const& tar,std::chrono::milliseconds const& _timeout = std::chrono::milliseconds(0)){
        return __write_all(_timeout,buf,::sendto,(sockaddr*)&tar.addr_in_,(int)sizeof(sockaddr_in));
    }


    // socket recv function
    // param 0 size to read
    // if data in read buf is not enough will return ok(readsize)
    Result<ByteBuf> readsome(int _expect_size){
        return __readsome(_expect_size,::recv);
    }
    
    // socet recvfrom function
    // param 0 size to read
    // param 1 target of udp
    // if data in read buf is not enough will return ok(readsize)
    Result<ByteBuf> readsome_from(int _expect_size, IPAddr const& tar){
#ifdef __linux__
        socklen_t addr_len = sizeof(sockaddr_in);
#elif defined(_WIN32)
        int addr_len = sizeof(sockaddr_in);
#endif
        return __readsome(_expect_size,::recvfrom,(sockaddr*)&tar.addr_in_,&addr_len);
    }
    
    Result<ByteBuf> readall(int _expect_size,std::chrono::milliseconds const& _timeout = std::chrono::milliseconds(0)){
        return __readall(_timeout,_expect_size,::recv);
    }

    Result<ByteBuf> readall_from(int _expect_size, IPAddr const& tar,std::chrono::milliseconds const& _timeout = std::chrono::milliseconds(0)){
#ifdef __linux__
        socklen_t addr_len = sizeof(sockaddr_in);
#elif defined(_WIN32)
        int addr_len = sizeof(sockaddr_in);
#endif
        return __readall(_timeout,_expect_size,::recvfrom,(sockaddr*)&tar.addr_in_,&addr_len);
    }

    // socket select function
    // return 0 error
    // return 1 recv
    // return 2 send
    Result<bool,bool,bool> select(std::chrono::milliseconds const& _timeout){
        std::tuple<bool,bool,bool> res = {false,false,false};
        auto _ms = _timeout.count();
        timeval _tval;
        _tval.tv_sec = (decltype(timeval::tv_sec))_ms/1000;
        _tval.tv_usec = (decltype(timeval::tv_usec))_ms%1000;
        fd_set _recv_set;
        fd_set _send_set;
        fd_set _err_set;
        FD_ZERO(&_recv_set);
        FD_ZERO(&_send_set);
        FD_ZERO(&_err_set);
        FD_SET(h_sock_,&_recv_set);
        FD_SET(h_sock_,&_send_set);
        FD_SET(h_sock_,&_err_set);
        int _ret = ::select((int)h_sock_+1,&_recv_set,&_send_set,&_err_set,&_tval);
        if(_ret == SOCKET_ERROR){
            return Result<bool,bool,bool>(false,std::move(res),TMC_R_CALL_POS(exact_err().ignore()));
        }
        else{
            if(FD_ISSET(h_sock_,&_recv_set)){// read
                std::get<1>(res) = true;
            }
            if(FD_ISSET(h_sock_,&_send_set)){
                std::get<2>(res) = true;
            }
            if(FD_ISSET(h_sock_,&_err_set)){
                std::get<0>(res) = true;
            }
            return Result<bool,bool,bool>(true,std::move(res),TMC_R_CALL_POS(exact_err().ignore()));
        }
    }

    // use select to check if the read buf is currently avaliable
    // Result can be ignored, will return false on error
    Result<bool> readable(){
        timeval _tval;
        _tval.tv_sec = 0;
        _tval.tv_usec = 0;
        fd_set set;
        fd_set _err_set;
        FD_ZERO(&set);
        FD_ZERO(&_err_set);
        FD_SET(h_sock_,&set);
        FD_SET(h_sock_,&_err_set);
        if(::select((int)h_sock_+1,&set,0,&_err_set,&_tval) == SOCKET_ERROR){
            return Result<bool>(false,false);
        }
        if(FD_ISSET(h_sock_,&_err_set)){
            if(FD_ISSET(h_sock_,&set)){// read
                return Result<bool>(false,true,TMC_R_CALL_POS(exact_err().ignore()));
            }else{
                return Result<bool>(false,false,TMC_R_CALL_POS(exact_err().ignore()));
            }
        }else{
            if(FD_ISSET(h_sock_,&set)){// read
                return Result<bool>(true,true);
            }else{
                return Result<bool>(true,false);
            }
        }
    }

    // use select to check if the write buf is currently avaliable
    // Result can be ignored, will return false on error
    Result<bool> writeable(){
        timeval _tval;
        _tval.tv_sec = 0;
        _tval.tv_usec = 0;
        fd_set set;
        fd_set _err_set;
        FD_ZERO(&set);
        FD_ZERO(&_err_set);
        FD_SET(h_sock_,&set);
        FD_SET(h_sock_,&_err_set);
        if(::select((int)h_sock_+1,0,&set,&_err_set,&_tval) == SOCKET_ERROR){
            return Result<bool>(false,false);
        }
        if(FD_ISSET(h_sock_,&_err_set)){
            if(FD_ISSET(h_sock_,&set)){// read
                return Result<bool>(false,true,TMC_R_CALL_POS(exact_err().ignore()));
            }else{
                return Result<bool>(false,false,TMC_R_CALL_POS(exact_err().ignore()));
            }
        }else{
            if(FD_ISSET(h_sock_,&set)){// read
                return Result<bool>(true,true);
            }else{
                return Result<bool>(true,false);
            }
        }
    }

    // wait until sock is readable
    // return ok(true) if readable after timeout
    // return ok(false) if readable after timeout
    // return err(false) 
    // if timeout is 0 wait forever
    // if you do not want to wait, please use readable
    Result<bool> await_readable(std::chrono::milliseconds const& timeout){
        if(timeout == std::chrono::milliseconds(0)){
            // wait forever
            try_readable:{
                Result<bool> readable_res = readable();
                if(readable_res.check()){
                    if(readable_res.ignore()){
                        return Result<bool>::ok(true);
                    }else{
                        goto try_readable;
                    }
                }else{
                    return Result<bool>::err(false,TMC_R_CALL_POS(exact_err().ignore()));
                }
            }
        }else{
            size_t count = timeout.count();
            timeval _tval;
            _tval.tv_sec = (int)count/1000;
            _tval.tv_usec = (int)count%1000;
            fd_set set;
            FD_ZERO(&set);
            FD_SET(h_sock_,&set);
            if(::select((int)h_sock_+1,&set,0,0,&_tval) == SOCKET_ERROR){
                return Result<bool>::err(false,TMC_R_CALL_POS(exact_err().ignore()));
            }
            if(FD_ISSET(h_sock_,&set)){// read
                return Result<bool>::ok(true);
            }else{
                return Result<bool>::ok(false);
            }
        }
    }

    // wait until sock is writeable
    // return ok(true) if writeable after timeout
    // return ok(false) if writeable after timeout
    // return err(false) 
    // if timeout is 0 wait forever
    // if you do not want to wait, please use writeable
    Result<bool> await_writeable(std::chrono::milliseconds const& timeout){
        if(timeout == std::chrono::milliseconds(0)){
            // wait forever
            try_writeable:{
                Result<bool> writeable_res = writeable();
                if(writeable_res.check()){
                    if(writeable_res.ignore()){
                        return Result<bool>::ok(true);
                    }else{
                        goto try_writeable;
                    }
                }else{
                    return Result<bool>::err(false,TMC_R_CALL_POS(exact_err().ignore()));
                }
            }
        }else{
            size_t count = timeout.count();
            timeval _tval;
            _tval.tv_sec = (int)count/1000;
            _tval.tv_usec = (int)count%1000;
            fd_set set;
            FD_ZERO(&set);
            FD_SET(h_sock_,&set);
            if(::select((int)h_sock_+1,0,&set,0,&_tval) == SOCKET_ERROR){
                return Result<bool>::err(false,TMC_R_CALL_POS(exact_err().ignore()));
            }
            if(FD_ISSET(h_sock_,&set)){// read
                return Result<bool>::ok(true);
            }else{
                return Result<bool>::ok(false);
            }
        }
    }

    // use select to check if has error
    // Result can be ignored, will return false on error
    Result<bool> has_error(){
        timeval _tval;
        _tval.tv_sec = 0;
        _tval.tv_usec = 0;
        fd_set set;
        FD_ZERO(&set);
        FD_SET(h_sock_,&set);
        if(::select((int)h_sock_+1,0,0,&set,&_tval) == SOCKET_ERROR){
           return Result<bool>::err(false,TMC_R_CALL_POS(exact_err().ignore()));
        }
        if(FD_ISSET(h_sock_,&set)){// error
            return Result<bool>::ok(true);
        }else{  // no error
            return Result<bool>::ok(false);
        }
    }
    
    // check if there is pending connections
    // Result can be ignored, will return false on error
    Result<bool> acceptable(){
        return readable();
    }

    // get set
    template<int _opt>
    Result<void> setopt(typename GetSockOptDetails<_opt>::type const& val){
        int ret = ::setsockopt(
            h_sock_,
            GetSockOptDetails<_opt>::level,
            _opt,
            (const char*)&val,
            sizeof(typename GetSockOptDetails<_opt>::type));
        return Result<void>(ret!=SOCKET_ERROR,TMC_R_CALL_POS(exact_err().ignore()));
    }
    
    template<int _opt>
    Result<typename GetSockOptDetails<_opt>::type> getopt(){
        Result<typename GetSockOptDetails<_opt>::type> res(true,TMC_R_CALL_POS(0));
#ifdef __linux__
        socklen_t len = sizeof(typename GetSockOptDetails<_opt>::type);
#elif defined(_WIN32)
        int len = sizeof(typename GetSockOptDetails<_opt>::type);
#endif
        int ret = ::getsockopt(
            h_sock_,
            GetSockOptDetails<_opt>::level,
            _opt,
            (char*)&res.ignore(),
            &len);
        res.change_res(ret!=SOCKET_ERROR);
        if(!res){
            res.set_ec(fast_err());
        }
        return res;
    }


    // timeout
    Result<void> set_write_timeout(std::chrono::milliseconds const& timeout){
        timeval val;
        size_t ms = timeout.count();
        val.tv_sec = (decltype(timeval::tv_sec))ms/1000;
        val.tv_usec = (decltype(timeval::tv_usec))ms%1000;
        return setopt<SO_SNDTIMEO>(val);
    }

    Result<std::chrono::milliseconds> get_write_timeout(){
        auto res = getopt<SO_SNDTIMEO>();
        auto& val = res.ignore();
        return Result<std::chrono::milliseconds>(res.check(),std::chrono::milliseconds((val.tv_sec*1000)+val.tv_usec));
    }

    Result<void> set_read_timeout(std::chrono::milliseconds const& timeout){
        timeval val;
        size_t ms = timeout.count();
        val.tv_sec = (decltype(timeval::tv_sec))ms/1000;
        val.tv_usec = (decltype(timeval::tv_usec))ms%1000;
        return setopt<SO_RCVTIMEO>(val);
    }
    
    Result<std::chrono::milliseconds> get_read_timeout(){
        auto res = getopt<SO_RCVTIMEO>();
        auto& val = res.ignore();
        return Result<std::chrono::milliseconds>(res.check(),std::chrono::milliseconds((val.tv_sec*1000)+val.tv_usec));
    }

    // buf size
    Result<void> set_write_bufsize(int bufsize){
        return setopt<SO_SNDBUF>(bufsize);
    }

    Result<int> get_write_bufsize(){
        return getopt<SO_SNDBUF>();
    }

    Result<void> set_read_bufsize(int bufsize){
        return setopt<SO_RCVBUF>(bufsize);
    }

    Result<int> get_read_bufsize(){
        return getopt<SO_RCVBUF>();
    }

    // sensitivity
    Result<void> set_write_sensitivity(int bytesize){
        return setopt<SO_SNDLOWAT>(bytesize);
    }

    Result<int> get_write_sensitivity(){
        return getopt<SO_SNDLOWAT>();
    }

    Result<void> set_read_sensitivity(int bytesize){
        return setopt<SO_RCVLOWAT>(bytesize);
    }

    Result<int> get_read_sensitivity(){
        return getopt<SO_RCVLOWAT>();
    }

    // set close type via SO_LINGER
    // param _wait
    // _wait == -1  wait forever
    // _wait >=0    wait for _wait second(s)
    Result<void> set_close_wait(int _wait){
        linger _l;
        if(_wait == -1){
            _l.l_onoff = 0;
        }else{
            _l.l_onoff = 1;
            _l.l_linger = _wait;
        }
        return setopt<SO_LINGER>(_l);
    }

    // get close type via SO_LINGER
    // return
    // == -1    wait forever
    // >=0      wait for _wait second(s)
    Result<int> get_close_wait(){
        auto res = getopt<SO_LINGER>();
        auto& _l = res.ignore();
        int _wait;
        if(_l.l_onoff){
            _wait = _l.l_linger;
        }else{
            _wait = -1;
        }
        return Result<int>(res.check(),std::move(_wait));
    }

    // reuseaddr
    Result<void> set_reuse_addr(bool _reuseable){
        return setopt<SO_REUSEADDR>((int)_reuseable);
    }
    
    Result<bool> get_reuse_addr(){
        auto res = getopt<SO_REUSEADDR>();
        return Result<bool>(res.check(),(bool)res.ignore());
    }

    // borad cast
    Result<void> enable_broadcast(){
        return setopt<SO_BROADCAST>(1);
    }
    Result<void> disable_broadcast(){
        return setopt<SO_BROADCAST>(0);
    }
    Result<bool> is_broadcast_enabled(){
        auto res = getopt<SO_BROADCAST>();
        return Result<bool>(res.check(),(bool)res.ignore());
    }

    // get the error number on this socket
    Result<int> exact_err(){
        return getopt<SO_ERROR>();
    }
    // rise an error that can be get by exact_err()
    Result<void> rise_err(int error_number){
        return setopt<SO_ERROR>(error_number);
    }

};


}

#undef ROUTE_SOCK_OPT

#endif