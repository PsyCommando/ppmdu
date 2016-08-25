#ifndef PARALLEL_TASKS_HPP
#define PARALLEL_TASKS_HPP
/*
parallel_tasks.hpp
2016/08/24
psycommando@gmail.com
Description: A set of utilities for handling multi-threaded tasks execution. Meant to replace the previous implementation.
*/
#include <utils/library_wide.hpp>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <deque>
#include <future>

namespace utils
{

    class TaskQueue
    {
    public:
        typedef std::packaged_task<void()> task_t;

        /*
            Push a task at the back of the queue.
        */
        void Push( task_t && task );

        /*
            Try to pop from the queue. Return false if pop failed, true otherwise.
        */
        bool TryPop( task_t & out_task );

        /*
            Try to pop from the queue, and wait until the delay is reached, or the queue has a new task. Return false if pop failed and/or delay expired, true otherwise.
        */
        bool TryPopWait( task_t & out_task );

        inline bool     empty()const    {return m_taskqueue.empty();}
        inline size_t   size()const     {return m_taskqueue.size();}


    private:
        std::mutex              m_queuemtx;
        std::deque<task_t>      m_taskqueue;

        std::mutex              m_waitfortaskmtx;
        std::condition_variable m_cvnewtask;
        static const std::chrono::microseconds WaitForNewTaskTime;
    };


//======================================================================================================================================
//  ThreadedTasks
//======================================================================================================================================
#if 0
    /*
    */
    class Worker
    {
    public:

        Worker();
        Worker(TaskQueue & tq);
        Worker(const Worker & cp);
        Worker & operator=( const Worker & cp );
        ~Worker();

        /*
        */
        inline bool IsValid()const{return m_ptaskqueue != nullptr;}

        /*
            Returns whether the worker is working on a task or not
        */
        inline bool IsBusy()const {return m_bisbusy;}

        inline void Start();

        /*
            This is what the worker thread executes.
        */
        void Work();

        /*
            This tells the thread to stop, and tries to join it, then pops any exceptions.
        */
        inline void Stop();
        inline void WaitStop();

        bool        TryJoin();
        void        TryPopException();

    private:
        inline void PushException( std::exception_ptr ptr );
        inline void RunATask( TaskQueue::task_t & curtask );
        inline void WaitForTask();

    private:
        TaskQueue                     * m_ptaskqueue;
        std::thread                     m_mythread;
        std::mutex                      m_exceptmtx;
        std::deque<std::exception_ptr>  m_excepts;
        std::atomic_bool                m_bshoulrun;
        std::atomic_bool                m_bisbusy;
        static const std::chrono::microseconds WaitOnTaskTime;
    };

#endif

//======================================================================================================================================
//  AsyncTasks
//======================================================================================================================================
    class AsyncWorker
    {
    public:

        AsyncWorker( TaskQueue & q )
            :m_ptasks(&q), m_bshouldwork(true)
        {}

        AsyncWorker( const AsyncWorker & cp ) 
            :m_ptasks(cp.m_ptasks), 
             m_bshouldwork(cp.m_bshouldwork.load())
        {
            if(cp.m_myfut.valid() && m_bshouldwork)
                Start();
        }

        AsyncWorker& operator=( const AsyncWorker & cp ) 
        {
            m_ptasks      = cp.m_ptasks;
            m_bshouldwork = cp.m_bshouldwork.load();
            if(cp.m_myfut.valid() && m_bshouldwork)
                Start();
            return *this;
        }

        ~AsyncWorker()
        {
            try
            {
                Stop();
                if(m_myfut.valid())
                    m_myfut.wait_for(std::chrono::milliseconds(1000));
            }
            catch(...)
            {}
        }

        inline bool IsValid()const
        {
            return m_ptasks != nullptr;
        }

        inline void Start()
        {
            m_bshouldwork = true;
            m_myfut       = std::move(std::async(std::launch::async, &AsyncWorker::Work, this));
        }

        inline void Stop()
        {
            m_bshouldwork = false;
        }

        inline void WaitStop()
        {
            Stop();
            if(m_myfut.valid())
                m_myfut.wait();
        }

        inline void WaitFinished()
        {
            if(m_myfut.valid())
                m_myfut.wait();
        }

    private:
        void Work()
        {
            while(!m_ptasks->empty() && m_bshouldwork)
            {
                TaskQueue::task_t mytask;
                if(m_ptasks->TryPopWait(mytask) && mytask.valid())
                {
                    mytask();
                }
            }
        }

    private:
        TaskQueue       * m_ptasks;
        std::atomic_bool  m_bshouldwork;
        std::future<void> m_myfut;
    };

    class AsyncTaskHandler
    {
    public:
        typedef TaskQueue::task_t task_t;

        AsyncTaskHandler()
            :m_bshouldrun(false)
        {
            const size_t nbth = utils::LibWide().getNbThreadsToUse();
            for( size_t cnt = 0; cnt < nbth; ++cnt )
                m_workers.emplace(m_workers.end(), m_taskqueue);
        }

        ~AsyncTaskHandler(){}

        inline void QueueTask( task_t && t )
        {
            m_taskqueue.Push(std::forward<task_t>(t));
        }

        inline void Start()
        {
            m_bshouldrun = true;
            for( auto & w : m_workers )
                w.Start();
        }

        inline void Stop()
        {
            m_bshouldrun = false;
            for( auto & w : m_workers )
                w.Stop();
        }

        inline void WaitStop()
        {
            m_bshouldrun = false;
            for( auto & w : m_workers )
                w.WaitStop();
        }

        inline void WaitTasksFinished()
        {
            for( auto & w : m_workers )
                w.WaitFinished();
        }

        inline bool empty()const {return m_taskqueue.empty();}

    private:
        TaskQueue                   m_taskqueue;
        std::vector<AsyncWorker>    m_workers;
        std::atomic_bool            m_bshouldrun;
    };
};

#endif
