#include "cmdline_util.hpp"
#include <vector>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <iomanip>


using namespace std;

namespace utils{ namespace cmdl
{
//============================================================================================
// Constants
//============================================================================================
    const std::string CArgsParser::SWITCH_SYMBOL                 = "-";
    const std::string CArgsParser::ADDITIONAL_INPUT_PARAM_SYMBOL = "+";


//============================================================================================
//  RAIIClogRedirect
//============================================================================================

    RAIIClogRedirect::RAIIClogRedirect( const std::string & logfilename )
    {
        m_filebuf.open( logfilename, std::ios::out );
        m_oldbuf = std::clog.rdbuf( &m_filebuf );
    }

    RAIIClogRedirect::~RAIIClogRedirect()
    {
        std::clog.rdbuf( m_oldbuf );
    }


//============================================================================================
// CArgsParser
//============================================================================================

    CArgsParser::CArgsParser( const char * argv[], int argc  )
        :m_rawargs( argv, argv + argc )
    {
        ResetReadPos();
    }

    CArgsParser::CArgsParser( std::vector<optionparsing_t> optionlist, const char * argv[], int argc  )
        :m_optionlist(std::move(optionlist)), m_rawargs( argv, argv + argc )
    {
        ResetReadPos();
    }

    //Return the reading pos to the beginning of the argument list
    void CArgsParser::ResetReadPos()
    {
        m_rawcurarg = m_rawargs.begin();
    }

    unsigned int CArgsParser::getNbArgsForOption( const std::string & option )
    {
        auto itfound = m_optionlist.end();

        //find the corresponding option to get how many parameters
        for( auto itoption = m_optionlist.begin(); itoption != m_optionlist.end(); ++itoption )
        {
            if( option.compare( 1, (option.size() - 1), itoption->optionsymbol ) == 0 )
                itfound = itoption;
        }

        if( itfound != m_optionlist.end() )
            return itfound->nbvalues;
        else
            return 0;
    }

    void CArgsParser::skipCurrentParam()
    {
        if( m_rawcurarg != m_rawargs.end() )
            ++m_rawcurarg;
    }

    std::string CArgsParser::getCurrentParam()
    {
        for( auto itargs = m_rawcurarg; itargs != m_rawargs.end(); ++itargs )
        {
            if( itargs->front() == SWITCH_SYMBOL.front() )
            {
                //If we stumble on an option and have that option in our option list, 
                //  just increment by the ammount of parameter, the for loop will handle the other necessary increment
                auto nbparamsfound = getNbArgsForOption( *itargs );
                std::advance( itargs, nbparamsfound );
            }
            else if( itargs->front() == ADDITIONAL_INPUT_PARAM_SYMBOL.front() )
            {
                continue; //If we stumble on an additional input param, skip it
            }
            else
            {
                m_rawcurarg = itargs; //set our new current param position
                return *itargs;
            }
        }

        m_rawcurarg = m_rawargs.end(); //Ensure we know we don't have anything else
        return string();
    }

    //Return the next whitespace bounded string of character that isn't a switch/option
    // If no more parameters, returns an empty string!
    std::string CArgsParser::getNextParam()
    {
        if( m_rawcurarg != m_rawargs.end() && (++m_rawcurarg) != m_rawargs.end() )
        {
            return getCurrentParam();
        }

        m_rawcurarg = m_rawargs.end(); //Ensure we know we don't have anything else
        return string();
    }

    //Returns whether the whole argument array has been scanned for arguments
    bool CArgsParser::hasReachedTheEndOfArgList()const
    {
        return m_rawcurarg == m_rawargs.end();
    }

    //Takes the index of the option from the option list passed at construction, if its the case!
    std::vector< std::string > CArgsParser::getOption( std::vector<optionparsing_t>::size_type optionindex )
    {
        vector<string> result;
        auto itfound = std::find( m_rawargs.begin(), m_rawargs.end(), (SWITCH_SYMBOL + m_optionlist[optionindex].optionsymbol) );

        if( itfound != m_rawargs.end() )
        {
            //Add the symbol to the output vector
            result.push_back(m_optionlist[optionindex].optionsymbol);

            //If we have extra params, get them if possible
            const auto nbtofetch = m_optionlist[optionindex].nbvalues;
            if( nbtofetch > 0 )
            {
                //Got to the next value
                ++itfound;

                //Fetch the specified amount of parameters
                for( unsigned int cptparams = 0; cptparams < nbtofetch; ++cptparams )
                {
                    if( itfound != m_rawargs.end() )
                    {
                        result.push_back( *itfound );
                        ++itfound;
                    }
                    else
                        result.push_back( string() ); //Push empty strings for any parameters we couldn't get
                }
            }
        }

        return result;

    }

    //Returns a list of all the found options considering the params passed at construction.
    // -> The first entry in each std::vector<std::string> is the symbol of the option that was found
    std::vector< std::vector<std::string> > CArgsParser::getAllFoundOptions()
    {
        vector< vector< string > > foundoptions;

        //Iterate through our option list, and try to find all of them. Just put those we found into the vector!
        for( unsigned int cptopts = 0; cptopts < m_optionlist.size(); ++cptopts )
        {
            auto result = getOption( cptopts );

            if( !result.empty() )
                foundoptions.push_back(result);
        }

        return foundoptions;
    }

    //Returns a list of all the found parameters passed at construction.
    std::vector< std::string > CArgsParser::getAllFoundParams()
    {
        vector<string> allparams; 
        auto           itbefore = m_rawcurarg; //save the initial read pos to restore the object to its previous state
        ResetReadPos(); //make sure we're at the beginning

        while( !hasReachedTheEndOfArgList() )
        {
            auto nextpara = getNextParam();
            if( nextpara.size() > 0 )
                allparams.push_back( nextpara );
        }

        m_rawcurarg = itbefore; //restore initial state
        return std::move(allparams); 
    }

    //Returns a list of all the found extra parameters passed at construction.
    std::vector< std::string > CArgsParser::getAllAdditionalParams()
    {
        vector<string> addparam;
        appendAllAdditionalInputParams( addparam );
        return std::move( addparam );
    }

    bool CArgsParser::appendAllAdditionalInputParams( std::vector<std::string> & out_vtoappendto )
    {
        bool bfoundsomething = false;

        for( auto & anarg : m_rawargs )
        {
            if( anarg.front() == ADDITIONAL_INPUT_PARAM_SYMBOL.front() )
            {
                out_vtoappendto.push_back( anarg.substr( 1 ) ); //Push the arg, except the "+" symbol
                bfoundsomething = true;
            }
        }

        return bfoundsomething;
    }

//============================================================================================
// CommandLineUtility
//============================================================================================
    
    void CommandLineUtility::PrintTitle()
    {
        stringstream strswholetitle;
        string       wholetitle;

        strswholetitle <<getTitle() <<" - " <<getVersionString();
        wholetitle = strswholetitle.str();

        string headerbars( wholetitle.length() + 6u, '=' );

	    cout << headerbars << "\n"
             << "== " <<wholetitle <<" ==\n" 
             << headerbars << "\n"
             << getShortDescription() << "\n"
             << endl;
    }

    void CommandLineUtility::PrintReadme()
    {
        static const string               BulletChr = "->";
        static const string               OptTag    = "(opt)";
        const vector<argumentparsing_t> & refMyArgs = getArgumentsList();
        const vector<optionparsing_t>   & refMyOpts = getOptionsList();
        const argumentparsing_t         * myExtra   = getExtraArg();
        bool                              bDisplayOptValLegend = false; //Whether we got at least one option that has a value
        unsigned int                      longestargname    = 0;
        unsigned int                      longestoptionname = 0;

        if( !refMyOpts.empty() )
        {
            bDisplayOptValLegend = std::all_of( refMyOpts.begin(), refMyOpts.end(), []( const optionparsing_t& opt ){ return opt.nbvalues > 0u; } ); 

            //Get the longest option name to properly indent things later on!
            longestoptionname = std::max_element( refMyOpts.begin(), refMyOpts.end(), 
                                                  [](const optionparsing_t& opt1, const optionparsing_t & opt2)
                                                  { 
                                                      return (opt1.optionsymbol.length() < opt2.optionsymbol.length()); 
                                                  } )->optionsymbol.length();
        }

        // -- Print the demo line --
        cout << getExeName() << " ";

        //Mention the format for options if applicable!
        if( !refMyOpts.empty() )
        {
            cout << "(option";

            if(bDisplayOptValLegend)
                cout << " \"optionvalue\") ";
            else 
                cout << ") ";
        }

        //Mention the format for parameters if applicable!
        vector<string> argumentsinorder(refMyArgs.size());
        if( !refMyArgs.empty() )
        {
            for( const auto & anarg : refMyArgs )
            {
                stringstream strs;

                //Get the longest argument name to get the proper indent
                if( longestargname < anarg.name.length() )
                {
                    //Add a little extra to fit the optional tag if neccessary
                    if( anarg.isoptional )
                        longestargname = anarg.name.length() + OptTag.length();
                    else
                        longestargname = anarg.name.length();
                }

                if( anarg.isoptional )
                    strs <<"(" <<anarg.name <<") ";
                else
                    strs <<anarg.name <<" ";
                argumentsinorder.at(anarg.argumentorder) = strs.str();
            }

            //Write them out
            for( auto & astring : argumentsinorder )
                cout << astring;
        }

        cout<<"\n";

        // -- Write the legend --
        // For options:
        if( !refMyOpts.empty() )
        {
            cout <<"  " <<BulletChr <<left <<setw(longestargname) <<setfill(' ') 
                 << " option" <<": An option from the list below..(optional)\n";
            if(bDisplayOptValLegend)
            {
                cout <<"  " <<BulletChr <<left <<setw(longestargname) <<setfill(' ') 
                     << " optionvalue" <<": An optional value for the specified option..\n";
            }
        }

        // For parameters:
        for( auto & anarg : refMyArgs )
        {
            stringstream argname;

            if( anarg.isoptional )
                argname << anarg.name <<OptTag;
            else
                argname << anarg.name;

            cout <<"  " <<BulletChr <<left <<setw(longestargname) <<setfill(' ')
                 <<argname.str() <<": " <<anarg.description <<"\n";
        }

        // For Extra:
        if( myExtra != nullptr )
        {
            cout << "Extra/Batch Parameter(s):\n"; 
            cout <<"  +" <<left <<setw(longestargname) <<setfill(' ') 
                 <<myExtra->name <<": " <<myExtra->description <<"\n";
        }


        cout<<"\n\n";

        // -- Write Option details --
        cout << "Options:\n";

        for( auto & aopt : refMyOpts )
            cout <<"  -" <<left  <<setw(longestoptionname) <<setfill(' ') <<aopt.optionsymbol <<right <<": " <<aopt.description <<"\n";

        cout <<"\n\n";

        // -- Write Example Lines --
        cout <<"Examples:\n"
             <<getExeName() <<" ";

        // Write options
        for( auto & anopt : refMyOpts )
            cout <<anopt.example << " ";

        // Write Params
        for( auto & aparam : refMyArgs )
            cout <<aparam.example << " ";

        //Write extra param
        if( myExtra != nullptr )
            cout <<"+" <<myExtra->example << " ";

        cout<<"\n\n";

        // -- Write Utility Description --
        cout <<"Description:\n"
             <<getLongDescription()
             <<endl;

        // -- Write Thanks, Credits, Copyrights, Misc --
        cout <<"----------------------------------------------------------\n"
             <<getMiscSectionText() <<"\n" 
             <<endl;
    }
    
    void CommandLineUtility::parseArgs( CArgsParser & argsparse )
    {
        const auto &   refParams = getArgumentsList();
        vector<string> params    = argsparse.getAllFoundParams();
        //bool           bsuccess = true;

        for( unsigned int i = 0; i < refParams.size(); ++i )
        {
            unsigned int argorder = refParams[i].argumentorder;

            if( argorder < params.size() )
            {
                if( ! refParams[i].myParseFun( params[argorder] ) )
                {
                    stringstream strserror;
                    strserror <<"<!>- Error while parsing \"" <<params[argorder] 
                              <<"\"\nas parameter \"" <<refParams[i].name <<"\"!";
                    throw std::runtime_error(strserror.str());
                }
            }
            else if( !(refParams[i].isoptional) )
            {
                stringstream strserror;
                strserror <<"<!>- Error! Command line is missing the parameter: \"" <<refParams[i].name <<"\"!";
                throw exMissingParameter(strserror.str());
            }
            //else // We missed an optional param, no big deal!
        }
    }

    void CommandLineUtility::parseOptions( CArgsParser & argsparse )
    {
        const auto &           refOptions = getOptionsList();
        vector<vector<string>> rawoptions = argsparse.getAllFoundOptions();

        for( auto & anopt : refOptions )
        {
            //Try to find an option with one of the symbols we got!
            vector<vector<string>>::const_iterator itFoundRaw = 
                std::find_if( rawoptions.begin(), 
                              rawoptions.end(), 
                              [&anopt](const vector<string>& av )->bool
                              {
                                return (av.front().compare( anopt.optionsymbol ) == 0 ) ;
                              } );

            //Parse the found option, if applicable
            if( itFoundRaw != rawoptions.end() && !anopt.myOptionParseFun( *itFoundRaw ) )
            {
                //If parsing fails !
                stringstream strserror;
                strserror <<"<!>- Error while parsing option : \"" <<(anopt.optionsymbol) <<"\"";

                if( itFoundRaw->size() > 1  )
                {
                    strserror<<"\nWith value(s) :\n";
                    for( unsigned int cptfopt = 0; cptfopt < itFoundRaw->size(); ++cptfopt )
                        strserror <<cptfopt <<" : " <<(*itFoundRaw)[cptfopt] <<"\n";
                }

                throw exMissingParameter(strserror.str());
            }
        }
    }

    bool CommandLineUtility::parseExtraArgs( CArgsParser & argsparse, bool bAbortOnError )
    {
        const auto & refExtraParam = getExtraArg();

        if( refExtraParam == nullptr )
            return false;
        
        auto addparams = argsparse.getAllAdditionalParams();

        for( auto & aparam : addparams )
        {
            if( ! refExtraParam->myParseFun(aparam) )
            {
                stringstream strserror;
                strserror <<"<!>- Warning! Error parsing extra parameter : " <<aparam <<"\n"
                          <<"Skipping..\n";

                if( bAbortOnError )
                    throw std::runtime_error(strserror.str());
                else
                    cerr << strserror.str();
            }
        }

        return true;
    }

};};