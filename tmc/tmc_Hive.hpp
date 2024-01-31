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