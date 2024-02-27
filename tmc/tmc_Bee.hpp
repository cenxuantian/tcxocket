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

#ifndef __TMC_BEE_HPP__
#define __TMC_BEE_HPP__

#include "tmc_Socket.hpp"
#include <vector>
#include <functional>
#include <thread>

#define ROUTE_BEE_DESC(_BeeType_enum,_Type)\
template<> struct BeeDesc<_BeeType_enum>{typedef _Type type;}
#define ROUTE_BEE_IMPL(_BeeType_enum,_Type)\
template<> struct BeeImpl<_BeeType_enum>{typedef _Type type;}


namespace TMC{
enum class BeeType:int;
enum class BeePublicity:int;
template<BeeType _bee_type> class Bee;
template<BeeType _bee_type> class BeeDesc;


struct RemoteBee{
    std::string name;
    std::vector<std::string> groups_in_common;
};

struct BeeMessageDesc{
    std::string sender;
    bool is_from_group;
    std::string group;
};

// interface of bee
class BeeItf{
public:
    virtual ~BeeItf() = 0;
    // connect to the hive
    virtual Result<void> login() = 0;
    // disconnect to the hive
    virtual Result<void> logout() = 0;
    // send message to a single bee
    virtual Result<void> send2bee(std::string const& beename, ByteBuf const& message) = 0;
    // send message in a group
    virtual Result<void> send2group(std::string const& groupname,ByteBuf const& message) = 0;
    // join a new group
    virtual Result<void> join(std::string const& groupname, std::string const& passwd) = 0;
    // get all bees in joined groups
    virtual Result<std::vector<RemoteBee>> friends() = 0;
    // when recv message, the cb will be called
    virtual Result<void> on_message(std::function<void(BeeMessageDesc const& fromdesc, ByteBuf const& message)> const& cb) = 0;
};

enum class BeeType:int{
    SOCKET_TCP = 0,
    SOCKET_UDP = 2,
    NAMED_PIPE = 3,
    SHARED_MEMORY = 4,
};

enum class BeePublicity:int{
    PRIVATE = 0,
    PUBLIC = 1,
};

struct BeeDescCommon{
    BeePublicity publicity;
};

template<>
struct BeeDesc<BeeType::SOCKET_TCP>:public BeeDescCommon{
    IPAddr host;
};
template<>
class Bee<BeeType::SOCKET_TCP>:public BeeItf{
private:
    bool valid_ = false;

    BeeDesc<BeeType::SOCKET_TCP> desc_;

    std::function<void(BeeMessageDesc const&, ByteBuf const&)> on_message_cb = [](BeeMessageDesc const&, ByteBuf const&){};
    
    std::thread* recv_thread = nullptr;
    Socket sock;

    Bee() = delete;
    Bee(BeeDesc<BeeType::SOCKET_TCP> const& desc)
        :desc_(desc)
        ,sock(Socket::create(Socket::Protocol::TCP).or_else([]()->Result<TMC::Socket>{
            if(Socket::fast_err() == 10093){//env not start
                Socket::env_start().except("socket create env error "+Socket::fast_err());
            }
            return Socket::create(Socket::Protocol::TCP);
        }).except("cannot create socket "+Socket::fast_err()))
    {
        valid_ = true;
    }
public:
    

    static Result<Bee> create(BeeDesc<BeeType::SOCKET_TCP> const& desc) {
        Bee _b{desc};
        return Result<Bee>{_b.valid_,std::move(_b)};
    }

    Result<void> login()override{

        return true;
    }
    // disconnect to the hive
    Result<void> logout()override {
        return true;
    }
    // send message to a single bee
    Result<void> send2bee(std::string const& beename, ByteBuf const& message)override {
        return true;
    }
    // send message in a group
    Result<void> send2group(std::string const& groupname,ByteBuf const& message) override{
        return true;
    }
    // join a new group
    Result<void> join(std::string const& groupname, std::string const& passwd) override{
        return true;
    }
    // get all bees in joined groups
    Result<std::vector<RemoteBee>> friends() {
        return Result<std::vector<RemoteBee>>::ok();
    }
    // when recv message, the cb will be called
    Result<void> on_message(std::function<void(BeeMessageDesc const&, ByteBuf const&)> const& cb)override {
        return true;
    }
};


}


#endif