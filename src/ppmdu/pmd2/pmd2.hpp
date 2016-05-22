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
        EoTEoD,     //Explorers of Time/Darkness
        NBGameVers, //Must be last
    };

    /*
        eGameLocale
            Locale of the game. Used mainly for determining offsets differences between versions.
    */
    enum struct eGameLocale
    {
        Invalid,
        Japan,
        NorthAmerica,
        Europe,
        NBLocales, //Must be last
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
    */
    const std::string FName_TextPref      = "text"; 
    const std::string FName_TextEngSufx   = "e";
    const std::string FName_TextJapSufx   = "j";
    const std::string FName_TextFreSufx   = "f";
    const std::string FName_TextGerSufx   = "g";
    const std::string FName_TextItaSufx   = "i";
    const std::string FName_TextSpaSufx   = "s";

//
//
//
    std::pair<eGameVersion,eGameLocale> DetermineGameVersionAndLocale( const std::string & pathfilesysroot );



    eGameVersion                        AnalyzeDirForPMD2Dirs      ( const std::string & pathdir );


    /*
        GameVersionNames
            For a given eGameVersion ID returns a string that represents the game version.
    */
    extern const std::array<std::string, static_cast<size_t>(eGameVersion::NBGameVers)> GameVersionNames;

    const std::string & GetGameVersionName( eGameVersion gv );

};

#endif