/*
    Handles exporting/importing the pokemon stats to/from XML.
*/

#include "pokemon_stats.hpp"
#include <utils/parse_utils.hpp>
#include <utils/pugixml_utils.hpp>
#include <pugixml.hpp>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <memory>
#include <functional>
#include <Poco/DirectoryIterator.h>
#include <Poco/File.h>
#include <Poco/Path.h>
using namespace std;
using namespace pugi;
using namespace pugixmlutils;

namespace pmd2 {namespace stats 
{
//===============================================================================================
//  Constants
//===============================================================================================
    namespace pkmnXML
    {
        static const string ROOT_Pkmn     = "Pokemon";
        static const string ATTR_GameVer  = "GameVersion";

        static const string NODE_GenderEnt= "GenderedEntity";

        //Strings
        static const string NODE_Strings  = "Strings";
        static const string PROP_Name     = "Name";
        static const string PROP_Category = "Category";

        //Stats growth
        static const string NODE_SGrowth  = "StatsGrowth";
        static const string NODE_Level    = "Level";
        static const string PROP_ExpReq   = "RequiredExp";
        static const string PROP_HP       = "HP";
        static const string PROP_Atk      = "Attack";
        static const string PROP_SpAtk    = "SpAttack";
        static const string PROP_Def      = "Defense";
        static const string PROP_SpDef    = "SpDefense";

        //Moveset
        static const string NODE_Moveset  = "Moveset";
        static const string NODE_LvlUpMv  = "LevelUpMoves";
        static const string NODE_EggMv    = "EggMoves";
        static const string NODE_HMTMMv   = "HmTmMoves";
        static const string PROP_MoveID   = "MoveID";
        static const string PROP_Learn    = "Learn";
        static const string &PROP_Level   = NODE_Level;

        //Stats
        static const string PROP_PokeID   = "PokeID";
        static const string PROP_Unk31    = "Unk31";
        static const string PROP_DexNb    = "PokedexNumber";
        static const string PROP_Unk1     = "Unk1";

        static const string NODE_EvoReq   = "EvolutionReq";
        static const string PROP_PreEvo   = "PreEvoIndex";
        static const string PROP_EvoMeth  = "Method";
        static const string PROP_EvoParam1= "Param1";
        static const string PROP_EvoParam2= "Param2";

        static const string PROP_SprID    = "SpriteIndex";
        static const string PROP_Gender   = "Gender";
        static const string PROP_BodySz   = "BodySize";
        static const string PROP_PrimaryTy= "PrimaryType";
        static const string PROP_SecTy    = "SecondaryType";
        static const string PROP_MvTy     = "MovementType";
        static const string PROP_IQGrp    = "IQGroup";
        static const string PROP_Ability1 = "PrimaryAbility";
        static const string PROP_Ability2 = "SecondaryAbility";
        static const string PROP_BitField = "Bitfield";
        static const string PROP_ExpYield = "ExpYield";
        static const string PROP_RecRate1 = "RecruitRate1";
        static const string PROP_RecRate2 = "RecruitRate2";

        static const string NODE_BaseStats= "BaseStats";

        static const string PROP_Weight   = "Weight";
        static const string PROP_Size     = "Size";
        static const string PROP_Unk17    = "Unk17";
        static const string PROP_Unk18    = "Unk18";
        static const string PROP_Unk19    = "Unk19";
        static const string PROP_Unk20    = "Unk20";
        static const string PROP_Unk21    = "Unk21";
        static const string PROP_BPkmnInd = "BasePokemonIndex";

        static const string NODE_ExItems  = "ExclusiveItems";
        static const string PROP_ExItemID = "ItemID";

        static const string PROP_Unk27    = "Unk27";
        static const string PROP_Unk28    = "Unk28";
        static const string PROP_Unk29    = "Unk29";
        static const string PROP_Unk30    = "Unk30";
    };

    //#TODO: Constants with the same values are spread out around the code! Do something about that!
    static const string GameVersion_EoS  = "EoS";
    static const string GameVersion_EoTD = "EoT/D";

//===============================================================================================
//  Classes
//===============================================================================================

    /***************************************************************************************
        PokemonDB_XMLWriter
            Writes a pokemon DB objects to XML. This is a single-use object/state.
    ***************************************************************************************/
    class PokemonDB_XMLWriter
    {
    public:
        PokemonDB_XMLWriter( const PokemonDB                         & src, 
                             const GameText                          * gtext )
            :m_src(src),m_pgametext(gtext), m_bNoStrings(false)
        {}

        void Write( const std::string & outdir )
        {
            if( !m_pgametext || (m_pgametext && !m_pgametext->AreStringsLoaded()) )
            {
                m_bNoStrings = true;
            }

            WriteAllEntries(outdir);
        }

    private:

        //Returns a pointer to the buffer passed as argument
        inline const char * FastTurnIntToHexCStr( unsigned int value )
        {
            sprintf_s( m_convBuff.data(), CBuffSZ, "0x%s", itoa( value, m_secConvbuffer.data(), 16 ) );
            return m_convBuff.data();
        }

        inline const char * FastTurnIntToCStr( uint32_t value )
        {
            //stringstream sstr;
            //sstr <<dec << value;
            //strcpy_s( m_convBuff.data(), m_convBuff.size(), sstr.str().c_str() );
            //return m_convBuff.data();
            return itoa( value, m_convBuff.data(), 10 );
        }

        inline const char * FastTurnIntToCStr( uint16_t value )
        {
            //stringstream sstr;
            //sstr <<dec << value;
            //strcpy_s( m_convBuff.data(), m_convBuff.size(), sstr.str().c_str() );
            //return m_convBuff.data();
            return itoa( value, m_convBuff.data(), 10 );
        }

        inline const char * FastTurnIntToCStr( uint8_t value )
        {
            //stringstream sstr;
            //sstr <<dec << value;
            //strcpy_s( m_convBuff.data(), m_convBuff.size(), sstr.str().c_str() );
            //return m_convBuff.data();
            return itoa( value, m_convBuff.data(), 10 );
        }


        inline string PreparePokeNameFName( const string & name, eGameLanguages lang )
        {
            const string * plocstr = m_pgametext->GetLocaleString(lang);
            if( plocstr )
                return utils::CleanFilename( name.substr( 0, name.find("\\0",0 ) ), std::locale( *plocstr ) ); //Remove ending "\0" and remove illegal characters for filesystem
            else 
                return utils::CleanFilename( name.substr( 0, name.find("\\0",0 ) ) );
        }

        stringstream & MakeFilename( stringstream & out_fname, const string & outpathpre, unsigned int cntpkmn )
        {
            const string * pfstr = nullptr;
            if( !m_bNoStrings && (pfstr = m_pgametext->begin()->second.GetStringInBlock( eStrBNames::PkmnNames, cntpkmn )) )
            {
                //const string * pfstr = m_pgametext->begin()->second.GetStringInBlock( eStrBNames::PkmnNames, cntpkmn );
                //if( pfstr )
                    out_fname <<outpathpre <<setw(4) <<setfill('0') <<cntpkmn <<"_" <<PreparePokeNameFName(*pfstr, m_pgametext->begin()->first) <<".xml";
                //else
                //    out_fname <<outpathpre <<setw(4) <<setfill('0') <<cntpkmn <<".xml";
            }
            else
                out_fname <<outpathpre <<setw(4) <<setfill('0') <<cntpkmn <<".xml";

            return out_fname;
        }

        void WriteAllEntries( const string & outdir )
        {
            using namespace pkmnXML;
            stringstream sstrfname;
            const string outpathpre = utils::TryAppendSlash(outdir);

            for( unsigned int cntpkmn = 0; cntpkmn < m_src.size(); ++cntpkmn )
            {
                sstrfname.str(string());
                MakeFilename(sstrfname, outpathpre, cntpkmn);
                //if( !m_bNoStrings )
                //{
                //    const string * pfstr = m_pgametext->begin()->second.GetStringInBlock( eStrBNames::PkmnNames, cntpkmn );
                //    if( pfstr )
                //        sstrfname <<outpathpre <<setw(4) <<setfill('0') <<cntpkmn <<"_" <<PreparePokeNameFName(*pfstr, m_pgametext->begin()->first) <<".xml";
                //    else
                //        sstrfname <<outpathpre <<setw(4) <<setfill('0') <<cntpkmn <<".xml";
                //}
                //else
                //    sstrfname <<outpathpre <<setw(4) <<setfill('0') <<cntpkmn <<".xml";

                xml_document doc;
                xml_node     pknode = doc.append_child( ROOT_Pkmn.c_str() );

                if( m_src.isEoSData() )
                    AppendAttribute( pknode, ATTR_GameVer, GameVersion_EoS );
                else
                    AppendAttribute( pknode, ATTR_GameVer, GameVersion_EoTD );

                WriteAPokemon( m_src[cntpkmn], pknode, cntpkmn );

                if( ! doc.save_file( sstrfname.str().c_str() ) )
                    throw std::runtime_error("Can't write xml file " + sstrfname.str());
            }
        }

        void WriteAPokemon( const CPokemon & pkmn, xml_node & pknode, unsigned int pkindex )
        {
            using namespace pkmnXML;
            //Write strings block
            WriteCommentNode( pknode, "In-game text" );
            WriteStrings( pknode, pkindex );

            //Write Gender entity 1
            WriteCommentNode( pknode, "Primary gender entity" );
            xml_node genderent1 = pknode.append_child( NODE_GenderEnt.c_str() );
            WriteMonsterData( pkmn.MonsterDataGender1(), genderent1 );

            if( pkmn.Has2GenderEntries() )
            {
                //Write Gender entity 2
                WriteCommentNode( pknode, "Secondary gender entity" );
                xml_node genderent2 = pknode.append_child( NODE_GenderEnt.c_str() );
                WriteMonsterData( pkmn.MonsterDataGender2(), genderent2 );
            }

            //Write common data
            WriteCommentNode( pknode, "Moveset from waza_p.bin" );
            WriteMoveSet    ( pkmn.MoveSet1(),    pknode );

            if( m_src.isEoSData() )
            {
                WriteCommentNode( pknode, "Moveset from waza_p2.bin" );
                WriteMoveSet    ( pkmn.MoveSet2(),    pknode );
            }

            WriteCommentNode( pknode, "Stats growth and exp requirements" );
            WriteStatsGrowth( pkmn.StatsGrowth(), pknode );
        }

        void WriteStrings( xml_node & pn, unsigned int pkindex )
        {
            using namespace pkmnXML;
            xml_node strnode = pn.append_child( NODE_Strings.c_str() );

            //Add all loaded languages
            for( const auto & alang : *m_pgametext )
            {
                xml_node langnode = strnode.append_child( GetGameLangName(alang.first).c_str() );
                //Write Name
                const string * pname = alang.second.GetStringInBlock(eStrBNames::PkmnNames,pkindex);
                if( pname )
                    WriteNodeWithValue( langnode, PROP_Name, utils::StrRemoveAfter( *pname, "\\0" ) ); //remove ending \0
                //Write Category
                const string * pcat = alang.second.GetStringInBlock(eStrBNames::PkmnCats,pkindex);
                if( pcat )
                    WriteNodeWithValue( langnode, PROP_Category, utils::StrRemoveAfter( *pcat, "\\0" ) ); //remove ending \0
            }
        }

        void WriteMonsterData( const PokeMonsterData & md, xml_node & pn )
        {
            using namespace pkmnXML;
            WriteNodeWithValue( pn, PROP_PokeID, md.pokeID)     ;
            WriteNodeWithValue( pn, PROP_Unk31,  md.mdunk31)    ;
            WriteNodeWithValue( pn, PROP_DexNb,  md.natPkdexNb) ;
            WriteNodeWithValue( pn, PROP_Unk1,   md.mdunk1)     ;

            //Evolution data
            //{
                xml_node evorq = pn.append_child( NODE_EvoReq.c_str() );
                WriteNodeWithValue( evorq, PROP_PreEvo,    md.evoData.preEvoIndex) ;
                WriteNodeWithValue( evorq, PROP_EvoMeth,   md.evoData.evoMethod)   ;
                WriteNodeWithValue( evorq, PROP_EvoParam1, md.evoData.evoParam1)   ;
                WriteNodeWithValue( evorq, PROP_EvoParam2, md.evoData.evoParam2)   ;
            //}

            WriteNodeWithValue( pn, PROP_SprID,     md.spriteIndex) ;
            WriteNodeWithValue( pn, PROP_Gender,     md.gender)     ;
            WriteNodeWithValue( pn, PROP_BodySz,    md.bodySize)    ;
            WriteNodeWithValue( pn, PROP_PrimaryTy, md.primaryTy)   ;
            WriteNodeWithValue( pn, PROP_SecTy,     md.secondaryTy) ;
            WriteNodeWithValue( pn, PROP_MvTy,      md.moveTy)      ;
            WriteNodeWithValue( pn, PROP_IQGrp,     md.IQGrp)       ;
            WriteNodeWithValue( pn, PROP_Ability1,  md.primAbility) ;
            WriteNodeWithValue( pn, PROP_Ability2,  md.secAbility)  ;

            WriteNodeWithValue( pn, PROP_BitField,  FastTurnIntToHexCStr(md.bitflags1) );

            WriteNodeWithValue( pn, PROP_ExpYield,  md.expYield)    ;
            WriteNodeWithValue( pn, PROP_RecRate1,  md.recruitRate1);
            WriteNodeWithValue( pn, PROP_RecRate2,  md.recruitRate2);

            //Base stats data
            //{
                xml_node bstats = pn.append_child( NODE_BaseStats.c_str() );
                WriteNodeWithValue( bstats, PROP_HP,    md.baseHP)    ;
                WriteNodeWithValue( bstats, PROP_Atk,   md.baseAtk)   ;
                WriteNodeWithValue( bstats, PROP_SpAtk, md.baseSpAtk) ;
                WriteNodeWithValue( bstats, PROP_Def,   md.baseDef)   ;
                WriteNodeWithValue( bstats, PROP_SpDef, md.baseSpDef) ;
            //}

            WriteNodeWithValue( pn, PROP_Weight,   md.weight)  ;
            WriteNodeWithValue( pn, PROP_Size,     md.size)    ;
            WriteNodeWithValue( pn, PROP_Unk17,    FastTurnIntToHexCStr(md.mdunk17) );
            WriteNodeWithValue( pn, PROP_Unk18,    FastTurnIntToHexCStr(md.mdunk18) );
            WriteNodeWithValue( pn, PROP_Unk19,    FastTurnIntToHexCStr(md.mdunk19) );
            WriteNodeWithValue( pn, PROP_Unk20,    FastTurnIntToHexCStr(md.mdunk20) );
            WriteNodeWithValue( pn, PROP_Unk21,    FastTurnIntToHexCStr(md.mdunk21) );

            WriteNodeWithValue( pn, PROP_BPkmnInd, md.BasePkmn) ;

            //Exclusive items data
            //{
                xml_node exclusive = pn.append_child( NODE_ExItems.c_str() );
                for( const auto & exitem : md.exclusiveItems )
                    WriteNodeWithValue( exclusive, PROP_ExItemID, exitem) ;
            //}

            WriteNodeWithValue( pn, PROP_Unk27, FastTurnIntToHexCStr(md.unk27) );
            WriteNodeWithValue( pn, PROP_Unk28, FastTurnIntToHexCStr(md.unk28) );
            WriteNodeWithValue( pn, PROP_Unk29, FastTurnIntToHexCStr(md.unk29) );
            WriteNodeWithValue( pn, PROP_Unk30, FastTurnIntToHexCStr(md.unk30) );
        }

        void WriteStatsGrowth( const PokeStatsGrowth & sg, xml_node & pn )
        {
            using namespace pkmnXML;
            xml_node growthnode = pn.append_child( NODE_SGrowth.c_str() );

            //Write every levels
            for( unsigned int i = 0; i < sg.size(); ++i )
            {
                //Write comment indicating the level #
                array<char,32> buflvl = {0};
                sprintf_s( buflvl.data(), buflvl.size(), "Level %i", (i+1) );
                WriteCommentNode( growthnode, buflvl.data() );

                //Write lvl-up data
                xml_node lvlnode = growthnode.append_child( NODE_Level.c_str() );
                //Exp Required
                WriteNodeWithValue( lvlnode, PROP_ExpReq, FastTurnIntToCStr( sg[i].first) );

                //Stats
                WriteNodeWithValue( lvlnode, PROP_HP,     FastTurnIntToCStr(sg[i].second.HP) );
                WriteNodeWithValue( lvlnode, PROP_Atk,    FastTurnIntToCStr(sg[i].second.Atk) );
                WriteNodeWithValue( lvlnode, PROP_SpAtk,  FastTurnIntToCStr(sg[i].second.SpA) );
                WriteNodeWithValue( lvlnode, PROP_Def,    FastTurnIntToCStr(sg[i].second.Def) );
                WriteNodeWithValue( lvlnode, PROP_SpDef,  FastTurnIntToCStr(sg[i].second.SpD) );
            }
        }

        void WriteMoveSet( const PokeMoveSet & mv, xml_node & pn )
        {
            using namespace pkmnXML;
            xml_node mvsetnode = pn.append_child( NODE_Moveset.c_str() );
            array<char,32> commentbuf = {0};

            sprintf_s( commentbuf.data(), commentbuf.size(), "Learns %i move(s)", mv.lvlUpMoveSet.size() );
            WriteCommentNode( mvsetnode, commentbuf.data() );

            //Level-up moves
            xml_node lvlupnode = mvsetnode.append_child( NODE_LvlUpMv.c_str() );
            for( const auto & lvlupmv : mv.lvlUpMoveSet )
            {
                xml_node learnnode = lvlupnode.append_child( PROP_Learn.c_str() );
                WriteNodeWithValue( learnnode, PROP_Level,  FastTurnIntToCStr(lvlupmv.first)  );
                WriteNodeWithValue( learnnode, PROP_MoveID, FastTurnIntToCStr(lvlupmv.second) );
            }

            sprintf_s( commentbuf.data(), commentbuf.size(), "Has %i egg move(s)", mv.eggmoves.size() );
            WriteCommentNode( mvsetnode, commentbuf.data() );

            //Egg Moves
            xml_node eggnode = mvsetnode.append_child( NODE_EggMv.c_str() );
            for( const auto & eggmv : mv.eggmoves )
                WriteNodeWithValue( eggnode, PROP_MoveID, FastTurnIntToCStr(eggmv) );

            sprintf_s( commentbuf.data(), commentbuf.size(), "Can learn %i HM/TM move(s)", mv.teachableHMTMs.size() );
            WriteCommentNode( mvsetnode, commentbuf.data() );

            //HM/TM moves
            xml_node tmnode = mvsetnode.append_child( NODE_HMTMMv.c_str() );
            for( const auto & tmmv : mv.teachableHMTMs )
                WriteNodeWithValue( tmnode, PROP_MoveID, FastTurnIntToCStr(tmmv) );
        }


    private:
        //Length of the conversion buffer
        static const int                          CBuffSZ = (sizeof(int)*8+1);

        const PokemonDB                         & m_src;
        const GameText                          * m_pgametext;

        //std::vector<std::string>::const_iterator m_itbegnames;
        //std::vector<std::string>::const_iterator m_itbegcat;

        //Conversion buffers. Used for faster value conversion. (Don't need all the extra locale stuff from stringstream, as all values are raw data)
        array<char,CBuffSZ>                      m_convBuff;
        array<char,CBuffSZ>                      m_secConvbuffer;

        bool                                     m_bNoStrings; //If true, omit strings entirely, and don't write pokemon names for each files!
    };

    /***************************************************************************************
        PokemonDB_XMLParser
            Read a pokemon DB objects from XML. This is a single-use object/state.
    ***************************************************************************************/
    class PokemonDB_XMLParser
    {
        //Lets hide that ginormous type plz
        typedef std::pair<std::vector<std::string>::iterator,std::vector<std::string>::iterator> strbounds_t;
    public:

        /*
        PokemonDB_XMLParser
            - out_pkdb   : The destination PokemonDB object
            - itbegnames : The beginning iterator of the game's pokemon name strings.
            - itendnames : The end iterator of the game's pokemon name strings.
            - itbegcat   : The beginning iterator of the game's pokemon category strings.
            - itendcat   : The end iterator of the game's pokemon category strings.
        */
        //PokemonDB_XMLParser( PokemonDB                          & out_pkdb,
        //                     std::vector<std::string>::iterator   itbegnames,
        //                     std::vector<std::string>::iterator   itendnames,
        //                     std::vector<std::string>::iterator   itbegcat,
        //                     std::vector<std::string>::iterator   itendcat )
        //   :m_out(out_pkdb),m_boundsNames(make_pair(itbegnames,itendnames)), m_boundsCats(make_pair(itbegcat,itendcat)), m_isEoS(false)
        //{}

        /*
        */
        PokemonDB_XMLParser( PokemonDB & out_pkdb, GameText  * inout_gtext )
            :m_out(out_pkdb), m_isEoS(false), m_pgametext(inout_gtext), m_bParsePokemonId(false)
        {
        }
        

        void Parse( const std::string & srcdir, bool bparsepkid = true )
        {
            if( !m_pgametext || (m_pgametext && !m_pgametext->AreStringsLoaded()) )
            {
                m_bNoStrings = true;
                clog << "<!>- PokemonDB_XMLParser::Parse(): No strings present, ignoring completely strings(names, text, etc..)!\n";
            }
            m_bParsePokemonId = bparsepkid;

            try
            {
                m_out.Pkmn() = std::move( ReadAllPokemon(srcdir) );
                m_out.isEoSData(m_isEoS);
            }
            catch( exception & e )
            {
                stringstream sstr;
                sstr <<"Got Exception while parsing XML from directory \"" <<srcdir <<"\" : " << e.what();
                throw runtime_error( sstr.str() );
            }
        }

    private:

        /*
            Decides whether to parse the pokemon id from the filename or just use the file counter value instead.
        */
        uint32_t GetCurrentPokemonID( const std::string & pkmn, uint32_t cntPkmn)
        {
            uint32_t pkid = 0;
            if( m_bParsePokemonId )
            {
                stringstream sstrmoveid;
                auto         itendnumber = std::find_if_not( pkmn.begin(), 
                                                             pkmn.end(), 
                                                            std::bind( &(std::isalnum<char>), 
                                                                        std::placeholders::_1, 
                                                                        std::ref(locale::classic()) ) );
                sstrmoveid << string( pkmn.begin(), itendnumber );
                sstrmoveid >> pkid;
            }
            else
                pkid = cntPkmn;
            return pkid;
        }

        vector<CPokemon> ReadAllPokemon( const string & srcdir )
        {
            using namespace pkmnXML;
            Poco::DirectoryIterator itDirEnd;
            vector<string>          filelst;

            for( Poco::DirectoryIterator itDir(srcdir); itDir != itDirEnd; ++itDir )
            {
                if( itDir->isFile() && Poco::Path(itDir.path()).getExtension() == "xml" )
                    filelst.push_back( (itDir.path().absolute().toString()) );
            }

            if( filelst.empty() )
                throw std::runtime_error( "PokemonDB_XMLParser::ReadAllPokemon(): Couldn't find any xml files under the path \"" + srcdir + "\"!" );

            vector<CPokemon> result;
            result.reserve(filelst.size());

            uint32_t cntEoSPk = 0;
            uint32_t cntPkmn  = 0;
            for( auto & pkmn : filelst )
            {
                uint32_t         pkid = GetCurrentPokemonID(pkmn,cntPkmn);
                xml_document     doc;
                xml_parse_result loadres = doc.load_file(pkmn.c_str());
                if( ! loadres )
                {
                    stringstream sstr;
                    sstr <<"Can't load XML document \"" <<pkmn <<"\"! Pugixml returned an error : \"" << loadres.description() <<"\"";
                    throw std::runtime_error(sstr.str());
                }

                bool     hadEoSAttribute = false;
                xml_node rootnode        = doc.child(ROOT_Pkmn.c_str());
                if( rootnode )
                    result.push_back( ReadPokemon( rootnode, cntPkmn, hadEoSAttribute ) );
                else
                {
                    clog << "<!>- PokemonDB_XMLParser::ReadAllPokemon(): Skipping XML file with no pokemon data..\n";
                    continue;
                }

                //Count EoS pokes
                if( hadEoSAttribute )
                    ++cntEoSPk;

                ++cntPkmn;
            }

            //Determine if is EoS data by looking at the total nb of EoS-like entries!
            if( cntEoSPk > 0 && cntEoSPk < (filelst.size()-1) ) //Account for always null first entry
            {
                cerr << "WARNING: One or more Pokemon's " <<ATTR_GameVer <<" attribute doesn't match the others!\n"
                     << "Got " <<dec <<cntEoSPk <<" entries that had Explorers of Sky as their game version, while the rest didn't!\n"
                     << "Attempting to continue.\n";
            }
            else
            {
                m_isEoS = cntEoSPk > 0;
            }
            return std::move(result);
        }

        ////Get pokemon name string reference
        //std::string & GetStringRefPkmn( uint32_t index )
        //{
        //    auto it = m_boundsNames.first + index;
        //    if( it < m_boundsNames.second )
        //        return *it;
        //    else
        //    {
        //        stringstream sstr;
        //        sstr << "ERROR: The name string for Pokemon at index " <<dec << index <<" is out of the valid Pokemon name string range!";
        //        throw std::out_of_range( sstr.str() );
        //    }
        //}

        ////Get category string reference
        //std::string & GetStringRefCat( uint32_t index )
        //{
        //    auto it = m_boundsCats.first + index;
        //    if( it < m_boundsCats.second )
        //        return *it;
        //    else
        //    {
        //        stringstream sstr;
        //        sstr << "ERROR: The category string for Pokemon at index " <<dec << index <<" is out of the valid Pokemon category string range!";
        //        throw std::out_of_range( sstr.str() );
        //    }
        //}

        CPokemon ReadPokemon( xml_node & pknode, uint32_t cntpokemon, bool & isEoS )
        {
            using namespace pkmnXML;
            //CPokemon curpoke;
            int curgenderEnt = 0;

            PokeStatsGrowth sg;
            PokeMoveSet     mvset1;
            PokeMoveSet     mvset2;
            PokeMonsterData gen1;
            PokeMonsterData gen2;
            bool            hasParsedMv1 = false;
            bool            has2gender   = false;

            //Check the game version of the pokemon data
            xml_attribute gv = pknode.attribute( ATTR_GameVer.c_str() );
            const string  gvs= gv.as_string(); 

            if( !gvs.empty() ) //If the attribute exists
            {
                //Easier to do it this way in order to handle the last case without running another string compare!
                if( gvs == GameVersion_EoS )
                    isEoS = true;
                else if( gvs == GameVersion_EoS )
                    isEoS = false;
                else
                {
                    stringstream sstr;
                    sstr << "Invalid game version attribute of \"" <<gvs <<"\" for Pokemon " <<cntpokemon <<"!";
                    throw runtime_error(sstr.str());
                } 
            }
            else
            {
                stringstream sstr;
                sstr << "Game version attribute for Pokemon " <<cntpokemon <<" is missing!";
                throw runtime_error(sstr.str());
            }

            for( auto & curnode : pknode.children() )
            {
                if( !m_bNoStrings && curnode.name() == NODE_Strings )
                {
                    ReadStrings(curnode,cntpokemon);
                }
                else if( curnode.name() == NODE_SGrowth )
                {
                    sg = ReadStatsGrowth( curnode );
                }
                else if( curnode.name() == NODE_Moveset )
                {
                    if( isEoS )
                    {
                        //Handle parsing the 2 movesets for EoS
                        if( !hasParsedMv1 )
                        {
                            mvset1 = ReadMoveSet( curnode );
                            hasParsedMv1 = true;
                        }
                        else
                        {
                            mvset2 = ReadMoveSet( curnode );
                            hasParsedMv1 = false;
                        }
                    }
                    else
                    {
                        mvset1 = ReadMoveSet( curnode );
                    }
                }
                else if( curnode.name() == NODE_GenderEnt )
                {
                    if( curgenderEnt == 0 )
                        gen1 = ReadGenderEnt( curnode );
                    else if( curgenderEnt == 1 )
                    {
                        gen2 = ReadGenderEnt( curnode );
                        has2gender = true;
                    }
                    ++curgenderEnt;
                }
            }

            if( has2gender )
                return CPokemon( move(gen1), move(gen2), move(sg), move(mvset1), move(mvset2) );
            else
                return CPokemon( move(gen1), move(sg), move(mvset1), move(mvset2) );
        }

        void ReadStrings( xml_node & strnode, uint32_t cntpkmn )
        {
            using namespace pkmnXML;
            for( auto & curnode : strnode.children() )
            {
                eGameLanguages glang = StrToGameLang(curnode.name());
                if( glang != eGameLanguages::invalid )
                {
                    //Parse multi-language strings
                    ReadLangStrings(curnode, glang, cntpkmn);
                }
                else
                {
                    //If the game isn't multi-lingual, just parse the strings for english
                    clog<<"<!>- PokemonDB_XMLParser::ReadStrings() : Found a non language named node!\n";
                    ReadLangStrings(curnode, eGameLanguages::english, cntpkmn);
                }
            }
        }

        void ReadLangStrings( xml_node & langnode, eGameLanguages lang, uint32_t cntpkmn )
        {
            StringAccessor * plangstr = m_pgametext->GetStrings(lang);
            if( !plangstr )
            {
                clog<<"<!>- PokemonDB_XMLParser::ReadLangStrings(): Found strings for " <<GetGameLangName(lang) <<", but the language was not loaded for editing! Skipping!\n";
                return;
            }

            using namespace pkmnXML;
            for( auto & curnode : langnode.children() )
            {
                if( curnode.name() == PROP_Name )
                {
                    string pkname = curnode.child_value();
                    pkname += "\\0"; //put back the \0
                    *(plangstr->GetStringInBlock( eStrBNames::PkmnNames, cntpkmn )) = pkname;
                }
                else if( curnode.name() == PROP_Category )
                {
                    string pkcat = curnode.child_value();
                    pkcat += "\\0"; //put back the \0
                    *(plangstr->GetStringInBlock( eStrBNames::PkmnCats, cntpkmn )) = pkcat;
                }
            }
        }

        PokeStatsGrowth ReadStatsGrowth( xml_node & sgrowthnode )
        {
            using namespace pkmnXML;
            PokeStatsGrowth result;

            for( auto & lvlnode : sgrowthnode.children( NODE_Level.c_str() ) )
            {
                PokeStatsGrowth::growthlvl_t entry;
                for( auto & curnode : lvlnode.children() )
                {
                    if( curnode.name() == PROP_ExpReq )
                        utils::parseHexaValToValue( curnode.child_value(), entry.first );
                    else if( curnode.name() == PROP_HP )
                        utils::parseHexaValToValue( curnode.child_value(), entry.second.HP );
                    else if( curnode.name() == PROP_Atk )
                        utils::parseHexaValToValue( curnode.child_value(), entry.second.Atk );
                    else if( curnode.name() == PROP_SpAtk )
                        utils::parseHexaValToValue( curnode.child_value(), entry.second.SpA );
                    else if( curnode.name() == PROP_Def )
                        utils::parseHexaValToValue( curnode.child_value(), entry.second.Def );
                    else if( curnode.name() == PROP_SpDef )
                        utils::parseHexaValToValue( curnode.child_value(), entry.second.SpD );
                }
                result.statsgrowth.push_back( std::move(entry) );
            }

            return std::move(result);
        }

        PokeMoveSet ReadMoveSet( xml_node & mvsetnode )
        {
            using namespace pkmnXML;
            PokeMoveSet result;

            for( auto & curnode : mvsetnode.children() )
            {
            //LvlUp moves
                if( curnode.name() == NODE_LvlUpMv )
                {
                    result.lvlUpMoveSet = ReadLvlUpMoves( curnode );
                }
            //Egg moves
                else if( curnode.name() == NODE_EggMv )
                {
                    for( auto & eggmv : curnode.children(PROP_MoveID.c_str()) )
                        result.eggmoves.push_back( utils::parseHexaValToValue<moveid_t>( eggmv.child_value() ) );
                }
            //TM moves
                else if( curnode.name() == NODE_HMTMMv )
                {
                    for( auto & tmmv : curnode.children(PROP_MoveID.c_str()) )
                        result.teachableHMTMs.push_back( utils::parseHexaValToValue<moveid_t>( tmmv.child_value() ) );
                }
            }
            return std::move(result);
        }

        PokeMoveSet::lvlUpMoveSet_t ReadLvlUpMoves( xml_node & lvlupnode )
        {
            using namespace pkmnXML;
            PokeMoveSet::lvlUpMoveSet_t result;

            for( auto & curnode : lvlupnode.children(PROP_Learn.c_str()) )
            {
                level_t  lvl  = 0;
                moveid_t mvid = 0;

                for( auto & learnnode : curnode.children() )
                {
                    if( learnnode.name() == PROP_Level )
                        utils::parseHexaValToValue( learnnode.child_value(), lvl );
                    else if( learnnode.name() == PROP_MoveID )
                        utils::parseHexaValToValue( learnnode.child_value(), mvid );
                }
                result.insert( make_pair( lvl, mvid ) );
            }

            return std::move( result );
        }

        PokeMonsterData ReadGenderEnt( xml_node & gendernode )
        {
            using namespace pkmnXML;
            PokeMonsterData result;

            for( auto & curnode : gendernode.children() )
            {
                if( curnode.name() == PROP_PokeID )
                    utils::parseHexaValToValue( curnode.child_value(), result.pokeID );
                else if( curnode.name() == PROP_Unk31 )
                    utils::parseHexaValToValue( curnode.child_value(), result.mdunk31 );
                else if( curnode.name() == PROP_DexNb )
                    utils::parseHexaValToValue( curnode.child_value(), result.natPkdexNb );
                else if( curnode.name() == PROP_Unk1 )
                    utils::parseHexaValToValue( curnode.child_value(), result.mdunk1 );
                else if( curnode.name() == NODE_EvoReq )
                {
                    for( auto & evoreqnode : curnode.children() )
                    {
                        if( evoreqnode.name() == PROP_PreEvo )
                            utils::parseHexaValToValue( evoreqnode.child_value(), result.evoData.preEvoIndex );
                        else if( evoreqnode.name() == PROP_EvoMeth )
                            utils::parseHexaValToValue( evoreqnode.child_value(), result.evoData.evoMethod );
                        else if( evoreqnode.name() == PROP_EvoParam1 )
                            utils::parseHexaValToValue( evoreqnode.child_value(), result.evoData.evoParam1 );
                        else if( evoreqnode.name() == PROP_EvoParam2 )
                            utils::parseHexaValToValue( evoreqnode.child_value(), result.evoData.evoParam2 );
                    }
                }
                else if( curnode.name() == PROP_SprID )
                    utils::parseHexaValToValue( curnode.child_value(), result.spriteIndex );
                else if( curnode.name() == PROP_Gender )
                    utils::parseHexaValToValue( curnode.child_value(), result.gender );
                else if( curnode.name() == PROP_BodySz )
                    utils::parseHexaValToValue( curnode.child_value(), result.bodySize );
                else if( curnode.name() == PROP_PrimaryTy )
                    utils::parseHexaValToValue( curnode.child_value(), result.primaryTy );
                else if( curnode.name() == PROP_SecTy )
                    utils::parseHexaValToValue( curnode.child_value(), result.secondaryTy );
                else if( curnode.name() == PROP_MvTy )
                    utils::parseHexaValToValue( curnode.child_value(), result.moveTy );
                else if( curnode.name() == PROP_IQGrp )
                    utils::parseHexaValToValue( curnode.child_value(), result.IQGrp );
                else if( curnode.name() == PROP_Ability1 )
                    utils::parseHexaValToValue( curnode.child_value(), result.primAbility );
                else if( curnode.name() == PROP_Ability2 )
                    utils::parseHexaValToValue( curnode.child_value(), result.secAbility );
                else if( curnode.name() == PROP_BitField )
                    utils::parseHexaValToValue( curnode.child_value(), result.bitflags1 );
                else if( curnode.name() == PROP_ExpYield )
                    utils::parseHexaValToValue( curnode.child_value(), result.expYield );
                else if( curnode.name() == PROP_RecRate1 )
                    utils::parseHexaValToValue( curnode.child_value(), result.recruitRate1 );
                else if( curnode.name() == PROP_RecRate2 )
                    utils::parseHexaValToValue( curnode.child_value(), result.recruitRate2 );
                else if( curnode.name() == NODE_BaseStats )
                {
                    for( auto & bstatnode : curnode.children() )
                    {
                        if( bstatnode.name() == PROP_HP )
                            utils::parseHexaValToValue( bstatnode.child_value(), result.baseHP );
                        else if( bstatnode.name() == PROP_Atk )
                            utils::parseHexaValToValue( bstatnode.child_value(), result.baseAtk );
                        else if( bstatnode.name() == PROP_SpAtk )
                            utils::parseHexaValToValue( bstatnode.child_value(), result.baseSpAtk );
                        else if( bstatnode.name() == PROP_Def )
                            utils::parseHexaValToValue( bstatnode.child_value(), result.baseDef );
                        else if( bstatnode.name() == PROP_SpDef )
                            utils::parseHexaValToValue( bstatnode.child_value(), result.baseSpDef );
                    }
                }
                else if( curnode.name() == PROP_Weight )
                    utils::parseHexaValToValue( curnode.child_value(), result.weight );
                else if( curnode.name() == PROP_Size )
                    utils::parseHexaValToValue( curnode.child_value(), result.size );
                else if( curnode.name() == PROP_Unk17 )
                    utils::parseHexaValToValue( curnode.child_value(), result.mdunk17 );
                else if( curnode.name() == PROP_Unk18 )
                    utils::parseHexaValToValue( curnode.child_value(), result.mdunk18 );
                else if( curnode.name() == PROP_Unk19 )
                    utils::parseHexaValToValue( curnode.child_value(), result.mdunk19 );
                else if( curnode.name() == PROP_Unk20 )
                    utils::parseHexaValToValue( curnode.child_value(), result.mdunk20 );
                else if( curnode.name() == PROP_Unk21 )
                    utils::parseHexaValToValue( curnode.child_value(), result.mdunk21 );
                else if( curnode.name() == PROP_BPkmnInd )
                    utils::parseHexaValToValue( curnode.child_value(), result.BasePkmn );
                else if( curnode.name() == NODE_ExItems )
                {
                    size_t insertat = 0;
                    for( auto & exitemnode : curnode.children(PROP_ExItemID.c_str()) )
                    {
                        result.exclusiveItems[insertat] = utils::parseHexaValToValue<uint16_t>( exitemnode.child_value() );
                        ++insertat;
                    }
                }
                else if( curnode.name() == PROP_Unk27 )
                    utils::parseHexaValToValue( curnode.child_value(), result.unk27 );
                else if( curnode.name() == PROP_Unk28 )
                    utils::parseHexaValToValue( curnode.child_value(), result.unk28 );
                else if( curnode.name() == PROP_Unk29 )
                    utils::parseHexaValToValue( curnode.child_value(), result.unk29 );
                else if( curnode.name() == PROP_Unk30 )
                    utils::parseHexaValToValue( curnode.child_value(), result.unk30 );
            }

            return std::move(result);
        }

    private:
        PokemonDB & m_out;
        GameText  * m_pgametext;
        //strbounds_t m_boundsNames;
        //strbounds_t m_boundsCats;
        bool        m_isEoS;
        bool        m_bNoStrings; //If true, omit strings entirely.
        bool        m_bParsePokemonId;
    };

//===============================================================================================
//  Functions
//===============================================================================================
    //void      ExportPokemonsToXML  ( const PokemonDB                         & src,
    //                                 std::vector<std::string>::const_iterator  itbegnames,
    //                                 std::vector<std::string>::const_iterator  itbegcat,
    //                                 const std::string                       & destDir )
    //{
    //    PokemonDB_XMLWriter( src, itbegnames, itbegcat ).Write(destDir);
    //}

    void      ExportPokemonsToXML  ( const PokemonDB                         & src,
                                     const GameText                          * gtext,
                                     const std::string                       & destdir )
    {
        PokemonDB_XMLWriter( src, gtext ).Write(destdir);
    }
    
    //void      ImportPokemonsFromXML( const std::string                  & srcdir, 
    //                                 PokemonDB                          & out_pkdb,
    //                                 std::vector<std::string>::iterator   itbegnames,
    //                                 std::vector<std::string>::iterator   itendnames,
    //                                 std::vector<std::string>::iterator   itbegcat,
    //                                 std::vector<std::string>::iterator   itendcat )
    //{
    //    PokemonDB_XMLParser( out_pkdb, itbegnames, itendnames, itbegcat, itendcat ).Parse(srcdir);
    //}


    void      ImportPokemonsFromXML ( const std::string                 & srcdir, 
                                      PokemonDB                         & out_pkdb,
                                      GameText                          * inout_gtext )
    {
        PokemonDB_XMLParser( out_pkdb, inout_gtext ).Parse(srcdir);
    }

};};