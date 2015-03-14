#include "pokemon_stats.hpp"
#include <ppmdu/utils/utility.hpp>
#include <ppmdu/utils/poco_wrapper.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>
using namespace std;

namespace pmd2{ namespace stats
{
    static const std::string PKMN_NamesFile     = "pkmn_names.txt";
    static const std::string PKMN_AbilitiesFile = "pkmn_abilities.txt";
    static const std::string PKMN_IQGrpsFile    = "pkmn_iq.txt";
    static const std::string PKMN_TypesFile     = "pkmn_types.txt";
    static const std::string PKMN_MovesFile     = "pkmn_moves.txt";
    static const std::string ItemsFile          = "items.txt";

//
//
//
    CStatsResLoader & CStatsResLoader::GetInstance()
    {
        static CStatsResLoader s_instance;
        return s_instance;
    }

    CStatsResLoader::CStatsResLoader()
    {
        Parse( utils::getCWD() );
    }

    void CStatsResLoader::Parse( const std::string & pathDataDir )
    {
        ParseData(pathDataDir);
    }

    //Parsing
    void CStatsResLoader::ParseData( std::string pathDataDir )
    {
        try
        {
            pathDataDir = utils::AppendTraillingSlashIfNotThere( pathDataDir );
            
            clog<<"Parsing " <<PKMN_NamesFile <<"...\n";
            ParsePkmnNames(pathDataDir + PKMN_NamesFile);
            
            clog<<"Parsing " <<PKMN_TypesFile <<"...\n";
            ParsePkmnTypes(pathDataDir + PKMN_TypesFile);
            
            clog<<"Parsing " <<PKMN_IQGrpsFile <<"...\n";
            ParseIQGrps   (pathDataDir + PKMN_IQGrpsFile);
            
            clog<<"Parsing " <<PKMN_AbilitiesFile <<"...\n";
            ParseAbilities(pathDataDir + PKMN_AbilitiesFile);
            
            clog<<"Parsing " <<PKMN_MovesFile <<"...\n";
            ParseMoves    (pathDataDir + PKMN_MovesFile);
            
            //clog<<"Parsing " <<ItemsFile <<"...\n";
            //ParseItems    (pathDataDir + ItemsFile );
            clog<<"All resources loaded!\n";
        }
        catch( exception & e )
        {
            stringstream sstr;
            sstr<<"ERROR: Unable to parse the pkmn_*.txt string files! " <<e.what();
            string errorstr = sstr.str();
            clog<<errorstr<<"\n";
            throw std::runtime_error(errorstr);
        }
    }

    void CStatsResLoader::ParsePkmnNames( const std::string & pkmnNamesPath )
    {
        m_pkmnnames = utils::io::ReadTextFileLineByLine( pkmnNamesPath );
    }

    void CStatsResLoader::ParsePkmnTypes( const std::string & pkmnTypesPath )
    {
        m_pkmnTypes = utils::io::ReadTextFileLineByLine( pkmnTypesPath );
    }
    
    void CStatsResLoader::ParseIQGrps   ( const std::string & pkmnIQPath )
    {
        m_iqgrps = utils::io::ReadTextFileLineByLine( pkmnIQPath );
    }

    void CStatsResLoader::ParseAbilities( const std::string & pkmnAbilitiesPath )
    {
        m_abilities = utils::io::ReadTextFileLineByLine( pkmnAbilitiesPath );
    }

    void CStatsResLoader::ParseMoves    ( const std::string & pkmnMovesPath )
    {
        m_moves = utils::io::ReadTextFileLineByLine( pkmnMovesPath );
    }

    void CStatsResLoader::ParseItems    ( const std::string & ItemsPath )
    {
        m_items = utils::io::ReadTextFileLineByLine( ItemsPath );
    }

};};