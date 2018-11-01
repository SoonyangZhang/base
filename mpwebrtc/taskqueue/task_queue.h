#ifndef NS3_TASK_QUEUE_H
#define NS3_TASK_QUEUE_H
#include <memory>
//#include <map>
#include <list>
#include <utility>
#include<stdint.h>
#include <stddef.h>
#include "rtc_base/platform_thread.h"
#include "rtc_base/criticalsection.h"
#include "minheap.h"
#include<vector>
template< class T, class M >
static inline constexpr ptrdiff_t offset_of( const M T::*member ) {
    return reinterpret_cast< ptrdiff_t >( &( reinterpret_cast< T* >( 0 )->*member ) );
}

template< class T, class M >
static inline constexpr T* owner_of( M *ptr, const M T::*member ) {
    return reinterpret_cast< T* >( reinterpret_cast< intptr_t >( ptr ) - offset_of( member ) );
}
#define my_container_of(ptr, type, member) \
     owner_of(ptr, &type::member)
namespace std
{
template<typename T, typename ...Args>
std::unique_ptr<T> make_unique( Args&& ...args )
{
    return std::unique_ptr<T>( new T( std::forward<Args>(args)... ) );
}
}
namespace zsy
{
class QueuedTask {
 public:
  QueuedTask() {}
  virtual ~QueuedTask() {}

  // Main routine that will run when the task is executed on the desired queue.
  // The task should return |true| to indicate that it should be deleted or
  // |false| to indicate that the queue should consider ownership of the task
  // having been transferred.  Returning |false| can be useful if a task has
  // re-posted itself to a different queue or is otherwise being re-used.
  virtual bool Run() = 0;
};
template <class Closure>
class ClosureTask : public QueuedTask {
 public:
  explicit ClosureTask(Closure&& closure)
      : closure_(std::forward<Closure>(closure)) {}

 private:
  bool Run() override {
    closure_();
    return true;
  }

  typename std::remove_const<
      typename std::remove_reference<Closure>::type>::type closure_;
};
template <class Closure, class Cleanup>
class ClosureTaskWithCleanup : public ClosureTask<Closure> {
 public:
  ClosureTaskWithCleanup(Closure&& closure, Cleanup&& cleanup)
      : ClosureTask<Closure>(std::forward<Closure>(closure)),
        cleanup_(std::forward<Cleanup>(cleanup)) {}
  ~ClosureTaskWithCleanup() { cleanup_(); }

 private:
  typename std::remove_const<
      typename std::remove_reference<Cleanup>::type>::type cleanup_;
};

// Convenience function to construct closures that can be passed directly
// to methods that support std::unique_ptr<QueuedTask> but not template
// based parameters.
template <class Closure>
static std::unique_ptr<QueuedTask> NewClosure(Closure&& closure) {
  return std::make_unique<ClosureTask<Closure>>(std::forward<Closure>(closure));
}

template <class Closure, class Cleanup>
static std::unique_ptr<QueuedTask> NewClosure(Closure&& closure,
                                              Cleanup&& cleanup) {
  return std::make_unique<ClosureTaskWithCleanup<Closure, Cleanup>>(
      std::forward<Closure>(closure), std::forward<Cleanup>(cleanup));
}
//std::map<key ,value> not tolerate diffent value have same key.
// but  std::priorty  seems having a cope operation of std::unique_ptr
struct TaskEvent{
TaskEvent(uint32_t d,std::unique_ptr<QueuedTask>t);
~TaskEvent();
int operator >(TaskEvent &b);
uint32_t delay;
std::unique_ptr<QueuedTask> task;
int min_heap_idx;
};
class TaskEventCpm{
public:
	int operator()(const TaskEvent &a,const TaskEvent &b);
};
class Ns3TaskQueue
{
public:
	Ns3TaskQueue();
	~Ns3TaskQueue();
	bool IsCurrent() const;
	void Start();
	void Stop();
    void Process();
  template <class Closure,
            typename std::enable_if<!std::is_convertible<
                Closure,
                std::unique_ptr<QueuedTask>>::value>::type* = nullptr>
    void PostTask(Closure&& closure) {
    PostTask(NewClosure(std::forward<Closure>(closure)));
  }
  template <class Closure,
            typename std::enable_if<!std::is_convertible<
                Closure,
                std::unique_ptr<QueuedTask>>::value>::type* = nullptr>
  void PostDelayedTask(Closure&& closure, uint32_t time_ms) {
    PostDelayedTask(NewClosure(std::forward<Closure>(closure)), time_ms);
  }
void PostDelayedTask(std::unique_ptr<QueuedTask> task, uint32_t time_ms);
void PostTask(std::unique_ptr<QueuedTask>task);
private:
	void Clear();
    static void ThreadMain(void* context);
	//std::map<uint64_t,std::unique_ptr<QueuedTask>> m_delayTasks; //will cause error when keys are same,
	//std::priority_queue<TaskEvent,std::vector<TaskEvent>,TaskEventCpm> m_delayTasks;
	bool m_running{false};
	rtc::PlatformThread  thread_;
	rtc::CriticalSection pending_lock_;
	min_heap<TaskEvent> s_heap_;
};
}
#endif

