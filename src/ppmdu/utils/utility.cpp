#include "utility.hpp"
//#include <Poco/DirectoryIterator.h>
#include "gfileutils.hpp"
#include <iostream>
#include <chrono>
#include <fstream>
#include <cassert>
using namespace std;

namespace utils
{
//============================================================================================
// Constants
//============================================================================================
    //const std::string CArgsParser::SWITCH_SYMBOL                 = "-";
    //const std::string CArgsParser::ADDITIONAL_INPUT_PARAM_SYMBOL = "+";

//============================================================================================
// MrChronometer
//============================================================================================
    MrChronometer::MrChronometer( const string name, std::ostream * messageoutput )
        :_name(name)
    {
        _start  = chrono::steady_clock::now();

        if( messageoutput == nullptr )
            _output = &(std::cout);
        else
            _output = messageoutput;
    }

    MrChronometer::~MrChronometer()
    {
        auto myduration = chrono::steady_clock::now() - _start;
        (*_output) << "#" <<_name << ": Time elapsed : " << chrono::duration_cast<std::chrono::milliseconds>( myduration ).count() << "ms\n";
    }

//============================================================================================
// CArgsParser
//============================================================================================

    //CArgsParser::CArgsParser( const char * argv[], int argc  )
    //    :m_rawargs( argv, argv + argc )
    //{
    //    ResetReadPos();
    //}

    //CArgsParser::CArgsParser( std::vector<optionparsing_t> optionlist, const char * argv[], int argc  )
    //    :m_optionlist(std::move(optionlist)), m_rawargs( argv, argv + argc )
    //{
    //    ResetReadPos();
    //}

    ////Return the reading pos to the beginning of the argument list
    //void CArgsParser::ResetReadPos()
    //{
    //    m_rawcurarg = m_rawargs.begin();
    //}

    //unsigned int CArgsParser::getNbArgsForOption( const std::string & option )
    //{
    //    auto itfound = m_optionlist.end();

    //    //find the corresponding option to get how many parameters
    //    for( auto itoption = m_optionlist.begin(); itoption != m_optionlist.end(); ++itoption )
    //    {
    //        if( option.compare( 1, (option.size() - 1), itoption->optionsymbol ) == 0 )
    //            itfound = itoption;
    //    }

    //    if( itfound != m_optionlist.end() )
    //        return itfound->nbvalues;
    //    else
    //        return 0;
    //}

    //void CArgsParser::skipCurrentParam()
    //{
    //    if( m_rawcurarg != m_rawargs.end() )
    //        ++m_rawcurarg;
    //}

    //std::string CArgsParser::getCurrentParam()
    //{
    //    for( auto itargs = m_rawcurarg; itargs != m_rawargs.end(); ++itargs )
    //    {
    //        if( itargs->front() == SWITCH_SYMBOL.front() )
    //        {
    //            //If we stumble on an option and have that option in our option list, 
    //            //  just increment by the ammount of parameter, the for loop will handle the other necessary increment
    //            auto nbparamsfound = getNbArgsForOption( *itargs );
    //            std::advance( itargs, nbparamsfound );
    //        }
    //        else if( itargs->front() == ADDITIONAL_INPUT_PARAM_SYMBOL.front() )
    //        {
    //            continue; //If we stumble on an additional input param, skip it
    //        }
    //        else
    //        {
    //            m_rawcurarg = itargs; //set our new current param position
    //            return *itargs;
    //        }
    //    }

    //    m_rawcurarg = m_rawargs.end(); //Ensure we know we don't have anything else
    //    return string();
    //}

    ////Return the next whitespace bounded string of character that isn't a switch/option
    //// If no more parameters, returns an empty string!
    //std::string CArgsParser::getNextParam()
    //{
    //    if( m_rawcurarg != m_rawargs.end() && (++m_rawcurarg) != m_rawargs.end() )
    //    {
    //        return getCurrentParam();
    //    }

    //    m_rawcurarg = m_rawargs.end(); //Ensure we know we don't have anything else
    //    return string();
    //}

    ////Returns whether the whole argument array has been scanned for arguments
    //bool CArgsParser::hasReachedTheEndOfArgList()const
    //{
    //    return m_rawcurarg != m_rawargs.end();
    //}

    ////Takes the index of the option from the option list passed at construction, if its the case!
    //std::vector< std::string > CArgsParser::getOption( std::vector<optionparsing_t>::size_type optionindex )
    //{
    //    vector<string> result;
    //    auto itfound = std::find( m_rawargs.begin(), m_rawargs.end(), (SWITCH_SYMBOL + m_optionlist[optionindex].optionsymbol) );

    //    if( itfound != m_rawargs.end() )
    //    {
    //        //Add the symbol to the output vector
    //        result.push_back(m_optionlist[optionindex].optionsymbol);

    //        //If we have extra params, get them if possible
    //        const auto nbtofetch = m_optionlist[optionindex].nbvalues;
    //        if( nbtofetch > 0 )
    //        {
    //            //Got to the next value
    //            ++itfound;

    //            //Fetch the specified amount of parameters
    //            for( unsigned int cptparams = 0; cptparams < nbtofetch; ++cptparams )
    //            {
    //                if( itfound != m_rawargs.end() )
    //                {
    //                    result.push_back( *itfound );
    //                    ++itfound;
    //                }
    //                else
    //                    result.push_back( string() ); //Push empty strings for any parameters we couldn't get
    //            }
    //        }
    //    }

    //    return result;

    //}

    ////Returns a list of all the found options considering the params passed at construction.
    //// -> The first entry in each std::vector<std::string> is the symbol of the option that was found
    //std::vector< std::vector<std::string> > CArgsParser::getAllFoundOptions()
    //{
    //    vector< vector< string > > foundoptions;

    //    //Iterate through our option list, and try to find all of them. Just put those we found into the vector!
    //    for( unsigned int cptopts = 0; cptopts < m_optionlist.size(); ++cptopts )
    //    {
    //        auto result = getOption( cptopts );

    //        if( !result.empty() )
    //            foundoptions.push_back(result);
    //    }

    //    return foundoptions;
    //}

    //bool CArgsParser::appendAllAdditionalInputParams( std::vector<std::string> & out_vtoappendto )
    //{
    //    bool bfoundsomething = false;

    //    for( auto & anarg : m_rawargs )
    //    {
    //        if( anarg.front() == ADDITIONAL_INPUT_PARAM_SYMBOL.front() )
    //        {
    //            out_vtoappendto.push_back( anarg.substr( 1 ) ); //Push the arg, except the "+" symbol
    //            bfoundsomething = true;
    //        }
    //    }

    //    return bfoundsomething;
    //}

//============================================================================================
// ReadStructWithArrayOperator
//============================================================================================
    //pmd2::types::constitbyte_t ReadStructWithArrayOperator( pmd2::types::constitbyte_t itbeg, data_array_struct * pstruct )
    //{
    //    for( unsigned int i = 0; i < pstruct->size(); ++i, ++itbeg )
    //        (*pstruct)[i] = *itbeg;

    //    return itbeg;
    //}

    //pmd2::types::itbyte_t WriteStructWithArrayOperator( const data_array_struct * pstruct, pmd2::types::itbyte_t itoutbeg )
    //{
    //    for( unsigned int i = 0; i < pstruct->size(); ++i, ++itoutbeg )
    //        (*itoutbeg) = (*pstruct)[i];

    //    return itoutbeg;
    //}


//============================================================================================
// Simple Exception Handler
//============================================================================================
    void SimpleHandleException( exception & e )
    {
        assert(false);
        cerr << "<!>-EXCEPTION: " <<e.what() <<endl;
    }



};