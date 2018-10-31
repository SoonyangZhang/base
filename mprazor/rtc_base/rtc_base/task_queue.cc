/*
 * ns3taskqueue.cc
 *
 *  Created on: 2018.6.19
 *      Author: zsy
 *      email 865678017@qq.com
 */
#include "rtc_base/task_queue.h"
#include "rtc_base/timeutils.h"
namespace rtc
{
TaskEvent::TaskEvent(uint32_t d,std::unique_ptr<QueuedTask>t){
	delay=d;
	task=std::move(t);
}
TaskEvent::~TaskEvent(){}
int TaskEvent::operator>(TaskEvent &b){
	return delay>b.delay;
}

Ns3TaskQueue::Ns3TaskQueue():thread_(&Ns3TaskQueue::ThreadMain,this,"taskqueue")
{
	min_heap_ctor(&s_heap_);
    Start();
}
Ns3TaskQueue::~Ns3TaskQueue(){
	Clear();
	min_heap_dtor(&s_heap_);
}
void Ns3TaskQueue::Clear(){
	while(!min_heap_empty(&s_heap_)){
	TaskEvent *e=min_heap_top(&s_heap_);
	min_heap_pop(&s_heap_);
	delete e;
	}
}
bool Ns3TaskQueue::IsCurrent() const{return true;}
void Ns3TaskQueue::Start()
{
    m_running=true;
    thread_.Start();
}
void Ns3TaskQueue::Stop()
{
    m_running=false;
    thread_.Stop();
}
void Ns3TaskQueue::Process()
{
   //well that seems better,make the lock part as small as possible
	while(m_running)
	{
        uint64_t time_ms=TimeMillis();
        std::list<std::unique_ptr<QueuedTask>> pending_tasks;
        {
            CritScope lock(&pending_lock_);
            while(!min_heap_empty(&s_heap_)){
            TaskEvent *event=min_heap_top(&s_heap_);
            if(event->delay<=time_ms){
            pending_tasks.push_back(std::move(event->task));
            delete event;
            min_heap_pop(&s_heap_);
            }else{
            break;
            }
            }
	    }
	    while(!pending_tasks.empty())
	    {
	        std::unique_ptr<QueuedTask> task=std::move(pending_tasks.front());//move ownership
	        pending_tasks.pop_front();
	        task->Run();
	        //task.release();// this will cause memory leakgy
	    }            
    }  
}
void Ns3TaskQueue::PostTask(std::unique_ptr<QueuedTask>task)
{
    uint64_t delta=0;
	PostDelayedTask(std::move(task),delta);
}
void Ns3TaskQueue::PostDelayedTask(std::unique_ptr<QueuedTask> task, uint32_t time_ms)
{

    uint64_t now=TimeMillis();
    uint64_t future=now+time_ms;
    TaskEvent *event=new TaskEvent(future,std::move(task));
    CritScope lock(&pending_lock_);
    min_heap_push(&s_heap_,event);
    //m_delayTasks.insert(std::make_pair(future,std::move(task)));
}
void Ns3TaskQueue::ThreadMain(void* context)
{
    Ns3TaskQueue *task=static_cast<Ns3TaskQueue*>(context);
    task->Process();
}
}
