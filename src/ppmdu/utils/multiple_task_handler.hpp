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

namespace multitask
{
    //Typedefs

    //Since MS screwed up with the C++ 11 support in MSVC 2012, I had to do this.. 
    // Packaged tasks with void return types won't compile.. And they didn't patch it.. Ever..
    // That's really rude.. GCC doesn't have that issue..
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

    private:
        //Methods
        void RunTasks();
        //void AssignTasks( std::vector< std::pair<std::future<pktaskret_t>,std::thread> > & futthreads );
        bool WorkerThread( std::atomic_bool * out_isworking, int id );

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
    };
};
#endif