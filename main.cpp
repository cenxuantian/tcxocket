#include <iostream>
#include <tmc>
#include <thread>
#include <chrono>
#include <vector>
#include <fstream>
#include <mutex>
#include <string>
#include <cstring>
#include <assert.h>



using namespace TMC;

static bool need_stop = false;

void tcp_server(){
    
    // Socket::env_start().except("cannot start environmenet");

    Socket sock = Socket::create(Socket::Protocol::TCP).except("cannot create socket");

    sock.set_read_bufsize(10).no_except("set read buff error");
    sock.set_write_bufsize(10).no_except("set write buff error");

    std::cout << "opt SO_RCVBUF: " 
        << sock.get_read_bufsize().no_except() << '\n';
    std::cout << "opt SO_SNDBUF: " 
        << sock.get_write_bufsize().no_except() << '\n';

    if(!sock.bind(IPAddr::v4("127.0.0.1",8888)).no_except()){
        goto close_sock;
    }
    if(!sock.listen().no_except()){
        goto close_sock;
    }

    {
        auto accept_res = sock.accept();
        
        if(!accept_res.check()){
            std::cout << "accept error"<<Socket::fast_err()<<'\n';
            goto close_sock;
        }
        Socket remote = accept_res.ignore();

        std::cout << remote.address().host()<<':'<<remote.address().port()<<" connected\n";

        while (!need_stop)
        {
            auto read_all_res = remote.readall(10,std::chrono::milliseconds(5000));
            if(!read_all_res.check()){
                std::cout << "readall error"<<Socket::fast_err()<<'\n';
                remote.shutdown();
                remote.close();
                goto close_sock;
            }else{
                auto& msg = read_all_res.ignore();
                if(msg.size()){
                    std::cout   << "recv from " 
                                << remote.address().host()<<':'<<remote.address().port()<<' '
                                << '['<<msg.size() << "] "
                                << msg.c_str()<<'\n';
                }
            }

            // auto can_read = remote.readable();
            // if(!can_read.check()){
            //     std::cout << "readable error"<<Socket::fast_err()<<'\n';
            //     remote.shutdown();
            //     remote.close();
            //     goto close_sock;
                
            // }
            // if(can_read.ignore()){
            //     auto res = remote.readsome(1024);
            //     if(!res.check()){
            //         std::cout << "accept error"<<Socket::fast_err()<<'\n';
            //         remote.shutdown();
            //         remote.close();
            //         goto close_sock;
            //     }
            //     auto& msg = res.ignore();
            //     if(msg.size()){
            //         std::cout   << "recv from " 
            //                     << remote.address().host()<<':'<<remote.address().port()<<' '
            //                     << '['<<msg.size() << "] "
            //                     << msg.c_str()<<'\n';
            //     }
            // }



        }

        remote.shutdown().except("remote shutdown error: "+std::to_string(Socket::fast_err()));
        remote.close().except("remote close error: "+std::to_string(Socket::fast_err()));
    }

    close_sock:
    sock.close().no_except();
    Socket::env_stop().except();
    return;
}

void tcp_client(){
    Socket::env_start().except(Socket::fast_err());

    Socket sock = Socket::create(Socket::Protocol::TCP).except();

    if(!sock.connect(IPAddr::v4("127.0.0.1",8888)).no_except()){
        goto close_sock;    // connect not successful
    }
    std::cout << "client: "<< sock.address().host() <<':'<< sock.address().port()<<'\n';

    while (1)
    {
        std::string s;
        std::cin >> s;
        if(s=="stop"){
            need_stop = true;
            break;
        }else{
            sock.write_all(s).no_except();
        }
    }
    

    close_sock:
    sock.shutdown().except();
    sock.close().except();
    Socket::env_stop().except();
    return;
}

void udp_recv(){
    Socket::env_start().except("start env error: "+std::to_string(Socket::fast_err()));

    IPAddr remote_addr = IPAddr::v4("127.0.0.1",8802);

    Socket sock = Socket::create(Socket::Protocol::UDP).except("socket create error: "+std::to_string(Socket::fast_err()));


    if(!sock.bind(IPAddr::v4("127.0.0.1",8801)).check()){
        std::cout << "bind error: "<<Socket::fast_err()<<'\n';
        goto close_sock;
    }

    while (!need_stop)
    {   
        auto ret = sock.readable();
        if(!ret.check()){
            goto close_sock;
        }else{
            if(ret.ignore()){
                // can be read
                // std::cout << "can read\n";
                auto res = sock.readsome_from(1024,remote_addr);
                if(!res.check()){
                    std::cout << "readsome error"<<Socket::fast_err()<<'\n';
                    goto close_sock;
                }
                auto& msg = res.ignore();
                if(msg.size()){
                    std::cout   << "recv from " 
                                << remote_addr.host()<<':'<<remote_addr.port()<<' '
                                << '['<<msg.size() << "] "
                                << msg.c_str()<<'\n';
                }
            }
        }
    }

    

    close_sock:
    sock.shutdown().except("shutdown error: "+std::to_string(Socket::fast_err()));
    sock.close().except("close error: "+std::to_string(Socket::fast_err()));
    Socket::env_stop().except("stop env error: "+std::to_string(Socket::fast_err()));
    return;
}

void udp_send(){
    Socket::env_start().except("start env error: "+std::to_string(Socket::fast_err()));

    IPAddr remote_addr = IPAddr::v4("127.0.0.1",8801);

    Socket sock = Socket::create(Socket::Protocol::UDP).except("socket create error: "+std::to_string(Socket::fast_err()));

    if(!sock.bind(IPAddr::v4("127.0.0.1",8802)).no_except("bind error: " + std::to_string(Socket::fast_err()))){
        goto close_sock;
    }

    while (1)
    {
        std::string s;
        std::cin >> s;
        if(s=="stop"){
            need_stop = true;
            break;
        }else{
            sock.write_all_to(s,remote_addr).no_except("write error: "+std::to_string(Socket::fast_err()));
        }
    }

    close_sock:
    sock.shutdown().except("shutdown error: "+std::to_string(Socket::fast_err()));
    sock.close().except("close error: "+std::to_string(Socket::fast_err()));
    Socket::env_stop().except("stop env error: "+std::to_string(Socket::fast_err()));
    return;
}



int main(){
    // Logger os(std::cout);

    

    // ThreadPool<2,2> pool;
    // std::this_thread::sleep_for(std::chrono::milliseconds(200));
    // pool.submit(tcp_server);
    // pool.submit(tcp_client);
    // // pool.submit(udp_send);
    // // pool.submit(udp_recv);

    // pool.stop();
    return 0;
}