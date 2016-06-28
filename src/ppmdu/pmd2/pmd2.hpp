#ifndef PMD2_HPP
#define PMD2_HPP
/*
pmd2.hpp
2015/09/11
psycommando@gmail.com
Description:
    Contains declarations and tools that applies to the entirety of the PMD2 games.
*/
#include <cstdint>
#include <array>
#include <string>

namespace pmd2
{
//======================================================================================
//  Constants
//======================================================================================
    /*
        eGameVersion
            Unique IDs for each versions of the PMD2 games.
    */
    enum struct eGameVersion
    {
        Invalid,
        EoS,        //Explorers of Sky
        EoT,        //Explorers of Time
        EoD,        //Explorers of Darkness
        //EoTEoD,     //Both/either Explorers of Time/Darkness
        NBGameVers, //Must be last
    };

    /*
        eGameRegion
            Locale of the game. Used mainly for determining offsets differences between versions.
    */
    enum struct eGameRegion
    {
        Invalid,
        Japan,
        NorthAmerica,
        Europe,
        NBRegions, //Must be last
    };

    /*
        eGameLanguages
            List of the different possible languages.
            Each language's value is the letter suffix for its text_*.str file name.
    */
    enum struct eGameLanguages
    {
        invalid = 0,

        english,
        japanese,
        french ,
        german ,
        italian,
        spanish,

        NbLang,
    };


    /*
        Event/Resource Names
    */
    const std::string ResourcePrefix_A = "a";
    const std::string ResourcePrefix_B = "b";
    const std::string ResourcePrefix_D = "d";   //Dungeon
    const std::string ResourcePrefix_G = "g";   //Guild
    const std::string ResourcePrefix_H = "h";   //Home?
    const std::string ResourcePrefix_M = "m";
    const std::string ResourcePrefix_N = "n";
    const std::string ResourcePrefix_P = "p";   //Part?
    const std::string ResourcePrefix_S = "s";
    const std::string ResourcePrefix_T = "t";   //Town
    const std::string ResourcePrefix_V = "v";   //Visual?

    /*
        List of directory names from the PMD2 games.
    */
    const std::string DirName_DefData     = "data";
    const std::string DirName_DefOverlay  = "overlay";

    const std::string DirName_BACK        = "BACK";
    const std::string DirName_BALANCE     = "BALANCE";
    const std::string DirName_DUNGEON     = "DUNGEON";
    const std::string DirName_EFFECT      = "EFFECT";
    const std::string DirName_FONT        = "FONT";
    const std::string DirName_GROUND      = "GROUND";
    const std::string DirName_MAP_BG      = "MAP_BG";
    const std::string DirName_MESSAGE     = "MESSAGE";
    const std::string DirName_MONSTER     = "MONSTER";
    const std::string DirName_RESCUE      = "RESCUE";   //EoS Only
    const std::string DirName_SCRIPT      = "SCRIPT";
    const std::string DirName_SOUND       = "SOUND";
    const std::string DirName_SYNTH       = "SYNTH";    //EoS Only
    const std::string DirName_SYSTEM      = "SYSTEM";
    const std::string DirName_TABLEDAT    = "TABLEDAT"; //EoS Only
    const std::string DirName_TOP         = "TOP";

    /*
        List of some unique filenames from PMD2 games.
    */
    const std::string FName_MonsterMND   = "monster.mnd";

    /*
        Language file names
            text_e.str for example

        !#FIXME: Its probably  not a good idea to have those static in here, when we can query the text_*.str filename from the xml file
                 instead.
    */
    //!#FIXME: Should use GameLangLoader instead!
    const std::string FName_TextPref      = "text"; 
    const std::string FName_TextEngSufx   = "e";
    const std::string FName_TextJapSufx   = "j";
    const std::string FName_TextFreSufx   = "f";
    const std::string FName_TextGerSufx   = "g";
    const std::string FName_TextItaSufx   = "i";
    const std::string FName_TextSpaSufx   = "s";

    //!#FIXME: Should use GameLangLoader instead!
    const std::array<std::string, static_cast<size_t>(eGameLanguages::NbLang)> GameLanguagesSuffixes=
    {{
        FName_TextEngSufx,
        FName_TextJapSufx,
        FName_TextFreSufx,
        FName_TextGerSufx,
        FName_TextItaSufx,
        FName_TextSpaSufx,
    }};

    //
    const std::array<std::string, static_cast<size_t>(eGameLanguages::NbLang)> GameLanguagesNames=
    {{
        "English",
        "Japanese",
        "French",
        "German",
        "Italian",
        "Spanish",
    }};

    inline eGameLanguages StrToGameLang( const std::string & lang )
    {
        for( uint32_t cntlang = 0; cntlang < GameLanguagesNames.size(); ++cntlang )
            if( lang == GameLanguagesNames[cntlang] ) return static_cast<eGameLanguages>(cntlang+1);
        return eGameLanguages::invalid;
    }

    inline const std::string & GetGameLangName( eGameLanguages lang )
    {
        if( lang == eGameLanguages::invalid )
            return "Invalid";
        return GameLanguagesNames[ static_cast<size_t>(lang)-1 ];
    } 

//
//
//
    /*
        DetermineGameVersionAndLocale
            Returns both the locale and the version of PMD2 that the target ROM filesystem is from.
    */
    std::pair<eGameVersion,eGameRegion> DetermineGameVersionAndLocale( const std::string & pathfilesysroot );



    /*
        AnalyzeDirForPMD2Dirs
            Determine the version of PMD2 from looking at the ROM filesystem directories structure.
    */
    eGameVersion                        AnalyzeDirForPMD2Dirs      ( const std::string & pathdir );


    /*
        GameVersionNames
            For a given eGameVersion ID returns a string that represents the game version.
    */
    extern const std::array<std::string, static_cast<size_t>(eGameVersion::NBGameVers)> GameVersionNames;

    const std::string & GetGameVersionName( eGameVersion gv )
    {
        if( gv < eGameVersion::NBGameVers )
            return GameVersionNames[static_cast<size_t>(gv)];
        else
            throw std::out_of_range("GetGameVersionName(): Invalid GameVersion!");
    }

    inline eGameVersion StrToGameVersion( const std::string & strvers )
    {
        for( size_t i = 0; i < GameVersionNames.size(); ++i )
        {
            if( strvers == GameVersionNames[i] )
                return static_cast<eGameVersion>(i);
        }
        return eGameVersion::Invalid;
    }

    /*
        GameRegionNames
            
    */
    extern const std::array<std::string,static_cast<size_t>(eGameRegion::NBRegions)> GameRegionNames;

    inline const std::string & GetGameRegionNames( eGameRegion gr )
    {
        if( gr < eGameRegion::NBRegions )
            return GameRegionNames[static_cast<size_t>(gr)];
        else
            throw std::out_of_range("GetGameRegionNames(): Invalid GameRegion!");
    }

    inline eGameRegion StrToGameRegion( const std::string & strregion )
    {
        for( size_t i = 0; i < GameRegionNames.size(); ++i )
        {
            if( strregion == GameRegionNames[i] )
                return static_cast<eGameRegion>(i);
        }
        return eGameRegion::Invalid;
    }

};

#endif