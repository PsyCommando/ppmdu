#include "move_data.hpp"
#include <utils/parse_utils.hpp>
#include <utils/pugixml_utils.hpp>
#include <utils/library_wide.hpp>
#include <utils/utility.hpp>
#include <pugixml.hpp>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <locale>
#include <vector>
#include <utility>
#include <functional>
#include <Poco/DirectoryIterator.h>
#include <Poco/File.h>
#include <Poco/Path.h>
using namespace std;
using namespace pugi;
using namespace pugixmlutils;


namespace pmd2 { namespace stats 
{
//=================================================================================
//  Constant
//=================================================================================
    namespace movesXML
    {

        const string ROOT_Move    = "Move";
        const string ATTR_GameVer = "GameVersion";

        const string NODE_Strings = "Strings";
        const string PROP_Name    = "Name";
        const string PROP_Desc    = "Description";

        const string NODE_Data    = "Data";
        const string PROP_BasePow = "BasePower";
        const string PROP_Type    = "Type";
        const string PROP_Category= "Category";
        const string PROP_Unk4    = "Unk4";
        const string PROP_Unk5    = "Unk5";
        const string PROP_BasePP  = "BasePP";
        const string PROP_Unk6    = "Unk6";
        const string PROP_Unk7    = "Unk7";
        const string PROP_Accuracy= "Accuracy";
        const string PROP_Unk9    = "Unk9";
        const string PROP_Unk10   = "Unk10";
        const string PROP_Unk11   = "Unk11";
        const string PROP_Unk12   = "Unk12";
        const string PROP_Unk13   = "Unk13";
        const string PROP_Unk14   = "Unk14";
        const string PROP_Unk15   = "Unk15";
        const string PROP_Unk16   = "Unk16";
        const string PROP_Unk17   = "Unk17";
        const string PROP_Unk18   = "Unk18";
        const string PROP_MoveID  = "MoveID";
        const string PROP_Unk19   = "Unk19";

        const char * GameVer_EoS  = "EoS";
        const char * GameVer_EoTD = "EoTEoD";
    };

//=================================================================================
//  Classes
//=================================================================================
    /**********************************************************************
        MoveDB_XMLWriter
    **********************************************************************/
    class MoveDB_XMLWriter
    {
    public:
        MoveDB_XMLWriter( const MoveDB      & src1,
                          const MoveDB      * src2   = nullptr,
                          const GameText    * pgtext = nullptr )
            :m_src1(src1), m_psrc2(src2), m_pgametext(pgtext), m_bNoStrings(false)
        {}

        void Write( const std::string & destdir )
        {
            if( !m_pgametext || ( m_pgametext && !m_pgametext->AreStringsLoaded() ) )
            {
                clog << "<!>- MoveDB_XMLWriter::Write(): No strings loaded! Ignoring strings!(Names, descriptions, etc..)\n";
                m_bNoStrings = true;
            }

            if( !utils::isFolder(destdir) )
            {
                stringstream sstr;
                sstr << "Invalid directory \"" <<destdir <<"\"!";
                throw runtime_error( sstr.str() );
            }

            if( !m_psrc2 )
                WriteEoS( destdir );
            else
                WriteEoTD( destdir );
        }

    private:

        inline string PrepareMvNameFName( const string & name, eGameLanguages lang  )
        {
            const string * plocstr = m_pgametext->GetLocaleString(lang);
            if( plocstr )
                return utils::CleanFilename( name.substr( 0, name.find("\\0",0 ) ), std::locale( *plocstr ) ); //Remove ending "\0" and remove illegal characters for filesystem
            else 
                return utils::CleanFilename( name.substr( 0, name.find("\\0",0 ) ) );
        }

        stringstream & MakeFilename( stringstream & out_fname, const string & outpathpre, unsigned int cntmv )
        {
            const string * pfstr = nullptr;
            if( !m_bNoStrings && (pfstr = m_pgametext->GetDefaultLanguage().GetStringIfBlockExists( eStringBlocks::MvNames, cntmv )) )
            {
                out_fname <<outpathpre <<setw(4) <<setfill('0') <<cntmv <<"_" 
                          <<PrepareMvNameFName(*pfstr, m_pgametext->begin()->first) <<".xml";
            }
            else
                out_fname <<outpathpre <<setw(4) <<setfill('0') <<cntmv <<".xml";

            return out_fname;
        }


        void WriteEoS( const std::string & destdir )
        {
            using namespace movesXML;
            const string dirprefix = utils::TryAppendSlash( destdir );
            stringstream fname;

            if( m_src1.size() != m_psrc2->size() )
                throw runtime_error("Size mismatch between the two move data lists! One list of moves is longer than the other!");

            for( unsigned int cntmv = 0; cntmv < m_src1.size(); ++cntmv/*, ++itcurname, ++itcurdesc*/ )
            {
                fname.str(string());
                xml_document doc;
                xml_node     movedata = doc.append_child( ROOT_Move.c_str() );
                AppendAttribute( movedata, ATTR_GameVer, pmd2::GetGameVersionName( eGameVersion::EoS ) );
                WriteCommentNode( movedata, "Pokemon Mystery Dungeon: Explorers of Sky move data" );

                if( !m_bNoStrings )
                    WriteStrings( movedata, cntmv );

                WriteCommentNode( movedata, "Move data from waza_p.bin" );
                WriteMove( movedata, m_src1[cntmv] );
                WriteCommentNode( movedata, "Move data from waza_p2.bin" );
                WriteMove( movedata, (*m_psrc2)[cntmv] );

                MakeFilename(fname, dirprefix, cntmv);

                if( ! doc.save_file( fname.str().c_str() ) )
                {
                    stringstream strerr;
                    strerr << "Pugixml couldn't write file \"" <<fname.str() <<"\"!";
                    throw runtime_error(strerr.str());
                }
            }
        }

        void WriteEoTD( const std::string & destdir )
        {
            using namespace movesXML;
            const string dirprefix = utils::TryAppendSlash( destdir );
            stringstream fname;

            for( unsigned int cntmv = 0; cntmv < m_src1.size(); ++cntmv/*, ++itcurname, ++itcurdesc*/ )
            {
                fname.str(string());
                xml_document doc;
                xml_node     movedata = doc.append_child( ROOT_Move.c_str() );
                AppendAttribute( movedata, ATTR_GameVer, pmd2::GetGameVersionName( eGameVersion::EoT ) );
                AppendAttribute( movedata, ATTR_GameVer, pmd2::GetGameVersionName( eGameVersion::EoD ) );
                WriteCommentNode( movedata, "Pokemon Mystery Dungeon: Explorers of Time/Darkness move data" );

                if( !m_bNoStrings )
                    WriteStrings( movedata, cntmv );

                WriteCommentNode( movedata, "Move data from waza_p.bin" );
                WriteMove   ( movedata, m_src1[cntmv] );

                MakeFilename(fname, dirprefix, cntmv);
                //fname <<dirprefix <<setw(4) <<setfill('0') <<dec <<i <<"_" << PrepareMvNameFName(*itcurname) <<".xml";

                if( ! doc.save_file( fname.str().c_str() ) )
                {
                    stringstream strerr;
                    strerr << "Pugixml couldn't write file \"" <<fname.str() <<"\"!";
                    throw runtime_error(strerr.str());
                }
            }
        }

        void WriteStrings( xml_node & mn, unsigned int cntmv )
        {
            using namespace movesXML;
            WriteCommentNode( mn, "In-game text" );
            xml_node strnode = mn.append_child( NODE_Strings.c_str() );

            //Add all loaded languages
            for( const auto & alang : *m_pgametext )
            {
                xml_node langnode = strnode.append_child( GetGameLangName(alang.first).c_str() );
                //Write Name
                const string * pname = alang.second.GetStringIfBlockExists(eStringBlocks::MvNames,cntmv);
                if( pname )
                    WriteNodeWithValue( langnode, PROP_Name, utils::StrRemoveAfter( *pname, "\\0" ) ); //remove ending \0
                //Write Description
                const string * pdesc = alang.second.GetStringIfBlockExists(eStringBlocks::MvDesc,cntmv);
                if( pdesc )
                    WriteNodeWithValue( langnode, PROP_Category, utils::StrRemoveAfter( *pdesc, "\\0" ) ); //remove ending \0
            }
        }

        void WriteMove( xml_node & pn, const MoveData & mvdata )
        {
            using namespace movesXML;
            xml_node datnode = pn.append_child( NODE_Data.c_str() );
            WriteNodeWithValue( datnode, PROP_BasePow, mvdata.basePower );
            WriteNodeWithValue( datnode, PROP_Type,    mvdata.type      );
            WriteNodeWithValue( datnode, PROP_Category,mvdata.category  );
            WriteNodeWithValue( datnode, PROP_Unk4,    mvdata.unk4      );
            WriteNodeWithValue( datnode, PROP_Unk5,    mvdata.unk5      );
            WriteNodeWithValue( datnode, PROP_BasePP,  mvdata.basePP    );
            WriteNodeWithValue( datnode, PROP_Unk6,    mvdata.unk6      );
            WriteNodeWithValue( datnode, PROP_Unk7,    mvdata.unk7      );
            WriteNodeWithValue( datnode, PROP_Accuracy,mvdata.accuracy  );
            WriteNodeWithValue( datnode, PROP_Unk9,    mvdata.unk9      );
            WriteNodeWithValue( datnode, PROP_Unk10,   mvdata.unk10     );
            WriteNodeWithValue( datnode, PROP_Unk11,   mvdata.unk11     );
            WriteNodeWithValue( datnode, PROP_Unk12,   mvdata.unk12     );
            WriteNodeWithValue( datnode, PROP_Unk13,   mvdata.unk13     );
            WriteNodeWithValue( datnode, PROP_Unk14,   mvdata.unk14     );
            WriteNodeWithValue( datnode, PROP_Unk15,   mvdata.unk15     );
            WriteNodeWithValue( datnode, PROP_Unk16,   mvdata.unk16     );
            WriteNodeWithValue( datnode, PROP_Unk17,   mvdata.unk17     );
            WriteNodeWithValue( datnode, PROP_Unk18,   mvdata.unk18     );
            WriteNodeWithValue( datnode, PROP_MoveID,  mvdata.moveID    );
            WriteNodeWithValue( datnode, PROP_Unk19,   mvdata.unk19     );
        }

    private:
        const MoveDB                   &m_src1;
        const MoveDB                   *m_psrc2;
        const GameText                 *m_pgametext;
        bool                            m_bNoStrings;
    };

    /**********************************************************************
        MoveDB_XMLParser
    **********************************************************************/
    class MoveDB_XMLParser
    {
    public:
        typedef pair<vector<string>::iterator,vector<string>::iterator> range_t;

        /*
        */
        MoveDB_XMLParser( MoveDB & out_mdb1, MoveDB * out_mdb2 = nullptr, GameText * pgtext = nullptr )
            :m_out1(out_mdb1), m_pout2(out_mdb2), m_pgametext(pgtext),m_bNoStrings(false), m_bParseMoveId(false)
        {}

        /*
        */
        void Parse( const string & srcdir, bool bparsemoveids = true ) 
        {
            m_bParseMoveId = bparsemoveids;
            if( !m_pgametext || ( m_pgametext && !m_pgametext->AreStringsLoaded() ) )
            {
                clog << "<!>- MoveDB_XMLParser::Parse(): No strings loaded! Ignoring strings!(Names, descriptions, etc..)\n";
                m_bNoStrings = true;
            }

            try
            {
                Poco::DirectoryIterator itDirEnd;
                vector<string>          filelst;

                for( Poco::DirectoryIterator itDir(srcdir); itDir != itDirEnd; ++itDir )
                {
                    if( itDir->isFile() && Poco::Path(itDir.path()).getExtension() == "xml" )
                        filelst.push_back( (itDir.path().absolute().toString()) );
                }

                if(filelst.empty())
                    throw runtime_error( "MoveDB_XMLParser::Parse(): Found no move files to parse!" );

                ReadAllMoves(filelst);
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
            Decides whether to parse the move id from the filename or just use the file counter value instead.
        */
        uint32_t GetCurrentMoveId( const std::string & mv, uint32_t cntmv )
        {
            uint32_t moveid = 0;
            if( m_bParseMoveId )
            {
                stringstream sstrmoveid;
                auto         itendnumber = std::find_if_not( mv.begin(), 
                                                             mv.end(), 
                                                             std::bind( &(std::isalnum<char>), 
                                                                        std::placeholders::_1, 
                                                                        std::ref(locale::classic()) ) );
                sstrmoveid << string( mv.begin(), itendnumber );
                sstrmoveid >> moveid;
            }
            else
                moveid = cntmv;
            return moveid;
        }

        /*
        */
        void ReadAllMoves( const vector<string> & files )
        {
            using namespace movesXML;
            MoveDB result1;
            MoveDB result2;

            result1.reserve(files.size());

            if( !m_pout2 )
                result2.reserve(files.size());

            //Parse files
            uint32_t cntmv = 0;
            for( auto & mv : files )
            {
                uint32_t         moveid = GetCurrentMoveId( mv, cntmv );
                xml_document     doc;
                xml_parse_result loadres = doc.load_file(mv.c_str());
                if( ! loadres )
                {
                    stringstream sstr;
                    sstr <<"Can't load XML document \"" <<mv <<"\"! Pugixml returned an error : \"" << loadres.description() <<"\"";
                    throw std::runtime_error(sstr.str());
                }

                pugi::xml_node movenode  = doc.child(ROOT_Move.c_str());
                if( !movenode )
                {
                    clog <<"<!>- MoveDB_XML_Parser::ReadAllMoves(): No move data found in XML file \"" <<mv <<"\". Skipping..\n";
                    continue;
                }

                eGameVersion gamever = DetectGameVersion(movenode);

                if( gamever == eGameVersion::EoS )
                    HandleMoveEoS( movenode, moveid, result1, result2 );
                else if( gamever == eGameVersion::EoT || gamever == eGameVersion::EoD )
                    HandleMoveEoTD( movenode, moveid, result1 );
                else
                {
                    clog <<"<!>- MoveDB_XML_Parser::ReadAllMoves(): Got move with invalid game version.. Skipping..\n";
                    continue;
                }

                ++cntmv;
            }

            m_out1 = move( result1 );
            if( m_pout2 )
                *m_pout2 = move( result2 );
            else if( !result2.empty() )
            {
                clog << "<!>- MoveDB_XML_Parser::ReadAllMoves(): Warning: Parsed extra move data for EoS, but the data was expected to be for EoT/D..\n";
            }
        }

        /*
        */
        eGameVersion DetectGameVersion( xml_node & movenode )
        {
            using namespace movesXML;
            auto          datanchilds = movenode.children(NODE_Data.c_str());
            xml_attribute agv         = movenode.attribute(ATTR_GameVer.c_str());
            eGameVersion  gamever     = eGameVersion::Invalid;

            if( agv )
                gamever = StrToGameVersion( agv.as_string() );
            else
            {
                clog <<"<!>- MoveDB_XML_Parser::ReadAllMoves(): No game version attribute string was found! Guessing what game its for..\n";
                if( std::distance( datanchilds.begin(), datanchilds.end() ) > 1 ) //If has 2 is EoS
                    gamever = eGameVersion::EoS;
                else
                    gamever = eGameVersion::EoT;
            }
            return gamever;
        }

        /*
        */
        void HandleMoveEoS( xml_node & pn, uint32_t moveid, MoveDB & result1, MoveDB & result2 )
        {
            using namespace movesXML;
            bool breadData1 = false; //This is to alternate between data entry 1 and 2
            for( auto & cnode : pn.children() )
            {
                if( !m_bNoStrings && cnode.name() == NODE_Strings )
                    ReadStrings( cnode, moveid );
                else if( cnode.name() == NODE_Data )
                {
                    if( !breadData1 )
                    {
                        result1.push_back(ReadMoveData( cnode ));
                        breadData1 = true;
                    }
                    else
                    {
                        result2.push_back(ReadMoveData( cnode ));
                        breadData1 = false;
                    }
                }
            }
        }

        /*
        */
        void HandleMoveEoTD( xml_node & pn, uint32_t moveid, MoveDB & result1 )
        {
            using namespace movesXML;
            for( auto & cnode : pn.children() )
            {
                if( !m_bNoStrings && cnode.name() == NODE_Strings )
                    ReadStrings( cnode, moveid );
                else if( cnode.name() == NODE_Data )
                    result1.push_back(ReadMoveData( cnode ));
            }
        }

        /*
        */
        void ReadStrings( xml_node & pn, uint32_t moveid )
        {
            using namespace movesXML;
            for( auto & curnode : pn.children() )
            {
                eGameLanguages glang = StrToGameLang(curnode.name());
                if( glang != eGameLanguages::Invalid )
                {
                    //Parse multi-language strings
                    ReadLangStrings(curnode, glang, moveid);
                }
                else
                {
                    //If the game isn't multi-lingual, just parse the strings for english
                    clog<<"<!>- MoveDB_XML_Parser::ReadStrings() : Found a non language named node!\n";
                    ReadLangStrings(curnode, eGameLanguages::english, moveid);
                }
            }
        }

        /*
        */
        void ReadLangStrings( xml_node & langnode, eGameLanguages lang, uint32_t moveid )
        {
            using namespace movesXML;
            auto langstr = m_pgametext->GetStrings(lang);
            if( langstr == m_pgametext->end() )
            {
                clog<<"<!>- MoveDB_XML_Parser::ReadLangStrings(): Found strings for " <<GetGameLangName(lang) <<", but the language was not loaded for editing! Skipping!\n";
                return;
            }

            for( auto & curnode : langnode.children() )
            {
                if( curnode.name() == PROP_Name )
                {
                    string name = curnode.child_value();
                    name += "\\0"; //put back the \0
                    string * pstr = langstr->second.GetStringIfBlockExists(eStringBlocks::MvNames, moveid);
                    if(!pstr)
                        throw std::runtime_error("MoveDB_XML_Parser::ReadLangStrings(): Couldn't access the move name string block!");
                    *(pstr) = name;
                }
                else if( curnode.name() == PROP_Desc )
                {
                    string desc = curnode.child_value();
                    desc += "\\0"; //put back the \0
                    string * pstr = langstr->second.GetStringIfBlockExists(eStringBlocks::MvDesc, moveid);
                    if(!pstr)
                        throw std::runtime_error("MoveDB_XML_Parser::ReadLangStrings(): Couldn't access the move description string block!");
                    *(pstr) = desc;
                }
            }
        }

        /*
        */
        MoveData ReadMoveData( xml_node & pn )
        {
            using namespace movesXML;
            using namespace utils;
            MoveData mvdata;

            for( auto & cnode: pn.children() )
            {
                if( cnode.name() == PROP_BasePow ) 
                    parseHexaValToValue( cnode.child_value(), mvdata.basePower );
                else if( cnode.name() == PROP_Type )    
                    parseHexaValToValue( cnode.child_value(), mvdata.type      );
                else if( cnode.name() == PROP_Category )    
                    parseHexaValToValue( cnode.child_value(), mvdata.category  );
                else if( cnode.name() == PROP_Unk4 )    
                    parseHexaValToValue( cnode.child_value(), mvdata.unk4      );
                else if( cnode.name() == PROP_Unk5 )    
                    parseHexaValToValue( cnode.child_value(), mvdata.unk5      );
                else if( cnode.name() == PROP_BasePP )    
                    parseHexaValToValue( cnode.child_value(), mvdata.basePP    );
                else if( cnode.name() == PROP_Unk6 )    
                    parseHexaValToValue( cnode.child_value(), mvdata.unk6      );
                else if( cnode.name() == PROP_Unk7 )    
                    parseHexaValToValue( cnode.child_value(), mvdata.unk7      );
                else if( cnode.name() == PROP_Accuracy )    
                    parseHexaValToValue( cnode.child_value(), mvdata.accuracy  );
                else if( cnode.name() == PROP_Unk9 )    
                    parseHexaValToValue( cnode.child_value(), mvdata.unk9      );
                else if( cnode.name() == PROP_Unk10 )    
                    parseHexaValToValue( cnode.child_value(), mvdata.unk10     );
                else if( cnode.name() == PROP_Unk11 )    
                    parseHexaValToValue( cnode.child_value(), mvdata.unk11     );
                else if( cnode.name() == PROP_Unk12 )    
                    parseHexaValToValue( cnode.child_value(), mvdata.unk12     );
                else if( cnode.name() == PROP_Unk13 )    
                    parseHexaValToValue( cnode.child_value(), mvdata.unk13     );
                else if( cnode.name() == PROP_Unk14 )    
                    parseHexaValToValue( cnode.child_value(), mvdata.unk14     );
                else if( cnode.name() == PROP_Unk15 )    
                    parseHexaValToValue( cnode.child_value(), mvdata.unk15     );
                else if( cnode.name() == PROP_Unk16 )    
                    parseHexaValToValue( cnode.child_value(), mvdata.unk16     );
                else if( cnode.name() == PROP_Unk17 )    
                    parseHexaValToValue( cnode.child_value(), mvdata.unk17     );
                else if( cnode.name() == PROP_Unk18 )    
                    parseHexaValToValue( cnode.child_value(), mvdata.unk18     );
                else if( cnode.name() == PROP_MoveID )    
                    parseHexaValToValue( cnode.child_value(), mvdata.moveID    );
                else if( cnode.name() == PROP_Unk19 )    
                    parseHexaValToValue( cnode.child_value(), mvdata.unk19     );
                }
            return move( mvdata );
        }

    private:
        MoveDB      & m_out1;
        MoveDB      * m_pout2;
        GameText    * m_pgametext;
        bool          m_bNoStrings;
        bool          m_bParseMoveId;
    };

//=================================================================================
//  Functions
//=================================================================================
    /**********************************************************************
        Export move data to XML files.
    **********************************************************************/
    void      ExportMovesToXML     ( const MoveDB                            & src1,
                                     const MoveDB                            * src2,
                                     const GameText                          * gtext,
                                     const std::string                       & destdir )
    {
        MoveDB_XMLWriter(src1, src2, gtext).Write(destdir);
    }

    /**********************************************************************
        Import move data from xml files.
    **********************************************************************/
    void      ImportMovesFromXML   ( const std::string                  & srcdir, 
                                     MoveDB                             & out_mvdb1,
                                     MoveDB                             * out_mvdb2,
                                     GameText                           * gtext )
    {
        MoveDB_XMLParser(out_mvdb1, out_mvdb2, gtext).Parse(srcdir);
    }



};};