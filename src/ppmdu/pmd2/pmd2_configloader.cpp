#include "pmd2_configloader.hpp"
#include <utils/pugixml_utils.hpp>
#include <utils/parse_utils.hpp>
#include <utils/library_wide.hpp>
#include <utils/poco_wrapper.hpp>
#include <iostream>
#include <functional>
#include <deque>
#include <regex>
using namespace std;
using namespace pugi;


namespace pmd2
{
    const std::array<std::string, static_cast<uint32_t>(eStringBlocks::NBEntries)> StringBlocksNames=
    {
        "Pokemon Names",
        "Pokemon Categories",
        "Move Names",
        "Move Descriptions",
        "Item Names",
        "Item Short Descriptions",
        "Item Long Descriptions",
        "Ability Names",
        "Ability Descriptions",
        "Type Names",
        "Portrait Names",
    };

    const std::array<std::string, static_cast<uint32_t>(eBinaryLocations::NbLocations)> BinaryLocationNames=
    {
        "Entities",           
        "Events",             
        "ScriptOpCodes",      
        "StartersHeroIds",    
        "StartersPartnerIds", 
        "StartersStrings",    
        "QuizzQuestionStrs",  
        "QuizzAnswerStrs",    
    };


    const std::array<std::string, static_cast<uint32_t>(eGameConstants::NbEntries)> GameConstantNames=
    {
        "NbPossibleHeros",
        "NbPossiblePartners",
        "NbUniqueEntities",
        "NbTotalEntities",

        //"Overlay11LoadAddress",
        //"Overlay13LoadAddress",
    };


    namespace ConfigXML
    {
        const string ROOT_PMD2      = "PMD2";

        const string NODE_GameEd    = "GameEditions";
        const string NODE_Game      = "Game";
        const string NODE_GConsts   = "GameConstants";
        const string NODE_Binaries  = "Binaries";
        const string NODE_Bin       = "Bin";
        const string NODE_Block     = "Block";
        const string NODE_StrIndex  = "StringIndexData";
        const string NODE_Language  = "Language";
        const string NODE_Languages = "Languages";
        const string NODE_StrBlk    = "StringBlock";
        const string NODE_StrBlks   = "StringBlocks";
        const string NODE_Value     = "Value";

        const string NODE_ScriptDat = "ScriptData";
        const string NODE_GVarTbl   = "GameVariablesTable";
        const string NODE_GVarExtTbl= "GameVariablesTableExtended";
        const string NODE_GVar      = "GameVar";
        const string NODE_LivesTbl  = "LivesEntityTable";
        const string NODE_Entity    = "Entity";
        const string NODE_LevelList = "LevelList";
        const string NODE_Level     = "Level";
        const string NODE_FaceNames = "FaceNames";
        const string NODE_Face      = "Face";
        const string NODE_FacePosMo = "FacePositionModes";
        const string NODE_Mode      = "Mode";
        const string NODE_CRoutineI = "CommonRoutineInfo";
        const string NODE_Routine   = "Routine";
        const string NODE_ObjectLst = "ObjectsList";
        const string NODE_Object    = "Object";

        const string NODE_ExtFile   = "External";   //For external files to load config from

        const string ATTR_ID        = "id";
        const string ATTR_ID2       = "id2";
        const string ATTR_ID3       = "id3";

        const string ATTR_GameCode  = "gamecode";
        const string ATTR_Version   = "version";
        const string ATTR_Version2  = "version2";
        const string ATTR_Region    = "region";
        const string ATTR_ARM9Off   = "arm9off14";
        const string ATTR_DefLang   = "defaultlang";
        const string ATTR_Support   = "issupported";
        const string ATTR_Val       = "value";
        const string ATTR_Name      = "name";
        const string ATTR_Beg       = "beg";
        const string ATTR_End       = "end";
        const string ATTR_Loc       = "locale";
        const string ATTR_FName     = "filename";
        const string ATTR_FPath     = "filepath";
        const string ATTR_LoadAddr  = "loadaddress";

        const string ATTR_Type      = "type";
        const string ATTR_Unk1      = "unk1";
        const string ATTR_Unk2      = "unk2";
        const string ATTR_Unk3      = "unk3";
        const string ATTR_Unk4      = "unk4";
        const string ATTR_MemOffset = "memoffset";
        const string ATTR_BitShift  = "bitshift";
        const string ATTR_EntID     = "entid";
        const string ATTR_MapID     = "mapid";

    };



//
//
//
    class ConfigXMLParser
    {
    public:
        ConfigXMLParser(const std::string & configfile)
        {
            string confpath = utils::MakeAbsolutePath(configfile);
            pugi::xml_parse_result result = m_doc.load_file( confpath.c_str() );
            //regex basepathex("(.+.+(?=\\b\\/))(.+\\..+)");
            //smatch sm;
            m_confbasepath = std::move( utils::GetPathOnly( confpath ) );


            //if(regex_match( configfile, sm, basepathex ) && sm.size() > 2 )
                //m_confbasepath = sm[1].str();
            //else
            //    throw std::runtime_error("ConfigXMLParser::ConfigXMLParser(): Couldn't parse config file's base path!");

            if( !result )
                throw std::runtime_error("ConfigXMLParser::ConfigXMLParser(): Couldn't parse configuration file!");
        }

        void ParseDataForGameVersion( uint16_t arm9off14, ConfigLoader & target )
        {
            if( FindGameVersion(arm9off14) )
                DoParse(target);
            else
            {
                throw std::runtime_error("ConfigXMLParser::ParseDataForGameVersion(): Couldn't find a matching game version "
                                         "from comparing the value in the arm9.bin file at offset 0xE, to the values in the "
                                         "configuration file!");
            }
        }

        void ParseDataForGameVersion( eGameVersion version, eGameRegion region, ConfigLoader & target )
        {
            if( FindGameVersion(version, region) )
                DoParse(target);
            else
            {
                throw std::runtime_error("ConfigXMLParser::ParseDataForGameVersion(): Couldn't find a matching game version "
                                         "from comparing the region and version specified, to the values in the "
                                         "configuration file!");
            }
        }

    private:

        bool MatchesCurrentVersionID( xml_attribute_iterator itatbeg, xml_attribute_iterator itatend )
        {
            using namespace ConfigXML;
            for( ; itatbeg != itatend; ++itatbeg )
            {
                if( (itatbeg->name() == ATTR_ID ||
                    itatbeg->name() == ATTR_ID2 ||
                    itatbeg->name() == ATTR_ID3) &&
                    itatbeg->value() == m_curversion.id )
                {
                    return true;
                }
            }
            return false;
        }

        inline void ParseAllFields(xml_node & pmd2n)
        {
            using namespace ConfigXML;
            ParseLanguages(pmd2n);
            ParseBinaries (pmd2n);
            ParseConstants(pmd2n);
            ParseGameScriptData(pmd2n);

            for( auto & xternal : pmd2n.children(NODE_ExtFile.c_str()) )
            {
                xml_attribute xfp = xternal.attribute(ATTR_FPath.c_str());
                if(xfp)
                    HandleExtFile(xfp.value());
            }
        }

        void DoParse(ConfigLoader & target)
        {
            using namespace ConfigXML;
            xml_node pmd2node = m_doc.child(ROOT_PMD2.c_str());
            ParseAllFields(pmd2node);
            target.m_langdb      = std::move(LanguageFilesDB(std::move(m_lang)));
            target.m_binblocks   = std::move(m_bin);
            target.m_constants   = std::move(m_constants); 
            target.m_versioninfo = std::move(m_curversion); //Done in last
            target.m_gscriptdata = std::move(m_gscriptdata);
        }

        bool FindGameVersion( eGameVersion version, eGameRegion region )
        {
            using namespace pugi;
            using namespace ConfigXML;
            auto lambdafind = [&]( xml_node curnode )->bool
            {
                xml_attribute reg = curnode.attribute(ATTR_Region.c_str());
                xml_attribute ver = curnode.attribute(ATTR_Version.c_str());
                return (reg && ver) && ( ( StrToGameVersion(ver.value()) == version ) && ( StrToGameRegion(reg.value()) == region ) );
            };
            return FindGameVersion( lambdafind );
        }

        bool FindGameVersion(uint16_t arm9off14)
        {
            using namespace pugi;
            using namespace ConfigXML;
            auto lambdafind = [&]( xml_node curnode )->bool
            {
                xml_attribute foundattr = curnode.attribute(ATTR_ARM9Off.c_str());
                return foundattr && (static_cast<decltype(m_curversion.arm9off14)>(foundattr.as_uint()) == arm9off14);
            };
            return FindGameVersion( lambdafind );
        }
        
        bool FindGameVersion( std::function<bool(pugi::xml_node)> && predicate )
        {
            using namespace pugi;
            using namespace ConfigXML;
            xml_node gamed = m_doc.child(ROOT_PMD2.c_str()).child(NODE_GameEd.c_str());

            if( !gamed )
                return false;

            for( auto & curvernode : gamed.children(NODE_Game.c_str()) )
            {
                //GameVersionInfo curver;
                //xml_attribute foundattr = curvernode.attribute(ATTR_ARM9Off);

                //if( foundattr && static_cast<decltype(m_curversion.arm9off14)>(foundattr.as_uint()) == arm9off14 )
                if( predicate(curvernode) )
                {
                    for( auto & curattr : curvernode.attributes() )
                    {
                        if( curattr.name() == ATTR_ID )
                            m_curversion.id = curattr.value();
                        else if( curattr.name() == ATTR_GameCode )
                            m_curversion.code = curattr.value();
                        else if( curattr.name() == ATTR_Version )
                            m_curversion.version = StrToGameVersion(curattr.value());
                        else if( curattr.name() == ATTR_Region )
                            m_curversion.region = StrToGameRegion(curattr.value());
                        else if( curattr.name() == ATTR_ARM9Off )
                            m_curversion.arm9off14 = static_cast<decltype(m_curversion.arm9off14)>(curattr.as_uint());
                        else if( curattr.name() == ATTR_DefLang)
                            m_curversion.defaultlang = StrToGameLang( curattr.value() );
                        else if( curattr.name() == ATTR_Support)
                            m_curversion.issupported = curattr.as_bool();
                    }
                    return true;
                }
            }
            return false;
        }

        LanguageFilesDB::blocks_t::blkcnt_t ParseStringBlocks( pugi::xml_node & parentn )
        {
            using namespace ConfigXML;
            LanguageFilesDB::blocks_t::blkcnt_t stringblocks;
            for( auto strblk : parentn.child(NODE_StrBlks.c_str()).children(NODE_StrBlk.c_str()) )
            {
                string      blkn;
                strbounds_t bnd;
                for( auto att : strblk.attributes() )
                {
                    if( att.name() == ATTR_Name )
                        blkn = att.value();
                    else if( att.name() == ATTR_Beg )
                        bnd.beg = att.as_uint();
                    else if( att.name() == ATTR_End )
                        bnd.end = att.as_uint();
                }
                eStringBlocks strblock = StrToStringBlock(blkn);

                if( strblock != eStringBlocks::Invalid )
                    stringblocks.emplace( strblock, std::move(bnd) );
                else
                    clog <<"Ignored invalid string block \"" <<blkn <<"\".\n";
            }

            if( stringblocks.size() != static_cast<size_t>(eStringBlocks::NBEntries) )
            {
                stringstream sstr;
                sstr <<"ConfigXMLParser::ParseALang(): The ";
                for( size_t i = 0; i < static_cast<size_t>(eStringBlocks::NBEntries); ++i )
                {
                    if( stringblocks.find( static_cast<eStringBlocks>(i) ) == stringblocks.end() )
                    {
                        if( i != 0 )
                            sstr << ",";
                        sstr << StringBlocksNames[i];
                    }
                }
                if( (static_cast<size_t>(eStringBlocks::NBEntries) - stringblocks.size()) > 1 )
                    sstr << " string blocks for " <<m_curversion.id <<" are missing from the configuration file!";
                else
                    sstr << " string block for " <<m_curversion.id <<" is missing from the configuration file!";
                
                if(utils::LibWide().isLogOn())
                    clog << sstr.str();
                cout << sstr.str();
            }
            return std::move(stringblocks);
        }

        void ParseLanguageList( pugi::xml_node                       & gamev, 
                                LanguageFilesDB::blocks_t::blkcnt_t  & blocks, 
                                LanguageFilesDB::strfiles_t          & dest )
        {
            using namespace ConfigXML;
            using namespace pugi;

            for( auto lang : gamev.child(NODE_Languages.c_str()).children(NODE_Language.c_str()) )
            {
                xml_attribute fname    = lang.attribute(ATTR_FName.c_str());
                xml_attribute langname = lang.attribute(ATTR_Name.c_str());
                xml_attribute loc      = lang.attribute(ATTR_Loc.c_str());
                eGameLanguages elang   = StrToGameLang(langname.value());
                        
                if(elang >= eGameLanguages::NbLang )
                {
                    throw std::runtime_error("ConfigXMLParser::ParseLang(): Unexpected language string \"" + 
                                                std::string(langname.value()) + "\" in config file!!");
                }

                dest.emplace( fname.value(), std::move(LanguageFilesDB::blocks_t(fname.value(), loc.value(), elang, blocks )) );
            }
        }

        void ParseLanguages(xml_node & pmd2n)
        {
            using namespace ConfigXML;
            using namespace pugi;
            xml_node gamevernode = pmd2n.child(NODE_StrIndex.c_str());

            for( auto gamev : gamevernode.children(NODE_Game.c_str()) )
            {
                xml_attribute id   = gamev.attribute(ATTR_ID.c_str());
                xml_attribute id2  = gamev.attribute(ATTR_ID2.c_str());
                if( id.value() == m_curversion.id || id2.value() == m_curversion.id )   //Ensure one of the compatible game ids matches the parser's!
                    ParseLanguageList(gamev,ParseStringBlocks(gamev),m_lang);
            }
        }

        void ParseBinaries(xml_node & pmd2n)
        {
            using namespace pugi;
            using namespace ConfigXML;
            xml_node binnode  = pmd2n.child(NODE_Binaries.c_str());
            if(!binnode)
                return;

            xml_node foundver = binnode.find_child_by_attribute( NODE_Game.c_str(), ATTR_ID.c_str(), m_curversion.id.c_str() );
            if( !foundver )
            {
                throw std::runtime_error("ConfigXMLParser::ParseBinaries(): Couldn't find binary offsets for game version \"" + 
                                         m_curversion.id + "\"!" );
            }

            for( auto curbin : foundver.children(NODE_Bin.c_str()) )
            {
                xml_attribute xfpath = curbin.attribute(ATTR_FPath.c_str());
                xml_attribute xlad   = curbin.attribute(ATTR_LoadAddr.c_str());
                binaryinfo binfo;
                binfo.loadaddress = xlad.as_uint();

                for( auto curblock : curbin.children(NODE_Block.c_str()) )
                {
                    binlocation curloc;
                    string      blkname;
                    for( auto att : curblock.attributes() )
                    {
                        if( att.name() == ATTR_Name )
                            blkname = att.value();
                        else if( att.name() == ATTR_Beg )
                            curloc.beg = att.as_uint();
                        else if( att.name() == ATTR_End )
                            curloc.end = att.as_uint();
                    }
                    binfo.blocks.emplace( StrToBinaryLocation(blkname), std::move(curloc) );
                }
                m_bin.AddBinary( std::string(xfpath.value()), std::move(binfo) );
            }
        }


        void ParseConstants( xml_node & pmd2n )
        {
            using namespace ConfigXML;
            xml_node constnode = pmd2n.child(NODE_GConsts.c_str());

            for( auto ver : constnode.children(NODE_Game.c_str()) )
            {
                xml_attribute xv1 = ver.attribute(ATTR_Version.c_str());
                xml_attribute xv2 = ver.attribute(ATTR_Version2.c_str());
                if( StrToGameVersion(xv1.value()) == m_curversion.version || StrToGameVersion(xv2.value()) == m_curversion.version )
                {
                    for( auto value : ver.children(NODE_Value.c_str()) )
                    {
                        xml_attribute xid  = value.attribute(ATTR_ID.c_str());
                        xml_attribute xval = value.attribute(ATTR_Val.c_str());
                        eGameConstants gconst = StrToGameConstant(xid.value());
                        if( gconst != eGameConstants::Invalid )
                            m_constants.emplace( gconst, std::move(std::string(xval.value())) );
                        else
                            clog << "<!>- Ignored unknown constant " <<xid.value() <<"\n";
                    }
                }
            }
        }


        void ParseGameScriptData(const xml_node & pmd2n )
        {
            using namespace ConfigXML;
            xml_node xscrdat = pmd2n.child(NODE_ScriptDat.c_str());

            //First find one that matches our current version
            for( const auto & gamenode : xscrdat.children(NODE_Game.c_str()) )
            {
                if( !MatchesCurrentVersionID(gamenode.attributes_begin(), gamenode.attributes_end()) )
                    continue;

                for( const auto & subnode : gamenode )
                {
                    if( subnode.name() == NODE_GVarTbl )
                        ParseGameVars(subnode);
                    else if( subnode.name() == NODE_GVarExtTbl )
                        ParseGameVarsExt(subnode);
                    else if( subnode.name() == NODE_LivesTbl )
                        ParseLives(subnode);
                    else if( subnode.name() == NODE_LevelList )
                        ParseLevels(subnode);
                    else if( subnode.name() == NODE_FaceNames )
                        ParseFaceNames(subnode);
                    else if( subnode.name() == NODE_FacePosMo )
                        ParseFaceModeNames(subnode);
                    else if( subnode.name() == NODE_CRoutineI )
                        ParseCommonRoutines(subnode);
                    else if(subnode.name() == NODE_ObjectLst )
                        ParseObjects(subnode);
                }
            }

        }

        pair<string,gamevariable_info> ParseAGameVarEntry(const xml_node & gvarn )
        {
            using namespace ConfigXML;
            gamevariable_info gvarinf;
            for( const auto & attr : gvarn.attributes() )
            {
                if( attr.name() == ATTR_Name )
                    gvarinf.name = attr.value();
                else if( attr.name() == ATTR_Type )
                    gvarinf.type = static_cast<int16_t>(attr.as_int());
                else if( attr.name() == ATTR_Unk1 )
                    gvarinf.unk1 = static_cast<int16_t>(attr.as_int());
                else if( attr.name() == ATTR_MemOffset )
                    gvarinf.memoffset = static_cast<int16_t>(attr.as_int());
                else if( attr.name() == ATTR_BitShift )
                    gvarinf.bitshift = static_cast<int16_t>(attr.as_int());
                else if( attr.name() == ATTR_Unk3 )
                    gvarinf.unk3 = static_cast<int16_t>(attr.as_int());
                else if( attr.name() == ATTR_Unk4 )
                    gvarinf.unk4 = static_cast<int16_t>(attr.as_int());
            }
            if( gvarinf.name.empty() )
                throw std::runtime_error("ConfigXMLParser::ParseGameVars(): A game variable entity has no name!!");
            return make_pair( gvarinf.name, std::move(gvarinf) );
        }

        void ParseGameVars(const xml_node & gvarn)
        {
            using namespace ConfigXML;
            std::deque<pair<string,gamevariable_info>> gamevar;
            for( const auto & gv : gvarn.children(NODE_GVar.c_str()) )
                gamevar.push_back(std::move(ParseAGameVarEntry(gv)));
            m_gscriptdata.m_gvars.PushEntriesPairs(gamevar.begin(), gamevar.end());
        }

        void ParseGameVarsExt(const xml_node & gvarexn)
        {
            using namespace ConfigXML;
            std::deque<pair<string,gamevariable_info>> gamevarex;
            for( const auto & gv : gvarexn.children(NODE_GVar.c_str()) )
                gamevarex.push_back(std::move(ParseAGameVarEntry(gv)));
            m_gscriptdata.m_gvarsex.PushEntriesPairs(gamevarex.begin(), gamevarex.end());
        }

        void ParseLives(const xml_node & livesn)
        {
            using namespace ConfigXML;
            std::deque<pair<string,livesent_info>> lives;
            for( const auto & livesent : livesn.children(NODE_Entity.c_str()) )
            {
                livesent_info lvlinf;
                for( const auto & attr : livesent.attributes() )
                {
                    if( attr.name() == ATTR_Name )
                        lvlinf.name = attr.value();
                    else if( attr.name() == ATTR_Type )
                        lvlinf.type = static_cast<int16_t>(attr.as_int());
                    else if( attr.name() == ATTR_EntID )
                        lvlinf.entid = static_cast<int16_t>(attr.as_int());
                    else if( attr.name() == ATTR_Unk3 )
                        lvlinf.unk3 = static_cast<int16_t>(attr.as_int());
                    else if( attr.name() == ATTR_Unk4 )
                        lvlinf.unk4 = static_cast<int16_t>(attr.as_int());
                }
                if( lvlinf.name.empty() )
                    throw std::runtime_error("ConfigXMLParser::ParseLives(): A lives entity has no name!!");
                lives.push_back(make_pair(lvlinf.name,std::move(lvlinf)));
            }
            m_gscriptdata.m_livesent.PushEntriesPairs(lives.begin(), lives.end());
        }

        void ParseLevels(const xml_node & leveln)
        {
            using namespace ConfigXML;
            std::deque<pair<string,level_info>> levels;
            for( const auto & level : leveln.children(NODE_Level.c_str()) )
            {
                level_info lvlinf;
                for( const auto & attr : level.attributes() )
                {
                    if( attr.name() == ATTR_Name )
                        lvlinf.name = attr.value();
                    else if( attr.name() == ATTR_Unk1 )
                        lvlinf.unk1 = static_cast<int16_t>(attr.as_int());
                    else if( attr.name() == ATTR_Unk2 )
                        lvlinf.unk2 = static_cast<int16_t>(attr.as_int());
                    else if( attr.name() == ATTR_MapID )
                        lvlinf.mapid = static_cast<int16_t>(attr.as_int());
                    else if( attr.name() == ATTR_Unk4 )
                        lvlinf.unk4 = static_cast<int16_t>(attr.as_int());
                }
                if( lvlinf.name.empty() )
                    throw std::runtime_error("ConfigXMLParser::ParseLevels(): A level has no name!!");
                levels.push_back(make_pair(lvlinf.name,std::move(lvlinf)));
            }
            m_gscriptdata.m_levels.PushEntriesPairs(levels.begin(), levels.end());
        }

        void ParseFaceNames(const xml_node & facen)
        {
            using namespace ConfigXML;
            std::deque<pair<string,string>> facenames;
            for( const auto & face : facen.children(NODE_Face.c_str()) )
                facenames.push_back( make_pair(face.text().get(),face.text().get()) );
            m_gscriptdata.m_facenames.PushEntriesPairs(facenames.begin(), facenames.end());
        }

        void ParseFaceModeNames(const xml_node & facemodesn)
        {
            using namespace ConfigXML;
            std::deque<pair<string,string>> facemodes;
            for( const auto & facem : facemodesn.children(NODE_Mode.c_str()) )
                facemodes.push_back(make_pair(facem.text().get(), facem.text().get()));
            m_gscriptdata.m_faceposmodes.PushEntriesPairs(facemodes.begin(), facemodes.end());
        }

        void ParseCommonRoutines(const xml_node & routinesn)
        {
            using namespace ConfigXML;
            deque<pair<string,commonroutine_info>> routines;
            for( const auto & routine : routinesn.children(NODE_Routine.c_str()) )
            {
                commonroutine_info crinfo;
                for( const auto & attr : routine.attributes() )
                {
                    if( attr.name() == ATTR_ID )
                        crinfo.id = static_cast<int16_t>(attr.as_int());
                    else if( attr.name() == ATTR_Unk1 )
                        crinfo.unk1 = static_cast<int16_t>(attr.as_int());
                    else if( attr.name() == ATTR_Name )
                        crinfo.name = attr.value();
                }
                if( crinfo.name.empty() )
                    throw std::runtime_error("ConfigXMLParser::ParseCommonRoutines(): A routine has no name!!");
                routines.push_back(make_pair( crinfo.name, std::move(crinfo)));
            }
            m_gscriptdata.m_commonroutines.PushEntriesPairs(routines.begin(), routines.end());
        }

        void ParseObjects(const xml_node & objectsn)
        {
            using namespace ConfigXML;
            deque<pair<string,object_info>> objects;
            for( const auto & obj : objectsn.children(NODE_Object.c_str()) )
            {
                object_info curobj;
                for( const auto & attr : obj.attributes() )
                {
                    //if( attr.name() == ATTR_ID ) //The id is a dummy
                    //    curobj.id = static_cast<int16_t>(attr.as_int());
                    /*else*/ if( attr.name() == ATTR_Unk1 )
                        curobj.unk1 = static_cast<int16_t>(attr.as_int());
                    else if( attr.name() == ATTR_Unk2 )
                        curobj.unk2 = static_cast<int16_t>(attr.as_int());
                    else if( attr.name() == ATTR_Unk3 )
                        curobj.unk3 = static_cast<int16_t>(attr.as_int());
                    else if( attr.name() == ATTR_Name )
                        curobj.name = attr.value();
                }
                if( curobj.name.empty() )
                    throw std::runtime_error("ConfigXMLParser::ParseObjects(): An object has no name!!");
                objects.push_back(make_pair( curobj.name, std::move(curobj)));
            }
            m_gscriptdata.m_objectsinfo.PushEntriesPairs( objects.begin(), objects.end() );
        }

        void HandleExtFile( std::string extfile )
        {
            using namespace ConfigXML;
            //When we hid an external file node, parse it too if possible

            //Make path relative to the config file's base directory!
            if( utils::pathIsRelative(extfile) )
                extfile = utils::MakeAbsolutePath(extfile,m_confbasepath);

            bool bfoundsubdoc=false;
            for( const auto & entry: m_subdocs )
            {
                if( entry == extfile )
                {
                    bfoundsubdoc = true;
                    break;
                }
            }

            if( m_doc.path() != extfile && !bfoundsubdoc )
            {
                xml_document doc;
                m_subdocs.push_back(extfile);
                if( !(doc.load_file(extfile.c_str())) )
                {
                    m_subdocs.pop_back();
                    clog<<"<!>- ConfigXMLParser::HandleExtFile(): Couldn't open sub configuration file \"" <<extfile <<"\"!";
                    return;
                }
                xml_node pmd2node = doc.child(ROOT_PMD2.c_str());

                //Parse the data fields of the sub-file
                if(pmd2node)
                    ParseAllFields(pmd2node);
                m_subdocs.pop_back();
            }
            else
                clog<<"<!>- ConfigXMLParser::HandleExtFile(): External file node with same name as original document encountered! Ignoring to avoid infinite loop..\n";
        }

    private:
        GameVersionInfo                 m_curversion;
        pugi::xml_document              m_doc;
        std::deque<std::string>         m_subdocs;

        ConfigLoader::constcnt_t        m_constants;
        LanguageFilesDB::strfiles_t     m_lang;
        GameBinariesInfo                m_bin;
        string                          m_confbasepath;
        GameScriptData                  m_gscriptdata;
    };


//
//
//
    ConfigLoader::ConfigLoader(uint16_t arm9off14, const std::string & configfile)
        :m_conffile(configfile),m_arm9off14(arm9off14)
    {
        Parse(arm9off14);
    }

    ConfigLoader::ConfigLoader(eGameVersion version, eGameRegion region, const std::string & configfile)
        :m_conffile(configfile)
    {
        Parse(version,region);
    }

    void ConfigLoader::ReloadConfig()
    {
        Parse(m_versioninfo.version, m_versioninfo.region);
    }

    int ConfigLoader::GetGameConstantAsInt(eGameConstants gconst) const
    {
        return m_constants.GetConstAsInt<int>(gconst);
    }

    unsigned int ConfigLoader::GetGameConstantAsUInt(eGameConstants gconst) const
    {
        return m_constants.GetConstAsInt<unsigned int>(gconst);
    }

    void ConfigLoader::Parse( uint16_t arm9off14 )
    {
        ConfigXMLParser(m_conffile).ParseDataForGameVersion(arm9off14,*this);
    }

    void ConfigLoader::Parse(eGameVersion version, eGameRegion region)
    {
        ConfigXMLParser(m_conffile).ParseDataForGameVersion(version, region, *this);
    }


//
//
//
    MainPMD2ConfigWrapper  MainPMD2ConfigWrapper::s_instance;

    MainPMD2ConfigWrapper & MainPMD2ConfigWrapper::Instance()
    {
        return s_instance;
    }

    ConfigLoader & MainPMD2ConfigWrapper::CfgInstance()
    {
        if(s_instance.m_loaderinstance)
            return *(s_instance.m_loaderinstance.get());
        else
            throw std::logic_error("MainPMD2ConfigWrapper::CfgInstance(): Configuration was not loaded!!");
    }

    void MainPMD2ConfigWrapper::InitConfig(uint16_t arm9off14, const std::string & configfile)
    {
        m_loaderinstance.reset( new ConfigLoader(arm9off14,configfile) );
    }

    void MainPMD2ConfigWrapper::InitConfig(eGameVersion version, eGameRegion region, const std::string & configfile)
    {
        m_loaderinstance.reset( new ConfigLoader(version,region,configfile) );
    }

    inline void MainPMD2ConfigWrapper::ReloadConfig()
    {
        if(m_loaderinstance)
            m_loaderinstance->ReloadConfig();
        else
            throw std::logic_error("MainPMD2ConfigWrapper::ReloadConfig(): Configuration was not loaded!!");
    }

};