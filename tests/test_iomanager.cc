#include "vity/vity.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <sys/epoll.h>
#include <stdlib.h>

vity::Logger::ptr g_logger = VITY_LOG_ROOT();

int sock = 0;
void test_fiber()
{
    VITY_LOG_INFO(g_logger) << "test_fiber sock="<<sock;
    sock = socket(AF_INET,SOCK_STREAM,0);
    fcntl(sock,F_SETFL,O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET,"163.177.151.110",&addr.sin_addr);
    // 连接没有出错
    if(!connect(sock,(const sockaddr*)&addr,sizeof(addr)))
    {  
    }else if(errno == EINPROGRESS){
        VITY_LOG_ERROR(g_logger) << "add event errno=" << errno << " " << strerror(errno);
        vity::IOManager::getThis()->addEvent(sock, vity::IOManager::READ, [](){
            VITY_LOG_INFO(g_logger) << "read callback";
        });
        vity::IOManager::getThis()->addEvent(sock, vity::IOManager::WRITE, [](){
            VITY_LOG_INFO(g_logger) << "write callback";
            //close(sock);
            vity::IOManager::getThis()->cancelEvent(sock, vity::IOManager::READ);
            close(sock);
        });
    } else {
        VITY_LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
    }
}

void test()
{
    std::cout<< "EPOLLIN="<<EPOLLIN
        << "EPOLLOUT="<<EPOLLOUT << std::endl;

    vity::IOManager iom(2,false); // 在该对象结束的时候 调用stop
    iom.schedule(&test_fiber);
    return ;
}

int main(int argc,char** argv)
{
    test();
    return 0;
}
