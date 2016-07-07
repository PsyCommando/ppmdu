#include "library_wide.hpp"
#include <thread>
#include <iostream>
#include <iomanip>
using namespace std;

namespace utils
{
    lwData & LibWide()
    {
        return LibraryWide::getInstance().Data();
    }

//=========================================================================
// LWData
//=========================================================================
    lwData::lwData()
        :m_verboseOn(false), m_nbThreads(std::max( thread::hardware_concurrency()/2u, 1u )), m_LoggingOn(false),
        m_displayProgress(true)
    {
    }

    void lwData::setVerbose( bool ison ) 
    { 
        m_verboseOn = ison; 
    }

    bool lwData::isVerboseOn()const      
    { 
        return m_verboseOn; 
    }

    void lwData::setNbThreadsToUse( unsigned int nbthreads ) 
    { 
        m_nbThreads = nbthreads; 
    }

    unsigned int lwData::getNbThreadsToUse()const            
    { 
        return m_nbThreads; 
    }

//=========================================================================
// Library Wide
//=========================================================================
    LibraryWide::LibraryWide()
    {
    }

    LibraryWide & LibraryWide::getInstance()
    {
        static LibraryWide instance;
        return instance;
    }

    //
    lwData & LibraryWide::Data()
    {
        return m_data;
    }


//
//
//
    void LogError  ( const std::string & text )
    {
        if( LibWide().isLogOn() )
            clog << "<!>- " <<text;
    }

    void LogMessage( const std::string & text )
    {
        if( LibWide().isLogOn() )
            clog << "<*>- " <<text;
    }

    void LogTextAsIs( const std::string & text )
    {
        if( LibWide().isLogOn() )
            clog << text;
    }
};