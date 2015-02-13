#include "multiple_task_handler.hpp"
#include "library_wide.hpp"
#include "utility.hpp"
#include <thread>
#include <mutex>
#include <future>
#include <functional>
#include <algorithm>
#include <iostream>
#include <chrono>

using namespace std;
using namespace utils;

#ifdef _DEBUG
    //#define THREAD_VERBOSE_STATE //This tells the thread to specify when they're working or not
#endif

namespace multitask
{
//================================================================================================
// Constants
//================================================================================================
    static const chrono::nanoseconds DUR_WORKER_THREAD_WAIT_FOR_TASK = chrono::nanoseconds(10);
    static const chrono::nanoseconds DUR_WORKER_THREAD_WAIT          = chrono::nanoseconds(10);
    static const chrono::nanoseconds DUR_MANAGER_THREAD_WAIT         = chrono::nanoseconds(10);

//================================================================================================
// Utility
//================================================================================================
    //void doHandleException( exception & e )
    //{
    //    assert(false);
    //    cerr<< "<!>-EXCEPTION: " <<e.what() <<endl;
    //}



//================================================================================================
// MultiTaskHandler 
//================================================================================================
    CMultiTaskHandler::CMultiTaskHandler()
    {
        m_NoTasksYet.store( true );
        m_managerShouldStopAftCurTask.store( false );
    }

    CMultiTaskHandler::~CMultiTaskHandler()
    {
        //Put directly here to force it
        if( m_managerThread.joinable() )
        {
            m_managerShouldStopAftCurTask = true;
            m_managerThread.join();
        }
    }

    void CMultiTaskHandler::AddTask( pktask_t && task )
    {
        //we have a task to run, so these this to false
        m_NoTasksYet = false;

        try
        {
            lock_guard<mutex> mylock( m_mutextasks );
            m_tasks.push_back( std::move(task) );
        }
        catch( exception e ){SimpleHandleException(e);}

        try
        {
            unique_lock<mutex> mynewtasklock(m_mutexNewTask);
            m_newTask.notify_one(); //FIXME: This condition variable intermitently make the program crash for no obvious reasons at this line.. 
        }
        catch( exception e ){SimpleHandleException(e);}
    }

    void CMultiTaskHandler::BlockUntilTaskQueueEmpty()
    {
        if( m_NoTasksYet ) //If we never had a single task, don't bother
            return;

        try
        {
            unique_lock<mutex> ulock( m_mutexTaskFinished );
            m_lastTaskFinished.wait( ulock );
        }
        catch( exception e ){SimpleHandleException(e);}
    }


    //Start the manager's thread and begins handling tasks.
    // If the thread is running, it does nothing.
    void CMultiTaskHandler::Execute()
    {
        if( !m_managerThread.joinable() )
        {
            m_managerShouldStopAftCurTask = false;
            m_managerThread = std::move( std::thread(&CMultiTaskHandler::RunTasks, this) );
        }
    }

    //Stops the execution of the thread after the current task is completed
    // If the thread is stopped, it does nothing.
    void CMultiTaskHandler::StopExecute()
    {
        if( m_managerThread.joinable() )
        {
            m_managerShouldStopAftCurTask = true;
            m_managerThread.join();
        }
    }

    //Returns whether there are still tasks to run in the queue
    bool CMultiTaskHandler::HasTasksToRun()const
    {
        return !(m_tasks.empty());
    }

    bool CMultiTaskHandler::WorkerThread( atomic_bool * out_isworking, int id )
    {
        while( !( m_managerShouldStopAftCurTask.load() ) )
        {
            packaged_task<pktaskret_t()> mytask;
            bool                         shouldwaitfornewtask = false;

            //Grab a task if possible
            try
            {
                lock_guard<mutex> mylg( m_mutextasks );

                if( !m_tasks.empty() )
                {
                    (*out_isworking) = true;
                    mytask = std::move(m_tasks.front());
                    m_tasks.pop_front();
                }
                else
                    shouldwaitfornewtask = true;
            }
            catch( exception e ){SimpleHandleException(e);}

            //Run the task
            if( mytask.valid() )
            {
                mytask();
                (*out_isworking) = false;
            }
            else
                (*out_isworking) = false;

            //Wait a few more microsec if the queue is empty, while hoping for a trigger of the cond var
            if( shouldwaitfornewtask )
            {
                try
                {
                    unique_lock<mutex> ulock( m_mutexNewTask );
                    m_newTask.wait_for( ulock, DUR_WORKER_THREAD_WAIT_FOR_TASK );
                }
                catch( exception e ){SimpleHandleException(e);}
            }
            else
            {
                try
                {
                    this_thread::sleep_for(DUR_WORKER_THREAD_WAIT);
                }
                catch( exception e ){SimpleHandleException(e);}
            }
        }

        return true;
    }

    //This is what the manager thread runs!
    void CMultiTaskHandler::RunTasks()
    {
        auto                                        nbthreads = LibraryWide::getInstance().Data().getNbThreadsToUse();
        vector<thread>                              threadpool(nbthreads);
        vector<atomic_bool>                         taskstates(nbthreads); 
        bool                                        hasnotaskrunning = false;

        //Instantiate threads 
        for( unsigned int i = 0; i < threadpool.size(); ++i )
            threadpool[i] = std::move( thread( &CMultiTaskHandler::WorkerThread, this, &(taskstates[i]), i ) );

        //Check when all tasks are done
        while( !( m_managerShouldStopAftCurTask.load() && hasnotaskrunning ) )
        {
            //If all tasks are done set "hasnotaskrunning" to true!
            hasnotaskrunning = all_of( taskstates.begin(), taskstates.end(), [](atomic_bool& astate){ return !astate; } );

            //Trigger the all task finished cond var if the task queue is empty, and no tasks are running
            if( hasnotaskrunning )
            {
                bool notasks = false;

                try
                {
                    lock_guard<mutex> mylg( m_mutextasks );
                    notasks = m_tasks.empty();
                }
                catch( exception e ){SimpleHandleException(e);}

                if( notasks )
                {
                    try
                    {
                        lock_guard<mutex> taskfinished(m_mutexTaskFinished);
                        m_lastTaskFinished.notify_all();
                    }
                    catch( exception e ){SimpleHandleException(e);}
                }
            }

            try
            {
                this_thread::sleep_for(DUR_MANAGER_THREAD_WAIT);
            }
            catch( exception e ){SimpleHandleException(e);}
        }

        //Wait for all threads to finish
        for( unsigned int i = 0; i < threadpool.size(); ++i )
        {
            if( threadpool[i].joinable() )
                threadpool[i].join();
        }
    }
};