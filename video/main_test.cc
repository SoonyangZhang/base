#include <iostream>
#include "rtc_base/bind.h"
#include "rtc_base/thread.h"
#include "rtc_base/location.h"
#include "rtc_base/win32socketserver.h"
//#include  <windows.h>
#include <stdio.h>
#include <signal.h>
	class TestThread :public rtc::Thread{
	public:
		TestThread() {}
		~TestThread() {}
		void ThreadStart();
		void ThreadStop();
		virtual void Run() override;
	protected:
		bool					running_{false};

};
	void TestThread::Run() {
#if WIN32
		// Need to pump messages on our main thread on Windows.
		//rtc::Win32Thread w32_thread;
#endif
		while (running_)
		{
			{// ProcessMessages
				this->ProcessMessages(10);
			}
#if WIN32
			//w32_thread.ProcessMessages(1);
#else
			usleep(1000);
#endif
		}
	}
	void TestThread::ThreadStart() {
		running_ = true;
		rtc::Thread::SetName("TestThread", this);
		rtc::Thread::Start();
	}
	void TestThread::ThreadStop() {
		if (running_) {
			running_ = false;
			rtc::Thread::Stop();
		}
	}

class Test{
	public:
	Test(rtc::Thread *t):w_(t){}
	~Test(){}
	void RunTask(){
	w_->Invoke<void>(RTC_FROM_HERE, rtc::Bind(&Test::Print, this, nullptr));
	}
	void Print(void *handle){
	printf("hello test\n");
	}
	private:
	rtc::Thread *w_;
};
bool m_running=true;
void signal_exit_handler(int sig)
{
	m_running=false;
}
int main(){
	signal(SIGTERM, signal_exit_handler);
    signal(SIGINT, signal_exit_handler);
    signal(SIGTSTP, signal_exit_handler);
	TestThread th;
	th.ThreadStart();
	Test test(&th);
	test.RunTask();
	while(m_running){
	}
	th.ThreadStop();
	return 0;
}

