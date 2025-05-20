#include "buffer.h"

//初始化Buffer，initBuffersize是初始大小，给读、写索引赋初值为０
Buffer::Buffer(int initBuffersize):buffer_(initBuffersize),ReadPos_(0),WritePos_(0){};
char* Buffer::BeginPtr_(){
    //解引用迭代器再返回指针，迭代器更抽象，指针更底层
    return &*buffer_.begin();
}


const char* Buffer::BeginPtr_() const {
    //只读
    return &*buffer_.begin();
}

void Buffer::Init_Buffer(){
    //使用memset将buffer_置０
    std::memset(BeginPtr_(), 0, buffer_.size());
    ReadPos_=0;
    WritePos_=0;
}
void Buffer::We_Should_Be_Enough_(size_t Length_We_Need){
    if(ReadPos_ + How_Many_Bytes_Can_We_Write()<Length_We_Need){
        //空间不足所有可以覆盖的空间(写过的-读过的＋还可以写的)小于Length_We_Need
        buffer_.resize(WritePos_+Length_We_Need+1);
    }else{
        //T_ReadPos表示待读的,将待读的移到首段，ReadPos_=0，WritePos_=T_Not_Read;
        size_t T_Not_Read=How_Many_Bytes_We_Need_Read();
        //copy函数(待拷贝首，待拷贝尾，目标拷贝地址)
        std::copy(BeginPtr_()+ReadPos_,BeginPtr_()+WritePos_,BeginPtr_());
        WritePos_=T_Not_Read;
        ReadPos_=0;
    }
}

size_t Buffer::How_Many_Bytes_Can_We_Write() const {
    return buffer_.size()-WritePos_;
}

size_t Buffer::How_Many_Bytes_We_Need_Read() const {
    return WritePos_-ReadPos_;
}

const char* Buffer::Where_Did_We_Read() const{
    return &*buffer_.begin()+ReadPos_;
}

void Buffer::Update_ReadPos(size_t Step_Lenght){
    //需要首先确认更新位置有效(前进距离小于等于带读取的长度)
    assert(Step_Lenght<=How_Many_Bytes_We_Need_Read());
    ReadPos_=ReadPos_+Step_Lenght;
}

void Buffer::Update_ReadPos(const char* Destination){
    assert(Where_Did_We_Read()<Destination&&Destination<=Where_Did_We_Write());
    Update_ReadPos(Destination-Where_Did_We_Read());
}

char* Buffer::Where_Did_We_Write(){
    return BeginPtr_()+WritePos_;
}

const char* Buffer::Where_Did_We_Write_Const() const {
    return BeginPtr_()+WritePos_;
}

void Buffer::Do_We_Have_Enough_Spase(size_t Length_We_Need){
    if(How_Many_Bytes_Can_We_Write()<Length_We_Need){
        We_Should_Be_Enough_(Length_We_Need);
    }
}

void Buffer::Write_to_Buffer(const char* Data,size_t Data_Length){
    assert(Data);//确认非空
    Do_We_Have_Enough_Spase(Data_Length);
    std::copy(Data,Data+Data_Length,Where_Did_We_Write());
    WritePos_=WritePos_+Data_Length;//更新写指针
}

void Buffer::Write_to_Buffer(const std::string& Data){
    Write_to_Buffer(Data.data(),Data.length());
}

void Buffer::Write_to_Buffer(const void *Data,size_t Data_length){
    assert(Data);
    Write_to_Buffer(static_cast<const char*>(Data),Data_length);
}

void Buffer::Write_to_Buffer(const Buffer& Data_of_Other_Buffer){
    Write_to_Buffer(Data_of_Other_Buffer.Where_Did_We_Read(),Data_of_Other_Buffer.How_Many_Bytes_We_Need_Read());
}

ssize_t Buffer::Get_Data(int FileD,int* Errono){
    //Buffer_Temple临时存储超界数据
    char Buffer_Temple[8192];
    //iovec是标明可以写入或读取的内存区的结构体，需要给出起点和长度，配合函数readv以及writev
    iovec  Ready_for_Input[2];
    const size_t Write_Space_We_Can_Use=How_Many_Bytes_Can_We_Write();
    Ready_for_Input[0].iov_base=BeginPtr_()+WritePos_;
    Ready_for_Input[0].iov_len=Write_Space_We_Can_Use;
    Ready_for_Input[1].iov_base=Buffer_Temple;
    Ready_for_Input[1].iov_len=sizeof(Buffer_Temple);
    //readv函数配合结构体iovec标明的内存区将文件内容读入内存区，可以多个内存区
    const ssize_t Length_We_Get_From_File=readv(FileD,Ready_for_Input,2);
    if(Length_We_Get_From_File<0){
        *Errono=errno;
        return Length_We_Get_From_File;
    }else if(static_cast<size_t>(Length_We_Get_From_File)<=Write_Space_We_Can_Use){
        WritePos_=WritePos_+Length_We_Get_From_File;
    }else{
        WritePos_=buffer_.size();
        Write_to_Buffer(Buffer_Temple,Length_We_Get_From_File-Write_Space_We_Can_Use);
    }return Length_We_Get_From_File;
}

ssize_t Buffer::Put_Data(int FileD,int *Errno){
    const size_t Read_Space_We_Can_Use=How_Many_Bytes_We_Need_Read();
    ssize_t Length_We_Write_to_File=write(FileD,Where_Did_We_Read(),Read_Space_We_Can_Use);
    if(Length_We_Write_to_File<0){
        *Errno=errno;
        return Length_We_Write_to_File;
    }
    ReadPos_=ReadPos_+Length_We_Write_to_File;
    return Length_We_Write_to_File;
}

std::string Buffer::Get_All_Data_String(){
    //string的构造函数
    std::string Data_Unread(Where_Did_We_Read(),How_Many_Bytes_We_Need_Read());
    Init_Buffer();
    return Data_Unread;
}