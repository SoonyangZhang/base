#include <iostream>
#include<stdint.h>
#include <unistd.h>
#include "rtc_base/timeutils.h"
#include "rtc_base/task_queue.h"
using namespace std;
using namespace rtc;
class Test
{
public:
    Test(Ns3TaskQueue*taskQueue){m_queue=taskQueue;}
    ~Test(){}
    void Increase()
    {
        m_counter++;
        printf("hello world%d\n",m_counter);
    }
    void TestClosure()
    {
        m_queue->PostTask([this]{
        this->Increase();
                          });
    }
    void TestDelay(uint32_t delay)
    {
        printf("in test %d\n",delay);
        m_queue->PostDelayedTask([this,delay]{
                 this->Increase();
                 printf("%d\n",delay);
                                 },delay);
    }
private:
    Ns3TaskQueue *m_queue;
    uint32_t m_counter{0};

};
int main()
{
    Ns3TaskQueue taskQueue;
    Test test(&taskQueue);
    test.TestDelay(4);
    test.TestDelay(8);
    test.TestDelay(2);
    test.TestDelay(4);
    uint32_t elapse=20;
    uint32_t now=TimeMillis();
    uint32_t stop=now+elapse;
    while(TimeMillis()<stop)
    {

    }
    taskQueue.Stop();

}
