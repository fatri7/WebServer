#include"epoll.h"
//使用epoll_create创建一个epoll 实例，并存储返回的文件描述符在 EpollerFd_中
Epoller::Epoller(int MaxEvents):EpollerFd_(epoll_create(512)), Events_(MaxEvents){
    assert(EpollerFd_>=0&&Events_.size()>0);
}
//使用close来关闭epoll描述符
Epoller::~Epoller() {
    close(EpollerFd_);
}
bool Epoller::AddFd(int FileD,uint32_t Events){
    //判断Filed是否是一个有效的文件标识符
    if(FileD<0) return false;
    //初始化一个事件，它的值全赋0，确保已知可控
    epoll_event ev={0};
    //会发生动作的文件的描述符
    ev.data.fd=FileD;
    //想要监听的动作
    ev.events=Events;
    return 0==epoll_ctl(EpollerFd_,EPOLL_CTL_ADD,FileD,&ev);
}
bool Epoller::ModFd(int FileD,uint32_t Events){
    //判断Filed是否是一个有效的文件标识符
    if(FileD<0) return false;
    //初始化一个事件，它的值全赋0，确保已知可控
    epoll_event ev={0};
    //会发生动作的文件的描述符
    ev.data.fd=FileD;
    //想要监听的动作
    ev.events=Events;
    return 0==epoll_ctl(EpollerFd_,EPOLL_CTL_MOD,FileD,&ev);
}
bool Epoller::DelFd(int FileD){
    //判断Filed是否是一个有效的文件标识符
    if(FileD<0) return false;
    //初始化一个事件，它的值全赋0，确保已知可控
    epoll_event ev={0};
    //EPOLL_CTL_DEL操作不需要ev动作的信息，只需要有效的FileD
    return 0==epoll_ctl(EpollerFd_,EPOLL_CTL_DEL,FileD,&ev);
}
int Epoller::Wait(int TimeWait){
    return epoll_wait(EpollerFd_,&*Events_.begin(),Events_.size(),TimeWait);
}
int Epoller::Get_Event_FileD(size_t Index) const {
    assert(Index<Events_.size()&&Index>=0);
    return Events_[Index].data.fd;
}
uint32_t Epoller::Get_Event_events(size_t Index) const {
    assert(Index<Events_.size()&&Index>=0);
    return Events_[Index].events;
}
