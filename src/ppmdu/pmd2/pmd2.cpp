#include "pmd2.hpp"
#include <sstream>
#include <iomanip>
#include <utils/poco_wrapper.hpp>
#include <iostream>
using namespace std;

namespace pmd2
{
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

};