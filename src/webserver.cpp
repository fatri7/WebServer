
#include"webserver.h"
WebServe::WebServe(int port,int trig_mode,int timeout_ms,bool opt_linger,int thread_number):
port_(port),open_linger_(opt_linger),time_out_ms_(timeout_ms),close_or_not_(false),timer_(new TimerManager()),
m_threadpool_(std::make_unique<CoroutineThreadPool>(thread_number, 500)),epoller_(new Epoller()){
    //获取当前工作目录
    srcDir_ = getcwd(nullptr, 256);  // 动态分配内存
assert(srcDir_); 
    size_t len = strlen(srcDir_);
if (len > 0 && srcDir_[len - 1] != '/') {
    strncat(srcDir_, "/", 2);  // 追加分隔符
}
strncat(srcDir_, "resources/", 11);  // 追加目录
    HttpConnection::user_count=0;
    HttpConnection::srcDir=srcDir_;
    init_event_mode_(trig_mode);
    if(!init_socket_()){
        close_or_not_=true;
    }
}
WebServe::~WebServe(){
    close(listen_fd_);
    close_or_not_=true;
    free(srcDir_);
}
void WebServe::init_event_mode_(int trig_mode){
    //监听套接字的事件类型:对端关闭连接或者关闭写操作时触发
    listen_event_=EPOLLRDHUP;
    //连接套接字的事件类型:对端关闭连接或者关闭写操作时触发,事件只会被触发一次，触发后需要重新注册事件
    connection_event_=EPOLLONESHOT|EPOLLRDHUP;
    switch(trig_mode){
        case 0:
        //默认模式，不修改 listen_event_ 和 connection_event_
            break;
        case 1:
        //将连接套接字设置为边缘触发模式
            connection_event_|=EPOLLET;
            break;
        case 2:
        //将监听套接字设置为边缘触发模式
            listen_event_|=EPOLLET;
            break;
        case 3:
        //将监听套接字和连接套接字都设置为边缘触发模式
            listen_event_|=EPOLLET;
            connection_event_|=EPOLLET;
            break;
        default:
        //默认情况下，将监听套接字和连接套接字都设置为边缘触发模式
            listen_event_|=EPOLLET;
            connection_event_|=EPOLLET;
            break;
    }
    //根据连接套接字的事件类型设置 HttpConnection 类的静态成员 isEt，用于标识是否使用边缘触发模式
    //检查是否设置了边缘触发模式，按位与
    HttpConnection::isEt=(connection_event_& EPOLLET);
}
void WebServe::send_error_(int fd,const char* information){
    assert(fd>0);
    close(fd);
}
void WebServe::close_connection_(HttpConnection* client){
    assert(client);
    //epoll不会再监听这个文件描述符的事件
    epoller_->DelFd(client->get_Fd());
    //关闭连接并释放相关资源
    client->close_httpconnection();
}
void WebServe::add_client_connection_(int fd,sockaddr_in addr){
    assert(fd>0);
    //调用 HttpConnection 对象的 init_httpconnection 方法，初始化与客户端连接相关的信息
    users_[fd].init_httpconnection(fd,addr);
    //检查是否设置了超时时间
    if(time_out_ms_>0){
        //如果设置了超时时间，就添加一个定时器，当超时发生时，调用 WebServe::close_connection_ 方法关闭连接
        timer_->add_timer(fd,time_out_ms_,std::bind(&WebServe::close_connection_,this,&users_[fd]));
    }
    //将文件描述符添加到 epoll 的监听列表中，监听可读事件和连接事件（可能是边缘触发或水平触发，取决于 connection_event_ 的值）
    epoller_->AddFd(fd,EPOLLIN|connection_event_);
    //将文件描述符设置为非阻塞模式 I/O 操作不会阻塞进程
    set_fd_nonblock_(fd);
}
void WebServe::handle_listen_(){
    struct sockaddr_in addr;
    socklen_t length=sizeof(addr);
    //如果监听套接字设置为边缘触发模式（EPOLLET），则循环将继续接受连接，直到没有更多连接可接受
    do{
        //调用 accept 函数接受新的连接。如果成功，返回一个新的文件描述符用于与客户端通信
        int fd=accept(listen_fd_,(sockaddr*)&addr,&length);
        if(fd<=0){
            //检查 accept 是否成功
            return;
        }else if(HttpConnection::user_count>=max_fd_){
            //检查当前用户数是否达到服务器允许的最大文件描述符数。如果是，则发送错误消息给客户端并关闭连接。
            send_error_(fd,"erver busy!");
            return;
        }else{
            //调用 add_client_connection_ 函数来添加新的客户端连接
            add_client_connection_(fd,addr);
        }
    }while(listen_event_& EPOLLET);
}
void WebServe::handle_write_(HttpConnection* client) {
    extent_time_(client);
    m_threadpool_->submit([this, client] {
        on_write_(client); // 协程中执行
    });
}
// 处理写事件
// 处理读事件（自动协程化）
void WebServe::handle_read_(HttpConnection* client) {
    assert(client);
    extent_time_(client); // 更新连接活跃时间
    
    // 提交到协程线程池
    m_threadpool_->submit([this, client] {
        on_read_(client); // 在协程中执行
    });
}
void WebServe::extent_time_(HttpConnection* client){
    assert(client);
    if(time_out_ms_>0){
        timer_->update(client->get_Fd(),time_out_ms_);
    }
}
void WebServe::on_read_(HttpConnection* client){
    assert(client);
    int ret=-1;
    int read_error=0;
    ret=client->read_buffer(&read_error);
    if(ret<=0&&read_error!=EAGAIN){
        close_connection_(client);
        return;
    }
    on_process_(client);
}
void WebServe::on_process_(HttpConnection* client){
    if(client->handle_httpconnection()){
        epoller_->ModFd(client->get_Fd(),connection_event_|EPOLLOUT);
    }else{
        epoller_->ModFd(client->get_Fd(),connection_event_|EPOLLIN);
    }
}
void WebServe::on_write_(HttpConnection* client){
    assert(client);
    int ret=-1;
    int write_error=0;
    ret=client->write_buffer(&write_error);
    if(client->get_write_length()==0){
        if(client->get_alive_status()){
            on_process_(client);
            return;
        }
    }else if(ret<0){
        if(write_error==EAGAIN){
            epoller_->ModFd(client->get_Fd(),connection_event_|EPOLLOUT);
            return;
        }
    }
    close_connection_(client);
}
bool WebServe::init_socket_(){
    int ret;
    sockaddr_in addr;
    if(port_>65535||port_<1024){
        return false;
    }
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=htonl(INADDR_ANY);
    addr.sin_port=htons(port_);
    linger opt_linger={0};
    if(open_linger_){
        opt_linger.l_linger=1;
        opt_linger.l_onoff=1;
    }
    listen_fd_=socket(AF_INET,SOCK_STREAM,0);
    if(listen_fd_<0){
        return false;
    }
    ret=setsockopt(listen_fd_,SOL_SOCKET,SO_LINGER,&opt_linger,sizeof(opt_linger));
    if(ret<0){
        close(listen_fd_);
        return false;
    }
    int optval=1;
    ret=setsockopt(listen_fd_,SOL_SOCKET,SO_REUSEADDR,(const void*)&optval,sizeof(int));
    if(ret==-1){
        close(listen_fd_);
        return false;
    }
    ret=bind(listen_fd_,(sockaddr*)&addr,sizeof(addr));
    if(ret<0){
        close(listen_fd_);
        return false;
    }
    ret=listen(listen_fd_,6);
    if(ret<0){
        close(listen_fd_);
        return false;
    }
    ret=epoller_->AddFd(listen_fd_,listen_event_|EPOLLIN);
    if(ret==0){
        close(listen_fd_);
        return false;
    }
    set_fd_nonblock_(listen_fd_);
    return true;
}
int WebServe::set_fd_nonblock_(int fd){
    assert(fd>0);
    return fcntl(fd,F_SETFL,fcntl(fd,F_GETFD,0)|O_NONBLOCK);
}
void WebServe::start(){
    int time_ms=-1;
    if(!close_or_not_){
        std::cout<<"============================";
        std::cout<<"Server Start!";
        std::cout<<"============================";
        std::cout<<std::endl;
    }
    while(!close_or_not_){
        if(time_out_ms_>0){
            time_ms=timer_->get_next_timer_handle();
        }
        int event_cnt=epoller_->Wait(time_ms);
        for(int i=0;i<event_cnt;++i){
            int fd=epoller_->Get_Event_FileD(i);
            uint32_t events=epoller_->Get_Event_events(i);
            if(fd==listen_fd_){
                handle_listen_();
            }else if(events&(EPOLLRDHUP|EPOLLHUP|EPOLLERR)){
                assert(users_.count(fd)>0);
                close_connection_(&users_[fd]);
            }else if(events&EPOLLIN){
                assert(users_.count(fd)>0);
                handle_read_(&users_[fd]);
            }else if(events&EPOLLOUT){
                assert(users_.count(fd) > 0);
                handle_write_(&users_[fd]);
            }else{
                std::cout<<"Unexpected event"<<std::endl;
            }
        }
    }
}