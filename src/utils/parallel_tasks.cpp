#include "parallel_tasks.hpp"

using namespace std;

namespace utils
{

//======================================================================================================================================
//  TaskQueue
//======================================================================================================================================
    const std::chrono::microseconds TaskQueue::WaitForNewTaskTime(100);
    /*
        Push a task at the back of the queue.
    */
    void TaskQueue::Push( TaskQueue::task_t && task )
    {
        std::lock_guard<std::mutex> lck(m_queuemtx);
        m_taskqueue.push_back(std::forward<task_t>(task));
    }

    /*
        Try to pop from the queue. Return false if pop failed, true otherwise.
    */
    bool TaskQueue::TryPop( TaskQueue::task_t & out_task )
    {
        if(!empty())
        {
            std::lock_guard<std::mutex> lck(m_queuemtx);
            out_task = std::move(m_taskqueue.front());
            m_taskqueue.pop_front();
            return true;
        }
        return false;
    }

    /*
        Try to pop from the queue, and wait until the delay is reached, or the queue has a new task. Return false if pop failed and/or delay expired, true otherwise.
    */
    bool TaskQueue::TryPopWait( TaskQueue::task_t & out_task )
    {
        if(empty())
        {
            std::unique_lock<std::mutex> lcknewtask(m_waitfortaskmtx);
            m_cvnewtask.wait_for(lcknewtask, WaitForNewTaskTime);
        }
        //Recheck condition
        return TryPop(out_task);
    }

//======================================================================================================================================
//  Worker
//======================================================================================================================================
#if 0
    const std::chrono::microseconds Worker::WaitOnTaskTime(1);
    Worker::Worker()
        :m_bshoulrun(false), m_ptaskqueue(nullptr)
    {}

    Worker::Worker( TaskQueue & tq )
        :m_bshoulrun(true), m_ptaskqueue(&tq)
    {}

    Worker::Worker( const Worker & cp )
        :m_bshoulrun(cp.m_bshoulrun.load()),
            m_ptaskqueue(cp.m_ptaskqueue),
            m_excepts(cp.m_excepts)
    {
        if(cp.m_mythread.joinable())
            Start();
    }

    Worker & Worker::operator=( const Worker & cp )
    {
        m_bshoulrun  = cp.m_bshoulrun.load();
        m_ptaskqueue = cp.m_ptaskqueue;
        m_excepts    = cp.m_excepts;
        if(cp.m_mythread.joinable())
            Start();
        return *this;
    }

    Worker::~Worker()
    {
        try
        {
            Stop(); //Need to stop
            TryJoin();
        }
        catch(...){}
    }

    /*
    */
    inline bool Worker::IsValid()const{return m_ptaskqueue != nullptr;}

    /*
        Returns whether the worker is working on a task or not
    */
    inline bool Worker::IsBusy()const {return m_bisbusy;}

    inline void Worker::Start()
    {
        if(!IsValid())
            throw std::runtime_error("Worker::Start(): Worker thread is in an undefined state!");
        m_mythread = std::thread(&Worker::Work, this);
    }

    /*
        This is what the worker thread executes.
    */
    void Worker::Work()
    {
        do
        {
            TaskQueue::task_t mytask;
            if( m_ptaskqueue->TryPopWait(mytask) && mytask.valid() )
                RunATask(mytask);
            else
                WaitForTask();
        }
        while(m_bshoulrun);
    }

    /*
        This tells the thread to stop, and tries to join it, then pops any exceptions.
    */
    inline void Worker::Stop() 
    { 
        if(!IsValid())
            throw std::runtime_error("Worker::Stop(): Worker thread is in an undefined state!");
        m_bshoulrun = false;
    }

    inline void Worker::WaitStop()
    {
        Stop();
        TryJoin();
        do
        {
            TryPopException();
        }
        while(!m_excepts.empty());
    }

    bool Worker::TryJoin()
    {
        if(!IsValid())
            throw std::runtime_error("Worker::TryJoin(): Worker thread is in an undefined state!");
        if( m_mythread.joinable() )
        {
            m_mythread.join();
            return true;
        }
        return false;
    }

    void Worker::TryPopException()
    {
        if(!IsValid())
            throw std::runtime_error("Worker::TryPopException(): Worker thread is in an undefined state!");
        if( !m_excepts.empty() )
        {
            std::lock_guard<std::mutex> lck(m_exceptmtx);
            std::exception_ptr          eptr = m_excepts.front();
            m_excepts.pop_front();
            std::rethrow_exception(eptr);
        }
    }


    inline void Worker::PushException( std::exception_ptr ptr )
    {
        std::lock_guard<std::mutex> lck(m_exceptmtx);
        m_excepts.push_back(ptr);
    }

    inline void Worker::RunATask( TaskQueue::task_t & curtask )
    {
        try
        {
            m_bisbusy = true;
            curtask();
        }
        catch(...)
        {
            PushException(std::current_exception());
        }
        m_bisbusy= false;
    }

    inline void Worker::WaitForTask()
    {
        this_thread::sleep_for(WaitOnTaskTime);
    }

#endif


//======================================================================================================================================
//  
//======================================================================================================================================

};