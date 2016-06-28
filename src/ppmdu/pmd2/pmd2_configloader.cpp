#include "pmd2_configloader.hpp"
#include <utils/pugixml_utils.hpp>
#include <utils/parse_utils.hpp>
#include <iostream>
using namespace std;

namespace pmd2
{
    const std::array<std::string, static_cast<uint32_t>(eStringBlocks::NBEntries)> StringBlocksNames=
    {
        "PkmnNames",
        "PkmnCats",
        "MvNames",
        "MvDesc",
        "ItemNames",
        "ItemDescS",
        "ItemDescL",
        "AbilityNames",
        "AbilityDesc",
        "TypeNames",
        "PortraitNames",
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
    };


    eStringBlocks FindStrBlock( const std::string & blkname )
    {
        for( size_t i = 0; i < StringBlocksNames.size(); ++i )
        {
            if( blkname == StringBlocksNames[i] )
                return static_cast<eStringBlocks>(i);
        }
        return eStringBlocks::Invalid;
    }


    eBinaryLocations FindBinaryLocation( const std::string & locname )
    {
        for( size_t i = 0; i < BinaryLocationNames.size(); ++i )
        {
            if( locname == BinaryLocationNames[i] )
                return static_cast<eBinaryLocations>(i);
        }
        return eBinaryLocations::Invalid;
    }

    eGameConstants FindGameConstantName( const std::string & constname )
    {
        for( size_t i = 0; i < GameConstantNames.size(); ++i )
        {
            if( constname == GameConstantNames[i] )
                return static_cast<eGameConstants>(i);
        }
        return eGameConstants::Invalid;
    }


    namespace ConfigXML
    {
        const char * ROOT_PMD2      = "PMD2";

        const char * NODE_GameEd    = "GameEditions";
        const char * NODE_Game      = "Game";
        const char * NODE_GConsts   = "GameConstants";
        const char * NODE_Binaries  = "Binaries";
        const char * NODE_Bin       = "Bin";
        const char * NODE_Block     = "Block";
        const char * NODE_StrIndex  = "StringIndexData";
        const char * NODE_Language  = "Language";
        const char * NODE_StrBlk    = "StringBlock";
        const char * NODE_Value     = "Value";

        const char * ATTR_ID        = "id";
        const char * ATTR_ID2       = "id2";
        const char * ATTR_GameCode  = "gamecode";
        const char * ATTR_Version   = "version";
        const char * ATTR_Version2  = "version2";
        const char * ATTR_Region    = "region";
        const char * ATTR_ARM9Off   = "arm9off14";
        const char * ATTR_DefLang   = "defaultlang";
        const char * ATTR_Support   = "issupported";
        const char * ATTR_Val       = "value";
        const char * ATTR_Name      = "name";
        const char * ATTR_Beg       = "beg";
        const char * ATTR_End       = "end";
        const char * ATTR_Loc       = "locale";
        const char * ATTR_FName     = "filename";
        const char * ATTR_FPath     = "filepath";

    };





    //GameVersionsData::GameVersionsData(gvcnt_t && gvinf)
    //    :BaseGameKeyValData(std::forward<gvcnt_t>(gvinf))
    //{}

    //GameVersionsData::const_iterator GameVersionsData::GetVersionForArm9Off14(uint16_t arm9off14) const
    //{
    //    for( auto it= m_kvdata.begin(); it != m_kvdata.end(); ++it )
    //    {
    //        if( it->second.arm9off14 == arm9off14 )
    //            return it;
    //    }
    //    return m_kvdata.end();
    //}

    //GameVersionsData::const_iterator GameVersionsData::GetVersionForId(const std::string & id) const
    //{
    //    return m_kvdata.find(id);
    //}

//
//
//
    //GameBinaryOffsets::GameBinaryOffsets(gvcnt_t && gvinf)
    //    :BaseGameKeyValData<GameBinLocCatalog>(), m_kvdata(std::forward<gvcnt_t>(gvinf))
    //{
    //    
    //}
    //GameBinaryOffsets::const_iterator GameBinaryOffsets::GetOffset(const std::string & gameversionid) const
    //{
    //    return m_kvdata.find(gameversionid);
    //}

//
//
//
    //GameConstantsData::GameConstantsData(gvcnt_t && gvinf)
    //{
    //}

    //GameConstantsData::const_iterator GameConstantsData::GetConstant(eGameVersion vers, const std::string & name) const
    //{
    //    return const_iterator();
    //}

//
//
//
    //class ConfigLoaderImpl
    //{
    //public:
    //    ConfigLoaderImpl( const string & file )
    //    {
    //    }
    //};


//
//
//
    class ConfigXMLParser
    {
    public:
        ConfigXMLParser(const std::string & configfile)
        {
            pugi::xml_parse_result result = m_doc.load_file( configfile.c_str() );
            if( !result )
                throw std::runtime_error("ConfigXMLParser::ConfigXMLParser(): Couldn't parse configuration file!");
        }

        void ParseDataForGameVersion( uint16_t arm9off14, ConfigLoader & target )
        {
            if( FindGameVersion(arm9off14) )
            {
                target.m_strindex    = std::move( GameStringIndex(ParseGameStringIndices()) );
                target.m_binoffsets  = std::move( ParseBinaries() );
                target.m_constants   = std::move( ParseConstants() ); 

                target.m_versioninfo = std::move(m_curversion); //Done in last
            }
            else
            {
                throw std::runtime_error("ConfigXMLParser::ParseDataForGameVersion(): Couldn't find a matching game version "
                                         "from comparing the value in the arm9.bin file at offset 0xE, to the values in the "
                                         "configuration file!");
            }
        }

    private:

        bool FindGameVersion(uint16_t arm9off14)
        {
            using namespace pugi;
            using namespace ConfigXML;
            xml_node gamed = m_doc.child(ROOT_PMD2).child(NODE_GameEd);

            if( !gamed )
                return false;

            for( auto & curvernode : gamed.children(NODE_Game) )
            {
                //GameVersionInfo curver;
                xml_attribute foundattr = curvernode.attribute(ATTR_ARM9Off);

                if( foundattr && static_cast<decltype(m_curversion.arm9off14)>(foundattr.as_uint()) == arm9off14 )
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

        GameStringIndex::strfiles_t ParseGameStringIndices()
        {
            using namespace ConfigXML;
            using namespace pugi;
            GameStringIndex::strfiles_t dest;

            xml_node gamevernode = m_doc.child(ROOT_PMD2).child(NODE_StrIndex);

            for( auto gamev : gamevernode.children(NODE_Game) )
            {
                xml_attribute id  = gamev.attribute(ATTR_ID);
                xml_attribute id2 = gamev.attribute(ATTR_ID2);
                if( id.value() == m_curversion.id || id2.value() == m_curversion.id )
                {
                    for( auto lang : gamev.children(NODE_Language) )
                    {
                        xml_attribute fname    = lang.attribute(ATTR_FName);
                        xml_attribute langname = lang.attribute(ATTR_Name);
                        xml_attribute loc      = lang.attribute(ATTR_Loc);
                        eGameLanguages elang   = StrToGameLang(langname.value());
                        
                        if(elang >= eGameLanguages::NbLang )
                        {
                            throw std::runtime_error("ConfigXMLParser::ParseLang(): Unexpected language string \"" + 
                                                      std::string(langname.value()) + "\" in config file!!");
                        }

                        dest.emplace( fname, std::move(ParseLang(gamev, fname.value(), loc.value(), elang )) );
                    }
                }
            }
            return std::move(dest);
        }

        GameStringIndex::blocks_t ParseLang( pugi::xml_node langnode, std::string && fname, std::string && locale, eGameLanguages lang )
        {
            using namespace ConfigXML;
            using namespace pugi;
            GameStringIndex::blocks_t::blkcnt_t dest;

            for( auto strblk : langnode )
            {
                string                     blkn;
                GameStringIndex::strbounds bnd;
                for( auto att : strblk.attributes() )
                {
                    if( att.name() == ATTR_Name )
                        blkn = att.value();
                    else if( att.name() == ATTR_Beg )
                        bnd.beg = att.as_uint();
                    else if( att.name() == ATTR_End )
                        bnd.end = att.as_uint();
                }
                dest.emplace( std::move(blkn), std::move(bnd) );
            }
            
            return std::move(GameStringIndex::blocks_t( std::move(fname), std::move(locale), lang, std::move(dest) ) );
        }


        ConfigLoader::bincnt_t ParseBinaries()
        {
            using namespace pugi;
            using namespace ConfigXML;
            xml_node binnode  = m_doc.child(ROOT_PMD2).child(NODE_Binaries);
            xml_node foundver = binnode.find_child_by_attribute( NODE_Game, ATTR_ID, m_curversion.id.c_str() );

            if( !foundver )
            {
                throw std::runtime_error("ConfigXMLParser::ParseBinaries(): Couldn't find binary offsets for game version \"" + 
                                         m_curversion.id + "\"!" );
            }
            ConfigLoader::bincnt_t dest;

            for( auto curbin : foundver.children(NODE_Bin) )
            {
                xml_attribute xfpath = curbin.attribute(ATTR_FPath);
                string        fpath  = xfpath.value();

                for( auto curblock : curbin.children(NODE_Block) )
                {
                    GameBinaryOffsetInfo bifo;
                    string               blkname;
                    bifo.fpath = fpath;
                    for( auto att : curblock.attributes() )
                    {
                        if( att.name() == ATTR_Name )
                            blkname = att.value();
                        else if( att.name() == ATTR_Beg )
                            bifo.beg = att.as_uint();
                        else if( att.name() == ATTR_End )
                            bifo.end = att.as_uint();
                    }
                    dest.emplace( FindBinaryLocation(blkname), std::move(bifo) );
                }
            }

            return std::move(dest);
        }


        ConfigLoader::constcnt_t ParseConstants()
        {
            using namespace pugi;
            using namespace ConfigXML;
            xml_node constnode  = m_doc.child(ROOT_PMD2).child(NODE_GConsts);
            ConfigLoader::constcnt_t dest;

            for( auto ver : constnode.children(NODE_Game) )
            {
                xml_attribute xv1 = ver.attribute(ATTR_Version);
                xml_attribute xv2 = ver.attribute(ATTR_Version2);
                if( StrToGameVersion(xv1.value()) == m_curversion.version || StrToGameVersion(xv2.value()) == m_curversion.version )
                {
                    for( auto value : ver.children(NODE_Value) )
                    {
                        xml_attribute xid  = value.attribute(ATTR_ID);
                        xml_attribute xval = value.attribute(ATTR_Val);
                        eGameConstants gconst = FindGameConstantName(xid.value());
                        if( gconst != eGameConstants::Invalid )
                            dest.emplace( gconst, std::move(std::string(xval.value())) );
                        else
                            clog << "<!>- Ignored unknown constant " <<xid.value() <<"\n";
                    }
                }
            }
            return std::move(dest);
        }

    private:
        GameVersionInfo      m_curversion;
        pugi::xml_document   m_doc;
    };


//
//
//
    ConfigLoader::ConfigLoader(uint16_t arm9off14, const std::string & configfile)
        :m_conffile(configfile),m_arm9off14(arm9off14)
    {
        Parse();
    }

    int ConfigLoader::GetGameConstantAsInt(eGameConstants gconst) const
    {
        return utils::parseHexaValToValue<int>( m_constants.at(gconst) );
    }

    unsigned int ConfigLoader::GetGameConstantAsUInt(eGameConstants gconst) const
    {
        return utils::parseHexaValToValue<unsigned int>( m_constants.at(gconst) );
    }



    void ConfigLoader::Parse()
    {
        ConfigXMLParser(m_conffile).ParseDataForGameVersion(m_arm9off14,*this);
    }

};