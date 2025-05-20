#include"HttpRequest.h"
const std::unordered_set<std::string> HttpRequest::DEFAULT_HTML_{
    "/index", "/welcome", "/video", "/picture"
};
bool HttpRequest::Parse_Request_Line_(const std::string& Line){
    /*定义一个正则表达式模式，用于匹配 HTTP 请求行
    ^: 匹配字符串的开始。
    ([^ ]*): 匹配不包含空格的任意字符序列，即方法。
    : 匹配一个空格。
    ([^ ]*): 匹配不包含空格的任意字符序列，即路径。
    HTTP/: 匹配字符串 " HTTP/"。
    ([^ ]*): 匹配不包含空格的任意字符序列，即版本。
    $: 匹配字符串的结束*/
    std::regex Patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    //存储正则表达式匹配的结果
    std::smatch SubMatch;
    if(regex_match(Line, SubMatch, Patten)){
        Method_=SubMatch[1];
        Path_=SubMatch[2];
        Version_=SubMatch[3];
        State_=HEADERS;
        return true;
    }
    return false;
}
void HttpRequest::Parse_Reauest_Header_(const std::string& Line){
    /*用于匹配 HTTP 请求头部中的键值对
    ^：行开始。
    ([^:]*)：匹配冒号之前的所有字符，即头部字段的键，但不包括冒号。* 表示匹配0或多个字符。
    :：匹配冒号。
    ?：匹配0或1个空格，允许冒号后有空格。
    (.*)：匹配冒号之后的所有字符，即头部字段的值，直到行尾。* 表示匹配0或多个字符。
    $：行结束*/
    std::regex Patten("^([^:]*): ?(.*)$");
    std::smatch SubMatch;
    if(regex_match(Line,SubMatch,Patten)){
        Header_[SubMatch[1]]=SubMatch[2];
    }else{
        State_=BODY;
    }
}
void HttpRequest::Parse_Data_Body_(const std::string& Line){
    //将输入的行设置为请求体内容
    Body_=Line;
    Parse_Post_();
    State_=FINISH;
}
void HttpRequest::Parse_Path_(){
    //检查请求路径是否为根路径(“/”)
    if(Path_=="/"){
        //如果是根路径，将 Path_ 修改为 /index.html
        Path_="/index.html";
    }else{
        //遍历DEFAULT_HTML_
        for(auto &item:DEFAULT_HTML_){
            if (item==Path_)
            {
                //成功匹配后请求路径后添加 .html 扩展名
                Path_=Path_+".html";
                break;
            }
        }
    }
}
void HttpRequest::Parse_Post_(){
    if(Method_=="POST"&&Header_["Connect-Type"]=="application/x-www-from-urlencoded"){
        // 检查请求方法是否为 POST 以及内容类型是否为 application/x-www-form-urlencoded
        if(Body_.size()==0){
            //如果请求体为空，则直接返回
            return; 
        }
        std::string key,value;
        int num=0;
        int n=Body_.size();
        int i=0,j=0;
        for(;i<n;i++){
            char ch=Body_[i];
            switch (ch){
            case '=':
                key=Body_.substr(j,i-j); //提取键
                j=i+1;
                break;
            case '+':
                Body_[i]=' '; //将 '+' 转换为空格
                break;
            case '%':
                //解码URL编码的字符
                num=Convert_Hex(Body_[i+1])*16+Convert_Hex(Body_[i+2]);
                Body_[i]=num;
                //跳过已解码的字符
                i=i+2; 
                break;
            case '&':
                //提取值
                value = Body_.substr(j,i-j); 
                j=i+1;
                //将键值对存储到Post_ map中
                Post_[key] = value; 
                break;
            default:
                break;
            }
        }
        assert(j<=i);
        if(Post_.count(key)==0&&j<i){
            //处理最后一个键值对（如果存在）
            value=Body_.substr(j,i-j);
            Post_[key]=value;
        }
    }
}
int HttpRequest::Convert_Hex(char ch){
    if(ch >= '0' && ch <= '9') return ch - '0';
    if(ch >= 'A' && ch <= 'F') return ch -'A' + 10;
    if(ch >= 'a' && ch <= 'f') return ch -'a' + 10;
    return ch;
}
void HttpRequest::Init() {
    Method_ = Path_ = Version_ = Body_ = "";
    //初始化解析到行状态
    State_=REQUEST_LINE;
    //清空容器
    Header_.clear();
    //清空容器
    Post_.clear();
}
bool HttpRequest::Parse(Buffer& Buff){
    //常量字符数组，用于表示 HTTP 中的回车换行符，这是 HTTP 协议中用于分隔请求行的标准分隔符
    const char CRLF[]="\r\n";
    //缓冲区无数据则直接返回false
    if(Buff.How_Many_Bytes_We_Need_Read()<=0){
        return false;
    }
    //缓冲区可读数据非０且状态不是结束则保持循环
    while(Buff.How_Many_Bytes_We_Need_Read()&&State_!=FINISH){
        //使用search查找缓冲区中下一个 CRLF（回车换行符）的位置
        const char* Line_End =std::search(Buff.Where_Did_We_Read(),Buff.Where_Did_We_Write_Const(),CRLF,CRLF+2);
        //从当前读取指针到行结束位置提取出一行数据，存储在 line 字符串中
        std::string Line(Buff.Where_Did_We_Read(),Line_End);
        //根据当前的解析状态State_，执行不同的解析函数
        switch(State_){
            case REQUEST_LINE:
                if(!Parse_Request_Line_(Line)){
                    return false;
                }Parse_Path_();
                break;
            case HEADERS:
                Parse_Reauest_Header_(Line);
                if(Buff.How_Many_Bytes_We_Need_Read()<=2){
                    State_=FINISH;
                }break;
            case BODY:
                Parse_Data_Body_(Line);
                break;
            default:
                break;
        }//如果行结束指针等于缓冲区的写入指针，说明已经到达缓冲区末尾，跳出循环
        if(Line_End==Buff.Where_Did_We_Write()){
            break;
        }
        //更新缓冲区的读取指针，跳过当前行和 CRLF
        Buff.Update_ReadPos(Line_End+2);
    }return true;
}
std::string HttpRequest::Path() const {
    return Path_;
}
std::string& HttpRequest::Path(){
    return Path_;
}
std::string HttpRequest::Method() const {
    return Method_;
}
std::string HttpRequest::Version() const {
    return Version_;
}
std::string HttpRequest::Get_Post(const std::string& key) const {
    assert(key!="");
    if(Post_.count(key)==1){
        return Post_.find(key)->second;
    }
    return "";
}
std::string HttpRequest::Get_Post(const char* key) const {
    assert(key!=nullptr);
    if(Post_.count(key)==1){
        return Post_.find(key)->second;
    }
    return "";
}
bool HttpRequest::Are_You_Keep_Alive() const {
    //返回具有指定键的元素数量
    if(Header_.count("Connection")==1){
        return Header_.find("Connection")->second =="keep-aliv"&& Version_=="1.1";
    }else{
        return false;
    }
}