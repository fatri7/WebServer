#include"HttpResponse.h"
const std::unordered_map<std::string, std::string> HttpResponse::SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/msword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css"}, 
    { ".js",    "text/javascript"},
};
const std::unordered_map<int, std::string> HttpResponse::CODE_STATUS = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
};
const std::unordered_map<int, std::string> HttpResponse::CODE_PATH = {
    { 400, "/400.html" },
    { 403, "/403.html" },
    { 404, "/404.html" },
};
HttpResponse::HttpResponse(){
    code_=-1;
    path_=srcDir_="";
    Are_You_Keep_Alive_=false;
    mmFile_=nullptr;
    mmFileStat_={0};
};
HttpResponse::~HttpResponse(){
    unmap_File();
}

void HttpResponse::Init(const std::string& srcDir,std::string& path,bool Are_You_Keep_Alive,int code){
    assert(srcDir!="");
    if(mmFile_){
        //如果mmFile_已经被映射，调用unmap_File来释放资源
        unmap_File();
    }code_=code;
    Are_You_Keep_Alive_=Are_You_Keep_Alive;
    path_=path;
    srcDir_=srcDir;
    //将mmFile_设置为nullptr，表示当前没有文件被映射
    mmFile_=nullptr;
    mmFileStat_={0};
}
void HttpResponse::make_Response(Buffer& buffer){
    if(stat((srcDir_+path_).data(),&mmFileStat_)<0||S_ISDIR(mmFileStat_.st_mode)){
        //检查文件状态，如果文件不存在或是一个目录，则设置状态码为404;S_ISDIR(mmFileStat_.st_mode)宏，用于检查文件是否是一个目录
        code_=404;
    }else if(!(mmFileStat_.st_mode & S_IROTH)){
        //如果文件存在但不可读，则设置状态码为403,mmFileStat_.st_mode & S_IROTH检查文件是否对其他用户可读
        code_=403;
    }else if(code_==-1){
        //如果代码中设置了特定的条件（code_==-1），则设置状态码为200
        code_=200;
    }
    errorHTML_();
    add_State_Line_(buffer);
    add_Response_Header_(buffer);
    add_Response_Content_(buffer);
}
char* HttpResponse::file(){
    return mmFile_;
}
size_t HttpResponse::file_Length() const {
    return mmFileStat_.st_size;
}
void HttpResponse::errorHTML_(){
    if(CODE_PATH.count(code_)==1){
        //检查CODE_PATH映射中是否包含当前的状态码code_ 设置path_为对应的状态码的错误HTML文件路径
        path_=CODE_PATH.find(code_)->second;
        //调用stat系统函数来获取错误HTML文件的状态信息
        stat((srcDir_ + path_).data(), &mmFileStat_);
    }
}
void HttpResponse::add_State_Line_(Buffer& buffer){
    std::string status;
    if(CODE_STATUS.count(code_)==1){
        //检查CODE_STATUS映射中是否包含code_键,如果存在，将对应的状态描述赋值给status
        status=CODE_STATUS.find(code_)->second;
    }else{
        code_=400;
        //获取状态码400对应的状态描述
        status=CODE_STATUS.find(400)->second;
    }
    buffer.Write_to_Buffer("HTTP/1.1"+std::to_string(code_)+" "+status+"\r\n");
}
void HttpResponse::add_Response_Header_(Buffer& buffer){
    buffer.Write_to_Buffer("Connection:");
    if(Are_You_Keep_Alive_){
        buffer.Write_to_Buffer("keep-alive\r\n");
        buffer.Write_to_Buffer("keep-alive: max=6,timeout=120\r\n");
    }else{
        buffer.Write_to_Buffer("close\r\n");
    }
    buffer.Write_to_Buffer("Content-type:"+get_File_Type()+"\r\n");
}
void HttpResponse::add_Response_Content_(Buffer& buffer){
    int srcFD=open((srcDir_+path_).data(),O_RDONLY);
    if(srcFD<0){
        error_Content(buffer,"File NotFound!");
        return;
    }
    // 将文件映射到内存提高文件的访问速度 
    // MAP_PRIVATE 建立一个写入时拷贝的私有映射
    int* mmRet=(int*)mmap(0,mmFileStat_.st_size,PROT_READ,MAP_PRIVATE,srcFD,0);
    if(*mmRet==-1){
        error_Content(buffer,"File NotFound");
        return;
    }
    mmFile_=(char*)mmRet;
    close(srcFD);
    buffer.Write_to_Buffer("Content-Length:"+std::to_string(mmFileStat_.st_size)+"\r\n\r\n"); 
}
void HttpResponse::unmap_File(){
    if(mmFile_){
        munmap(mmFile_,mmFileStat_.st_size);
        mmFile_=nullptr;
    }
}
std::string HttpResponse::get_File_Type(){
    std::string::size_type idx=path_.find_last_of('.');
    if(idx==std::string::npos){
        //返回默认的MIME类型"text/plain"
        return "text/plain";
    }
    //从’.'的位置开始提取子字符串作为文件扩展名
    std::string suffix=path_.substr(idx);
    if(SUFFIX_TYPE.count(suffix)==1){
        //检查SUFFIX_TYPE映射（可能是一个std::map）中是否包含该扩展名。如果包含，则返回对应的MIME类型
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}
void HttpResponse::error_Content(Buffer& buffer, std::string message) {
    std::string body;
    std::string status;
    body+="<html><title>Error</title>";
    body+="<body bgcolor=\"ffffff\">";
    if(CODE_STATUS.count(code_)==1){
        //检查CODE_STATUS映射中是否包含当前的状态码code_，如果包含，则获取对应的状态消息；如果不包含，则默认为"Bad Request"
        status= CODE_STATUS.find(code_)->second;
    }else{
        status="Bad Request";
    }
    body+=std::to_string(code_)+":"+status+"\n";
    body+="<p>"+message+"</p>";
    body+="<hr><em>TinyWebServer</em></body></html>";
    buffer.Write_to_Buffer("Content-Length:"+ std::to_string(body.size()) + "\r\n\r\n");
    buffer.Write_to_Buffer(body);
}