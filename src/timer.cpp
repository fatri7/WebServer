#include"timer.h"

void TimerManager::siftup_(size_t index){
    assert(index>=0&&index<heap_.size());
    //计算当前节点的父节点索引。在二叉堆中，父节点的索引是(子节点索引 - 1) / 2
    size_t j=(index-1)/2;
    while (j>=0)
    {
        //只要当前节点有父节点
        if(heap_[j]<heap_[index]){
            //父节点的值小于当前节点的值，说明堆的性质已经满足，不需要继续调整
            break;
        }
        //交换这两个节点的内容，以维护堆的性质
        swap_node_(index,j);
        //准备进行下一轮的比较和交换
        index=j;
        j=(index-1)/2;
    }
}
bool TimerManager::siftdown_(size_t index,size_t n){
    assert(index>=0&&index<heap_.size());
    assert(n>=0&&n<=heap_.size());
    size_t i=index;
    //j是i的左子节点的索引
    size_t j=i*2+1;
    while(j<n){
        if(j+1<n&&heap_[j+1]<heap_[j]){
            //检查右子节点是否存在并且是否小于左子节点，如果是，则将 j 更新为右子节点的索引
            j++;
        }else if(heap_[i]<heap_[j]){
            //当前节点小于其子节点，则不需要继续下沉，跳出循环
            break;
        }
        //交换当前节点与其子节点的位置
        swap_node_(i,j);
        //继续下一轮
        i=j;
        j=i*2+1;
    }
    //表示节点是否发生了移动
    return i> index;
}
void TimerManager::swap_node_(size_t index1,size_t index2){
    assert(index1>=0&&index1<heap_.size());
    assert(index2>=0&&index2<heap_.size());
    //swap函数不会改变对象的地址，它只是交换对象的值
    std::swap(heap_[index1],heap_[index2]);
    //更新引用索引
    ref_[heap_[index1].id]=index1;
    ref_[heap_[index2].id]=index2;    
}
void TimerManager::add_timer(int id,int time_out,const timeout_callback& call_back){
    assert(id>=0);
    size_t i;
    if(ref_.count(id)==0){
        //新节点：堆尾插入，调整堆
        i=heap_.size();
        ref_[id]=i;
        heap_.push_back({id,hr_clock::now()+ms(time_out),call_back});
        siftup_(i);
    }else{
        //已有结点：调整堆
        i=ref_[id];
        heap_[i].expire=hr_clock::now()+ms(time_out);
        heap_[i].call_back=call_back;
        if(!siftdown_(i,heap_.size())){
            siftup_(i);
        }
    }
}
void TimerManager::work(int id){
    //删除指定id结点，并触发回调函数
    if(heap_.empty()||ref_.count(id)==0){
        return;
    }
    size_t i=ref_[id];
    timer_node node=heap_[i];
    node.call_back();
    del_(i);
}
void TimerManager::del_(size_t index){
    assert(!heap_.empty()&&index>=0&&index<heap_.size());
    //将要删除的结点换到队尾，然后调整堆
    size_t i=index;
    size_t n=heap_.size()-1;
    assert(i<=n);
    if(i<n){
        swap_node_(i,n);
        if(!siftdown_(i,n)){
            siftup_(i);
        }
    }
    //删除队尾元素
    ref_.erase(heap_.back().id);
    heap_.pop_back();
}
void TimerManager::update(int id,int time_out){
    assert(!heap_.empty()&&ref_.count(id)!=0);
    //更新与id相关联的定时器的超时时间
    heap_[ref_[id]].expire=hr_clock::now()+ms(time_out);
    siftdown_(ref_[id],heap_.size());
}
void TimerManager::handle_expired_event(){
    while(!heap_.empty()){
        timer_node node=heap_.front();
        if(std::chrono::duration_cast<ms>(node.expire-hr_clock::now()).count()>0){
            //检查定时器是否到期
            break;
        }
        node.call_back();
        pop();
    }
}
void TimerManager::pop(){
    assert(!heap_.empty());
    del_(0);
}
void TimerManager::clear(){
    ref_.clear();
    heap_.clear();
}
int TimerManager::get_next_timer_handle(){
    handle_expired_event();
    ssize_t res=-1;
    if(!heap_.empty()){
        //如果计算结果res小于0，说明堆顶定时器已经到期，将res设置为0
        res=std::chrono::duration_cast<ms>(heap_.front().expire-hr_clock::now()).count();
        if(res<0){
            res=0;
        }
    }
    return res;
}