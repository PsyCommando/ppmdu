#ifndef LIBRARY_WIDE_HPP
#define LIBRARY_WIDE_HPP
/*
library_wide.hpp
2014/10/20
psycommando@gmail.com
Description: 
    Contains a singleton holding information relevant to how most of the library should behave.
    Basically, a form of clean "globals" !

    #TODO: Use a map with named parameters instead ! It'll make it much more versatile. 
           And name it "SharedArguments".
           Then, individual application can implement their own wrapper over that that
           just parses the relevant data for the duration of the execution.

           Though, having some basic shared arguments might help. Like log on, and display progress?
*/
#include <locale>
#include <string>
#include <unordered_map>
#include <memory>
#include <utils/multithread_logger.hpp>

namespace utils
{
    /***************************************************************
        lwData
            Values to be shared library wide.
            Stored inside a struct.
            This might eventually get more elaborate at one point.
    ***************************************************************/
    class lwData
    {
    public:
        lwData();

        //Verbose
        void setVerbose( bool ison );
        bool isVerboseOn()const;

        //Nb threads to use at most
        void         setNbThreadsToUse( unsigned int nbthreads );
        unsigned int getNbThreadsToUse()const;

        inline void isLogOn( bool state ){ m_LoggingOn = state; }
        inline bool isLogOn()const       { return m_LoggingOn;  }

        inline bool ShouldDisplayProgress()const     {return m_displayProgress;}
        inline void ShouldDisplayProgress(bool bdisp){ m_displayProgress = bdisp; }

        inline logging::BaseLogger & Logger()                          
        { 
            static logging::DummyLogger dumlog;
            if(m_plog)
                return *m_plog; 
            else
                return dumlog;
        }
        inline void Logger(logging::BaseLogger * logger) { return m_plog.reset(logger); }

        /*
            Common values between all programs
        */
        enum struct eBasicValues
        {
            ProgramLogDir,
            ProgramExeDir,
            NbValues,
        };

        //! #TODO: Polish this, and integrate it!!
        inline std::string       & StringValue(eBasicValues val)      { return m_baseval[val];}
        inline const std::string & StringValue(eBasicValues val)const { return m_baseval.at(val);}

        inline std::string       & StringValue(const std::string & valname)      { return m_sharedvalues[valname];}
        inline const std::string & StringValue(const std::string & valname)const { return m_sharedvalues.at(valname);}

    private:
        bool         m_displayProgress;
        bool         m_verboseOn;
        bool         m_LoggingOn;
        unsigned int m_nbThreads;
        std::unique_ptr<logging::BaseLogger> m_plog;

        std::unordered_map<eBasicValues,std::string> m_baseval;
        std::unordered_map<std::string, std::string> m_sharedvalues;
    };

    /***************************************************************
        LibraryWide
            Singleton that init the library-wide settings.
    ***************************************************************/
    class LibraryWide
    {
    public:
        static LibraryWide & getInstance();

        //
        lwData & Data();

    private:
        LibraryWide();
        LibraryWide( const LibraryWide & );
        LibraryWide & operator=( const LibraryWide & );

        lwData  m_data;
    };

    /*
        LibWide
            Convenience function for getting the lib wide options.
    */
    lwData & LibWide();



    /*
        Conditional Log functions
            Used to log things to file, only if logging is enabled!
    */

    //Prefix <!>- to text and log into clog, only if log enabled.
    void LogError   ( const std::string & text );

    //Prefix <!>- to text and log into clog, only if log enabled.
    void LogWarning ( const std::string & text );

    //Prefix <*>- to text and log into clog, only if log enabled.
    void LogMessage ( const std::string & text );

    //Log into clog as-is, only if log enabled.
    void LogTextAsIs( const std::string & text );

    namespace logutil
    {
        //Helper function to hide the whole logger mechanism!
        inline std::ostream & slog()
        {
            return ::utils::LibWide().Logger().Log();
        }
    };
};

#endif