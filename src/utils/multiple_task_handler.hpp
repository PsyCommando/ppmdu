#ifndef MULTIPLE_TASK_HANDLER_HPP
#define MULTIPLE_TASK_HANDLER_HPP
/*
multiple_task_handler.hpp
2014/09/19
psycommando@gmail.com
Description: This is basically a system for handling multiple tasks in parallel.
             something close to a threadpool. 

             #TODO: Need testing.
*/
#include <thread>
#include <mutex>
#include <future>
#include <functional>
#include <deque>
#include <atomic>
#include <queue>

namespace multitask
{
    //Typedefs
    typedef bool                              pktaskret_t; 
    typedef std::packaged_task<pktaskret_t()> pktask_t;

    /*
        CMultiTaskHandler
            Class meant to manage a task queue and a small threadpool. It process tasks in parallel, and provide means to 
            wait for the end of the execution, or stop it/pause it.
    */
    class CMultiTaskHandler
    {
    public:
        CMultiTaskHandler();
        ~CMultiTaskHandler();

        //Add task
        void AddTask( pktask_t && task ); 

        //This waits until all tasks have been processed before returning!
        void BlockUntilTaskQueueEmpty();

        //Start the manager's thread and begins handling tasks.
        // If the thread is running, it does nothing.
        void Execute();

        //Stops the execution of the thread after the current task is completed
        // If the thread is stopped, it does nothing.
        void StopExecute();

        //Returns whether there are still tasks to run in the queue
        bool HasTasksToRun()const;

        //Exception handling from worker threads
        std::exception_ptr PopException();

    private:

        struct thRunParam
        {
            std::atomic<bool>        runningTask;  //Whether the thread is running a task or not
            std::chrono::nanoseconds waitTime;     //the time this thread will wait in-between checks for a new task
        };

        //Push exception to the exception queue
        void PushException( std::exception_ptr ex );

        //The manager thread run this
        void RunTasks();

        //The method that worker threads run
        bool WorkerThread( thRunParam & taskSlot );

        //Don't let anynone copy or move construct us
        CMultiTaskHandler( const CMultiTaskHandler & );
        CMultiTaskHandler( CMultiTaskHandler && );
        CMultiTaskHandler& operator=(const CMultiTaskHandler&);
        CMultiTaskHandler& operator=(CMultiTaskHandler&&);

        //Variables
        std::thread                                  m_managerThread;

        std::mutex                                   m_mutextasks;
        std::deque<pktask_t>                         m_tasks;

        std::mutex                                   m_mutexTaskFinished;
        std::condition_variable                      m_lastTaskFinished;

        std::mutex                                   m_mutexNewTask;
        std::condition_variable                      m_newTask;

        std::atomic_bool                             m_NoTasksYet; //This is to avoid blocking 
                                                                   //on the condition variable in 
                                                                   //case of wait on an empty task queue after init
        std::atomic_bool                             m_managerShouldStopAftCurTask;
        std::atomic_bool                             m_stopWorkers;

        std::atomic<int>                             m_taskcompleted;

        //Exception handling
        std::mutex                                   m_exceptionMutex;
        std::queue<std::exception_ptr>               m_exceptions;         
    };
};
#endif