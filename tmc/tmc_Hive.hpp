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

#ifndef __TMC_HIVE_HPP__
#define __TMC_HIVE_HPP__

#include "tmc_Socket.hpp"
#include "tmc_ThreadPool.hpp"

#include <unordered_set>
#include <unordered_map>

namespace TMC{

struct HiveSockets{
    Socket::Protocol protocol;
    u_short port;
};

struct HiveDesc{
    std::unordered_set<HiveSockets> sockets;

};

template<size_t _BeesCount, size_t _TaskMax>
class Hive{
private:
    bool valid = false;

    HiveDesc desc_;

    ThreadPool<_BeesCount,_TaskMax> pool_;

    

    Hive(HiveDesc _desc):desc_(_desc){
        if(!_desc.sockets.size()){
            valid = false;
            return;
        }


        valid = true;
    }

public:
    static Result<Hive> create(HiveDesc _desc){
        Result<Hive> _r(true,_desc);
        _r.change_res(_r.ignore().valid_);
        return _r;
    }

};

}


#endif