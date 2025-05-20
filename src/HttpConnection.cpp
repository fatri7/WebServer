#include"HttpConnection.h"

const char* HttpConnection::srcDir;
std::atomic<size_t>HttpConnection::user_count;
bool HttpConnection::isEt;
HttpConnection::HttpConnection() { 
    fd_=-1;
    addr_={0};
    close_or_not=true;
};
HttpConnection::~HttpConnection() { 
    close_httpconnection(); 
};
void HttpConnection::init_httpconnection(int fd,const sockaddr_in& addr){
    assert(fd>0);
    user_count++;
    addr_=addr;
    fd_=fd;
    write_buffer_.Init_Buffer();
    read_buffer_.Init_Buffer();
    close_or_not=false;
}
void HttpConnection::close_httpconnection(){
    response_.unmap_File();
    if(close_or_not==false){
        close_or_not=true;
        user_count--;
        close(fd_);
    }
}
int HttpConnection::get_Fd() const {
    return fd_;
}
struct sockaddr_in HttpConnection::get_addr() const {
    return addr_;
}
const char* HttpConnection::get_ip() const {
    //INET_ADDRSTRLEN是适用于IPv4地址的常量
    static char ip_str[INET_ADDRSTRLEN]; 
    if (inet_ntop(AF_INET, &addr_.sin_addr, ip_str, sizeof(ip_str)) == nullptr) {
        //inet_ntop是一个用于将网络地址结构转换为字符串表示的函数
        return nullptr;
    }
    return ip_str;
}

int HttpConnection::get_port() const {
    return addr_.sin_port;
}
ssize_t HttpConnection::read_buffer(int* save_erron){
    ssize_t length=-1;
    do{
        length=read_buffer_.Get_Data(fd_,save_erron);
        if(length<=0){
            break;
        }
    }while(isEt);
    return length;
}
ssize_t HttpConnection::write_buffer(int* save_erron){
    ssize_t length=-1;
    do{
        length=writev(fd_,iov_,iov_count_);
        if(length<0){
            //写入长度小于0，表示出现错误
            *save_erron=errno;
            break;
        }else if(iov_[0].iov_len+iov_[1].iov_len==0){
            //所有数据都已写入
            break;
        }else if (static_cast<size_t>(length)>iov_[0].iov_len){
            //写入的数据超过了第一个缓冲区的长度，调整第二个缓冲区的指针和长度
            iov_[1].iov_base=(uint8_t*)iov_[1].iov_base+(length-iov_[0].iov_len);
            iov_[1].iov_len-=(length-iov_[0].iov_len);
            if(iov_[0].iov_len){
                //第一个缓冲区还有数据未写入
                write_buffer_.Init_Buffer();
                iov_[0].iov_len=0;
            }
        }else{
            //如果写入的数据没有超过第一个缓冲区的长度，调整第一个缓冲区的指针和长度
            iov_[0].iov_base=(uint8_t*)iov_[0].iov_base+length;
            iov_[0].iov_len-=length;
            write_buffer_.Update_ReadPos(length);
        }
    }while(isEt||get_write_length()>10240);
    return length;
}
int HttpConnection::get_write_length(){
    return iov_[1].iov_len+iov_[0].iov_len;
}
bool HttpConnection::get_alive_status() const{
    return request_.Are_You_Keep_Alive();
}
bool HttpConnection::handle_httpconnection(){
    request_.Init();
    if(read_buffer_.How_Many_Bytes_We_Need_Read()<=0){
        //没有需要读取的字节，返回false
        return false; 
    }else if(request_.Parse(read_buffer_)){ 
        //解析成功，初始化响应对象为200 OK
        response_.Init(srcDir,request_.Path(),request_.Are_You_Keep_Alive(),200);
    }else{
        //解析失败，初始化响应对象为400 Bad Request
        response_.Init(srcDir,request_.Path(),false,400);
    }
    //生成响应并将其写入write_buffer_
    response_.make_Response(write_buffer_);

    //设置第一个iovec的基地址为write_buffer_的读取位置
    iov_[0].iov_base=const_cast<char*>(write_buffer_.Where_Did_We_Read());
    //设置第一个iovec的长度为write_buffer_中需要读取的字节数
    iov_[0].iov_len=write_buffer_.How_Many_Bytes_We_Need_Read();
    //设置iovec计数为1
    iov_count_=1; 

    if(response_.file_Length()>0&&response_.file()){ 
        //响应中有文件内容
        //设置第二个iovec的基地址为响应文件的地址
        iov_[1].iov_base=response_.file();
        //设置第二个iovec的长度为文件长度
        iov_[1].iov_len=response_.file_Length();
        //设置iovec计数为2
        iov_count_=2;
    }
    return true;
};
