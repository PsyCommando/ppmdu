#ifndef MULTI_THREAD_LOGGER_HPP
#define MULTI_THREAD_LOGGER_HPP
/*
mutlithread_logger.hpp
2016/08/01
psycommando@gmail.com
Description: A simple and quick implementation of a multi-thread logger.
             Unsuitable for heavy duty work, but only really basic simple applications where reliability isn't critical.
*/
#include <iostream>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <fstream>
#include <cassert>
#include <atomic>
#include <future>
#include "gfileutils.hpp"
#include "library_wide.hpp"

namespace logging
{
    /***********************************************************************************
        Meant to be used with a fixed threadpool. 
    ***********************************************************************************/
    namespace Strategy
    {
        class OneOutputPerThread{};
        class OneOutputForAll{};
    };

    /***********************************************************************************
        ILogger
            Interface for loggers.
    ***********************************************************************************/
    class ILogger
    {
    public:
        virtual ~ILogger(){}
        virtual std::ostream & Log() = 0;

    };

    /***********************************************************************************
        ThreadSafeFileLogger
        
    ***********************************************************************************/
    template<class _StrategyTy> class ThreadSafeFileLogger;


    /***********************************************************************************
        ThreadSafeFileLogger
            Shared output for multi-thread application.

            It prints out the thread's output in order on destruction, or when flush is called!
    ***********************************************************************************/
    template<>
        class ThreadSafeFileLogger<Strategy::OneOutputForAll> : public ILogger
    {
    public:
        ThreadSafeFileLogger( std::thread::id mainthid, std::ostream * output )
            :m_mainthredlogout(output), m_mainthread(mainthid)
        {}

        virtual ~ThreadSafeFileLogger()
        {
            try
            {
                _Flush();
            }
            catch(const std::exception & e)
            {
                assert(false);
                //If we get an exception, just print it to std stream and continue destruction
                std::cerr <<"<!>-ThreadSafeFileLogger::~ThreadSafeFileLogger(): Caught exception on destruction while flushing! " << e.what()
                     <<"\nIgnoring, and finishing destruction!\n";
            }
        }

        void SetupThreadLogger( std::thread::id thid )
        {
            SetupOrGetThreadLog(thid);
        }

        //Should never be null!!
        void SetMainThreadLogStream( std::ostream * mainlog )
        {
            m_mainthredlogout = mainlog;
        }

        void SetMainThreadID( std::thread::id thid )
        {
            m_mainthread = thid;
        }
    
        /*
            Returns the correct log stream for the current thread
            Main thread always goes through first, and never needs flushing!
        */
        std::ostream & Log()
        {
            std::ostream * pos = GetOstreamForThread( std::this_thread::get_id() );
            assert(pos);
            if(pos)
                return *pos;
            else
                throw std::runtime_error("ThreadSafeFileLogger::Log(): Error, logstream is null! This should never happen!");
        }

        void Flush()
        {
            std::lock_guard<std::mutex> loclock(m_logtblmutex); //Locks adding things
            _Flush();
        }

    private:

        void _Flush()
        {
            for(auto& log : m_logstreams)
            {
                (*m_mainthredlogout) << "\n====================\nLog Thread ID " <<log.first <<"\n====================\n";
                (*m_mainthredlogout) << log.second.out.str();
                (*m_mainthredlogout).flush();
            }
        }

        std::ostream * SetupOrGetThreadLog( std::thread::id thid )
        {
            std::lock_guard<std::mutex> loclock(m_logtblmutex);
            const auto result = m_logstreams.emplace( thid, logwrap{std::stringstream()} );
            if(result.second)
                result.first->second.out.exceptions( std::stringstream::badbit );
            return std::addressof( (result.first->second.out) );
        }

        std::ostream * GetOstreamForThread(std::thread::id thid)
        {
            if( thid == m_mainthread )
                return m_mainthredlogout;
            else
                return SetupOrGetThreadLog(thid);
        }

    private:
        struct logwrap
        {
            std::stringstream out;
        };

        std::mutex                                   m_logtblmutex;
        std::thread::id                              m_mainthread;
        std::ostream                               * m_mainthredlogout;
        std::unordered_map<std::thread::id, logwrap> m_logstreams;
    
    };

    /***********************************************************************************
        ThreadSafeFileLogger
            Multiple outputs for each threads!
    ***********************************************************************************/
    template<>
        class ThreadSafeFileLogger<Strategy::OneOutputPerThread> : public ILogger
    {
    public:
        ThreadSafeFileLogger( std::thread::id mainthid, std::ostream * mainlog )
            :m_mainthredlogout(mainlog), m_mainthread(mainthid)
        {}

        virtual ~ThreadSafeFileLogger(){}

        void SetupThreadLogger( std::thread::id thid )
        {
            SetupOrGetThreadLog(thid);
        }

        //Should never be null!!
        void SetMainThreadLogStream( std::ostream * mainlog )
        {
            m_mainthredlogout = mainlog;
        }

        void SetMainThreadID( std::thread::id thid )
        {
            m_mainthread = thid;
        }
    
        /*
            Returns the correct log stream for the current thread
        */
        std::ostream & Log()
        {
            std::ostream * pos = GetOstreamForThread( std::this_thread::get_id() );
            assert(pos);
            if(pos)
                return *pos;
            else
                throw std::runtime_error("ThreadSafeFileLogger::Log(): Error, logstream is null! This should never happen!");
        }

    private:
        inline const std::string & GetLogDirectory()
        {
            return utils::LibWide().StringValue(utils::lwData::eBasicValues::ProgramLogDir);
        }

        std::ostream * SetupOrGetThreadLog( std::thread::id thid )
        {
            std::lock_guard<std::mutex> loclock(m_logtblmutex);
            const auto result = m_logstreams.emplace( thid, std::ofstream() );
            if(result.second)
            {
                std::stringstream sstout;
                sstout <<utils::TryAppendSlash(GetLogDirectory()) <<"logthread_id" <<thid <<".log";
                result.first->second.open( sstout.str() );
                result.first->second.exceptions( std::ofstream::badbit );
            }
            return std::addressof( (result.first->second) );
        }

        std::ostream * GetOstreamForThread(std::thread::id thid)
        {
            if( thid == m_mainthread )
                return m_mainthredlogout;
            else
                return SetupOrGetThreadLog(thid);
        }

    private:
        std::mutex                                         m_logtblmutex;
        std::thread::id                                    m_mainthread;
        std::ostream                                     * m_mainthredlogout;
        std::unordered_map<std::thread::id, std::ofstream> m_logstreams;
    };
};

#endif // !MULTI_THREAD_LOGGER_HPP

