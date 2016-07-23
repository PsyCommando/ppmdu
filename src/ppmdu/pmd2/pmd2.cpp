#include "pmd2.hpp"
#include <sstream>
#include <iomanip>
#include <utils/poco_wrapper.hpp>
#include <iostream>
#include <unordered_map>
#include <regex>
using namespace std;

namespace pmd2
{
    const std::unordered_map<std::string, std::string> CommonEscapeCharactersToRaw
    {{
        { "\\n",  "\n"  }, //End of line
        { "\\t",  "   " }, //Replace tabs with 3 spaces
        { "\\'",  "\'"  }, //Single quote
        { "\\\"", "\""  }, //Double quote
        { "\\\\", "\\"  }, //Backslash
        { "\\0",  "\0"  }, //Ending 0
    }};

    const std::unordered_map<char, std::string> CharactersToCommonEscapeCharacters
    {{
        { '\n', "\\n"  }, //End of line
        //{ '\'', "\\'"  }, //Single quote
        //{ '\"', "\\\"" }, //Double quote
        { '\\', "\\\\" }, //Backslash
        { '\0', "\\0"  }, //Ending 0
    }};

    const std::unordered_map<char, std::string> CharactersToCommonEscapeCharactersXML
    {{
        { '\n',     "&#x0A;"    },
        { '\"',     "&quot;"    },
        { '&',      "&amp;"     },
        { '\'',     "&apos;"    },
        { '<',      "&lt;"      },
        { '>',      "&gt;"      },
        { '\x85',   "&hellip;"  },
        { '\x8C',   "&OElig;"   },
        { 'é',      "&#xE9;"    },
        //!TODO: finish
    }};


    const std::array<std::string, static_cast<size_t>(eGameVersion::NBGameVers)> GameVersionNames =
    {
        "EoS",
        "EoT",
        "EoD",
        //"EoTD",
    };

    const std::array<std::string,static_cast<size_t>(eGameRegion::NBRegions)> GameRegionNames =
    {
        "Japan",
        "NorthAmerica",
        "Europe",
    };

    const std::array<std::string, static_cast<size_t>(eGameLanguages::NbLang)> GameLanguagesNames=
    {{
        "English",
        "Japanese",
        "French",
        "German",
        "Italian",
        "Spanish",
    }};

    /*
        Directories present in all versions of PMD2
    */
    const std::array<std::string,13> PMD2BaseDirList
    {
        DirName_BACK,
        DirName_BALANCE,
        DirName_DUNGEON,
        DirName_EFFECT,
        DirName_FONT,
        DirName_GROUND,
        DirName_MAP_BG,
        DirName_MESSAGE,
        DirName_MONSTER,
        DirName_SCRIPT,
        DirName_SOUND,
        DirName_SYSTEM,
        DirName_TOP,
    };

    /*
        Directories unique to EoS!
    */
    const std::array<std::string,3> PMD2EoSExtraDirs
    {
        DirName_RESCUE,     //EoS Only
        DirName_SYNTH,      //EoS Only
        DirName_TABLEDAT,   //EoS Only
    };

    eGameVersion AnalyzeDirForPMD2Dirs( const std::string & pathdir )
    {
        auto                flist         = utils::ListDirContent_FilesAndDirs( pathdir,false );
        unsigned int        cntmatches    = 0;
        static const size_t NbMatchesEoTD = PMD2BaseDirList.size();
        static const size_t NbMatchesEoS  = PMD2BaseDirList.size() + PMD2EoSExtraDirs.size();


        for( const auto & fname : flist )
        {
            auto itfoundbase = std::find( PMD2BaseDirList.begin(), PMD2BaseDirList.end(), fname );
            if( itfoundbase != PMD2BaseDirList.end() )
                ++cntmatches;
            else
            {
                auto itfoundeos = std::find( PMD2EoSExtraDirs.begin(), PMD2EoSExtraDirs.end(), fname );
                if( itfoundeos != PMD2EoSExtraDirs.end() )
                    ++cntmatches;
            }
        }

        if( cntmatches == NbMatchesEoTD )
            return eGameVersion::EoT;
        else if( cntmatches == NbMatchesEoS )
            return eGameVersion::EoS;
        else if( cntmatches > NbMatchesEoTD )
        {
            clog << "AnalyzeDirForPMD2Dirs(): Directory contains some, but not all Explorers of Sky directories! Handling as Explorers of Time/Darkness.\n";
            return eGameVersion::EoT;
        }
        else
            return eGameVersion::Invalid;
    }

    std::pair<eGameVersion, eGameRegion> DetermineGameVersionAndLocale(const std::string & pathfilesysroot)
    {
        std::pair<eGameVersion, eGameRegion> resultpair;

        //Determine version
        std::stringstream                    sstr;
        sstr << pathfilesysroot << "/" <<DirName_BALANCE << "/" <<FName_MonsterMND;
        std::string                          pathmonstermnd = sstr.str();
        sstr.str(string());
        sstr.clear();
        sstr << pathfilesysroot << "/" <<DirName_BALANCE;
        std::string                          pathbalancedir = sstr.str();

        if(utils::pathExists(pathbalancedir) )
        {
            if( utils::isFile(pathmonstermnd) )
                resultpair.first = eGameVersion::EoT;
            else 
                resultpair.first = eGameVersion::EoS;
        }
        else
            resultpair.first = eGameVersion::Invalid;

        eGameVersion resultdiranalysis =  AnalyzeDirForPMD2Dirs(pathfilesysroot);

        if( resultdiranalysis != resultpair.first && 
            resultdiranalysis != eGameVersion::Invalid && 
            resultpair.first != eGameVersion::Invalid )
        {
            resultpair.first = resultdiranalysis; //This has priority over monster mnd!
        }

        //Determine Locale
        sstr.str(string());
        sstr.clear();
        sstr << pathfilesysroot << "/" <<DirName_MESSAGE;
        auto filelst = utils::ListDirContent_FilesAndDirs( sstr.str(), true );
        bool bhasEnglish  = false;
        bool bhasJapanese = false;
        bool bhaseurolang = false;

        //Search the text files for the relevant names
        for( const auto & fname : filelst)
        {
            auto itfound = std::search( fname.begin(), fname.end(), FName_TextPref.begin(), FName_TextPref.end() );

            if( itfound != fname.end() )
            {
                ++itfound; //Skip _ character
                if( itfound != fname.end() )
                {
                    char langid = *itfound;

                    if( langid == FName_TextEngSufx.front() )
                        bhasEnglish = true;
                    else if( langid == FName_TextJapSufx.front() )
                    {
                        bhasJapanese = true;
                        break;
                    }
                    else if( langid == FName_TextFreSufx.front() || langid == FName_TextGerSufx.front() || langid == FName_TextItaSufx.front() ||
                             langid == FName_TextSpaSufx.front() || langid == FName_TextFreSufx.front()  )
                    {
                        bhaseurolang = true;
                        break;
                    }
                }
                else
                    continue;
            }
        }

        if( bhasEnglish && !bhaseurolang )
            resultpair.second = eGameRegion::NorthAmerica;
        else if( bhaseurolang )
            resultpair.second = eGameRegion::Europe;
        else if (bhasJapanese)
            resultpair.second = eGameRegion::Japan;
        else
            resultpair.second = eGameRegion::Invalid;
        
        return std::move(resultpair);
    }

    //const std::string & GetGameVersionName( eGameVersion gv )
    //{
    //    return GameVersionNames[static_cast<size_t>(gv)];
    //}

//======================================================================================
//  Character Escaping
//======================================================================================
    std::unordered_map<std::string, std::string> PMD2SpecificCharsToEscape
    {{
        //{ "\x8C", "\\x8C" }, //eo

        //{ "\xE0", "\\xE0" }, //à

        //{ "\xE2", "\\xE2" }, //â

        //{ "\xE8", "\\xE8" }, //è
        //{ "\xE9", "\\xE9" }, //é
        //{ "\xEA", "\\xEA" }, //ê
       
        //{ "\xEE", "\\xEE" }, //î

        //{ "\xF4", "\\xF4" }, //ô

        //{ "\xF9", "\\xF9" }, //ù
        { "\x81\xF4", "\\x81\\xF4" }, //
    }};



    /*
        ParsePmd2EscapeSeq
            Parse escape characters sequence used in pmd2 script files. Ex: "~27"
    */
    template<class _init>
        bool ParsePmd2EscapeSeq( _init & itc, _init & itend, std::string & out )
    {
        static const regex EoTDEscapeSeq("\x7E([0-9a-fA-F]{2})");
        if( c == 0x7E &&                         //0x7E is the escape sequence marker in EoT and EoD games!
            std::distance(itc, itend) >= 3 ) //Check if we have enough space for the digits
        {
            smatch sm;
            if( regex_match( itc, itc+3, sm,EoTDEscapeSeq ) && sm.size() > 2 )
            {
                stringstream sstr;
                uint16_t     val = 0;
                sstr <<hex <<"0x" <<sm[2].str(); //Get the digits
                sstr >>val;
                out.push_back(static_cast<char>(val));
                return true;
            }
        }
        return false;
    }

    /*
        EscapeCharacter
            
    */
    template<bool _EscapeXML = false>
        inline void EscapeCharacter(char c, std::string & out);

    template<>
        inline void EscapeCharacter<false>(char c, std::string & out)
    {
        //auto itf = CharactersToCommonEscapeCharacters.find(c);
        //if( itf != CharactersToCommonEscapeCharacters.end() )
        //    out.append(itf->second);
        //else
        {
            stringstream sstr;
            sstr <<"\\x" <<hex <<uppercase <<(0x00FF & static_cast<uint16_t>(c));
            out.append(std::move(sstr.str()));
        }
    }

    template<bool _EscapeXML = false>
        inline void EscapeCharacter<true>(char c, std::string & out)
    {
        //auto itf = CharactersToCommonEscapeCharactersXML.find(c);
        //if( itf != CharactersToCommonEscapeCharactersXML.end() )
        //    out.append(itf->second);
        //else
        {
            stringstream sstr;
            sstr <<"&#x" <<hex <<uppercase <<(0x00FF & static_cast<uint16_t>(c)) <<";";
            out.append(std::move(sstr.str()));
        }
    }

    //template<bool _EscapeXML = false>
    //    inline bool IsPrintable( char c, const std::locale & loc );

    ////For XML output, we want to escape even characters in range!
    //template<>
    //    inline bool IsPrintable<true>( char c, const std::locale & loc )
    //{
    //    return std::isprint(c, loc) && ();
    //}

    //template<>
    //    inline bool IsPrintable<false>( char c, const std::locale & loc )
    //{
    //    return std::isprint(c, loc);
    //}

    template<bool _EscapeXML>
        inline bool HandleCommonEscapes( char c, std::string & out );

    template<>
        inline bool HandleCommonEscapes<true>( char c, std::string & out )
    {
        auto itf = CharactersToCommonEscapeCharactersXML.find(c);
        if( itf != CharactersToCommonEscapeCharactersXML.end() )
        {
            out.append(itf->second);
            return true;
        }
        return false;
    }

    template<>
        inline bool HandleCommonEscapes<false>( char c, std::string & out )
    {
        auto itf = CharactersToCommonEscapeCharacters.find(c);
        if( itf != CharactersToCommonEscapeCharacters.end() )
        {
            out.append(itf->second);
            return true;
        }
        return false;
    }



    static const unsigned char ShiftJIS_Marker     = 0x81;   //Those bytes are the bytes prefixed to Shift-JIS characters used in PMD2.
    static const unsigned char ShiftJIS_MarkerLast = 0x84;

    template<bool _EscapeForXML>
        void HandleCharacters( const std::string & src, std::string & out, bool escapejis, const std::locale & loc )
    {
        for( auto itc = src.begin(); itc != src.end(); ++itc )
        {
            unsigned char c = *itc;
            //Shift JIS characters should not be escaped in japanese, but in any other locale
            if( !HandleCommonEscapes<_EscapeForXML>(c,out) )
            {
                if( c >= ShiftJIS_Marker && c <= ShiftJIS_MarkerLast && (itc + 1) != src.end() )
                {
                    if(!escapejis)
                    {
                        out.append(itc, itc+2 ); //We just parse the characters as-is
                        ++itc;
                    }
                    else
                    {
                        EscapeCharacter<_EscapeForXML>(c,       out);  //If not japanese, go straight to escaping the character!
                        EscapeCharacter<_EscapeForXML>(*(++itc),out);
                    }
                }
                else if( std::isprint(c, loc) )
                    out.push_back(c);
                else
                    EscapeCharacter<_EscapeForXML>(c,out); //If all fails, escape it
            }
        }
    }


    /*
    */
    std::string EscapeUnprintableCharacters(const std::string & src, bool escapejis, bool escapeforxml, const std::locale & loc)
    {
        std::string out;
        out.reserve(src.size() * 2);
        if(escapeforxml)
            HandleCharacters<true>(src, out, escapejis, loc);
        else
            HandleCharacters<false>(src, out, escapejis, loc);
        out.shrink_to_fit();
        return std::move(out);
    }



//======================================================================================
//  Escaped Character Parsing
//======================================================================================
    /*
        FindCommonEscapeChar
            Lookup in the table for common escape sequences for a single char!
    */
    inline bool FindCommonEscapeChar(const string & str, std::stringstream & out)
    {
        auto itcom = CommonEscapeCharactersToRaw.find(str);
        if( itcom != CommonEscapeCharactersToRaw.end() )
        {
            out<<itcom->second; //Append the character directly then
            return true;
        }
        return false;
    }


    /*
        MatchCommonEscapeChar
            Check if at least the beginning of a string of arbitrary length matches
            a common escape character. Any excess characters will be appended
            to the "out" string stream after the escaped value only if it matched!
    */
    inline bool MatchCommonEscapeChar( const string & str, std::stringstream & out )
    {
        if( str.size() == 2 )
            return FindCommonEscapeChar(str, out);
        else if( str.size() > 2 && FindCommonEscapeChar(str.substr(0,2), out) )
        {
            out << str.substr(2);
            return true;
        }
        return false;
    }

    /*
    */
    inline bool MatchHexEscapeChar( const string & escexpr, std::stringstream & out )
    {
        static const regex MatchHexByteRegex     ("([0-9a-fA-F]{2})"); //Hex byte specifier
        static const regex MatchUnicodeByteRegex ("([0-9a-fA-F]{4})"); //Unicode specifier
        size_t foundx = string::npos;

        if( (foundx = escexpr.find_first_of('x')) != string::npos && (foundx+1) < escexpr.size() )
        {
            smatch hexmatch;
            string digits( std::move(escexpr.substr(foundx+1)) ); //Need to make a string here, or regex_match freaks out
            if( regex_match( digits, hexmatch, MatchHexByteRegex ) )
            {
                stringstream sstr;
                uint16_t val = 0;
                sstr <<hex <<"0x" <<hexmatch.str(); //Get the digits only
                sstr >>val;
                out <<static_cast<char>(val);
                return true;
            }
            else
            {
                throw exMalformedEscapedCharacterSequence("MatchHexEscapeChar(): The sequence for a hex byte didn't match the expected format of \\xhh!");
            }
        }
        else if( (foundx = escexpr.find_first_of('U')) != string::npos && (foundx+1) < escexpr.size() )
        {
            smatch unicodematch;
            string digits( std::move(escexpr.substr(foundx+1)) ); //Need to make a string here, or regex_match freaks out
            if( regex_match( digits, unicodematch, MatchUnicodeByteRegex ) )
            {
                stringstream sstr;
                uint16_t val = 0;
                sstr <<hex <<"0x" <<unicodematch.str(); //Get the digits only
                sstr >>val;
                out <<static_cast<char>(val);       //Lowest byte first
                out <<static_cast<char>(val>>8);    //Highest last, since the game seems to use something close to UTF1?
                return true;
            }
            else
            {
                throw exMalformedEscapedCharacterSequence("MatchHexEscapeChar(): The sequence for a hex byte didn't match the expected format of \\Uhhhh!");
            }
        }
        return false;
    }


    /*
    */
    std::string ReplaceEscapedCharacters(const std::string & src, const std::locale & loc )
    {
        static const regex   MatchEscCharRegex ("(\\\\(.)([0-9a-fA-F]{2})?)");  //This matches all escaped characters, including the leading backslash
        std::sregex_iterator ithex(src.begin(), src.end(), MatchEscCharRegex);
        std::sregex_iterator end;
        if( ithex == end )
            return src;
        
        try
        {
            stringstream deststr;
            deststr.imbue(loc);

            while (ithex != end) 
            {
                std::smatch match = *ithex;
                if( match.size() > 1 )
                {
                    //AppendPrefix
                    deststr<<match.prefix();
                    //Find if we match a common escape char
                    const string curmatch = match.str();
                    if( !MatchCommonEscapeChar(curmatch, deststr) )
                    {
                        if( !MatchHexEscapeChar(curmatch, deststr) ) //Find if it matches a hex byte
                        {
                            //!#TODO: CRASHED HERE BECAUSE OF "\nbe" !!!
                            throw std::runtime_error("ReplaceEscapedUnprintableCharacters(): Encountered unknown escape sequence \""s + curmatch +"\" !!");
                        }
                    }
                }
                ithex++;

                //On the last match, append the suffix!
                if(ithex == end && match.size() > 1 ) 
                    deststr <<match.suffix();
            } 

            return std::move(deststr.str());
        }
        catch( const exMalformedEscapedCharacterSequence & e )
        {
            stringstream ss;
            ss <<" Caught while parsing string \"" <<src <<"\".";
            std::throw_with_nested( std::runtime_error(ss.str()) );
        }
    }

};