#include "move_data.hpp"
#include <ppmdu/utils/parse_utils.hpp>
#include <ppmdu/utils/pugixml_utils.hpp>
#include <ppmdu/utils/library_wide.hpp>
#include <ppmdu/utils/utility.hpp>
#include <pugixml.hpp>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <locale>
#include <vector>
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

        static const string ROOT_Move    = "Move";

        static const string NODE_Strings = "Strings";
        static const string PROP_Name    = "Name";
        static const string PROP_Desc    = "Description";

        static const string NODE_Data    = "Data";
        static const string PROP_BasePow = "BasePower";
        static const string PROP_Type    = "Type";
        static const string PROP_Category= "Category";
        static const string PROP_Unk4    = "Unk4";
        static const string PROP_Unk5    = "Unk5";
        static const string PROP_BasePP  = "BasePP";
        static const string PROP_Unk6    = "Unk6";
        static const string PROP_Unk7    = "Unk7";
        static const string PROP_Accuracy= "Accuracy";
        static const string PROP_Unk9    = "Unk9";
        static const string PROP_Unk10   = "Unk10";
        static const string PROP_Unk11   = "Unk11";
        static const string PROP_Unk12   = "Unk12";
        static const string PROP_Unk13   = "Unk13";
        static const string PROP_Unk14   = "Unk14";
        static const string PROP_Unk15   = "Unk15";
        static const string PROP_Unk16   = "Unk16";
        static const string PROP_Unk17   = "Unk17";
        static const string PROP_Unk18   = "Unk18";
        static const string PROP_MoveID  = "MoveID";
        static const string PROP_Unk19   = "Unk19";
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
        MoveDB_XMLWriter( const MoveDB                   & src1,
                          const MoveDB                   & src2,
                          vector<string>::const_iterator  itbegnames,
                          vector<string>::const_iterator  itbegdesc )
            :m_src1(src1), m_src2(src2), m_itnames(itbegnames), m_itdescs(itbegdesc)
        {}

        void Write( const std::string & destdir )
        {
            if( !utils::isFolder(destdir) )
            {
                stringstream sstr;
                sstr << "Invalid directory \"" <<destdir <<"\"!";
                throw runtime_error( sstr.str() );
            }

            if( !m_src2.empty() )
            {
                WriteEoS( destdir );
            }
            else
            {
                WriteEoTD( destdir );
            }
        }

    private:

        void WriteEoS( const std::string & destdir )
        {
            using namespace movesXML;
            auto         itcurname = m_itnames;
            auto         itcurdesc = m_itdescs;
            const string dirprefix = utils::AppendTraillingSlashIfNotThere( destdir );
            stringstream fname;

            if( m_src1.size() != m_src2.size() )
                throw runtime_error("Size mismatch between the two move data lists! One list of moves is longer than the other!");

            for( unsigned int i = 0; i < m_src1.size(); ++i, ++itcurname, ++itcurdesc )
            {
                fname.str(string());
                xml_document doc;
                xml_node     movedata = doc.append_child( ROOT_Move.c_str() );
                WriteCommentNode( movedata, "Pokemon Mystery Dungeon: Explorers of Sky move data" );
                WriteCommentNode( movedata, "In-game text" );
                WriteStrings( movedata, *itcurname, *itcurdesc );
                WriteCommentNode( movedata, "Move data from waza_p.bin" );
                WriteMove( movedata, m_src1[i] );
                WriteCommentNode( movedata, "Move data from waza_p2.bin" );
                WriteMove( movedata, m_src2[i] );

                fname <<dirprefix <<setw(4) <<setfill('0') <<dec <<i <<"_" << PrepareMvNameFName(*itcurname) <<".xml";

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
            auto         itcurname = m_itnames;
            auto         itcurdesc = m_itdescs;
            const string dirprefix = utils::AppendTraillingSlashIfNotThere( destdir );
            stringstream fname;

            for( unsigned int i = 0; i < m_src1.size(); ++i, ++itcurname, ++itcurdesc )
            {
                fname.str(string());
                xml_document doc;
                xml_node     movedata = doc.append_child( ROOT_Move.c_str() );
                WriteCommentNode( movedata, "Pokemon Mystery Dungeon: Explorers of Time/Darkness move data" );
                WriteCommentNode( movedata, "In-game text" );
                WriteStrings( movedata, *itcurname, *itcurdesc );
                WriteCommentNode( movedata, "Move data from waza_p.bin" );
                WriteMove   ( movedata, m_src1[i] );

                fname <<dirprefix <<setw(4) <<setfill('0') <<dec <<i <<"_" << PrepareMvNameFName(*itcurname) <<".xml";

                if( ! doc.save_file( fname.str().c_str() ) )
                {
                    stringstream strerr;
                    strerr << "Pugixml couldn't write file \"" <<fname.str() <<"\"!";
                    throw runtime_error(strerr.str());
                }
            }
        }



        inline string PrepareMvNameFName( const string & name )
        {
            return utils::CleanFilename( name.substr( 0, name.find("\\0",0 ) ) ); //Remove ending "\0" and remove illegal characters for filesystem
        }

        void WriteStrings( xml_node & pn, const string & name, const string & desc )
        {
            using namespace movesXML;
            xml_node strnode = pn.append_child( NODE_Strings.c_str() );
            WriteNodeWithValue( strnode, PROP_Name, utils::StrRemoveAfter( name, "\\0" ).c_str() ); //remove trailling \0
            WriteNodeWithValue( strnode, PROP_Desc, utils::StrRemoveAfter( desc, "\\0" ).c_str() ); //remove trailling \0

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
        const MoveDB                   &m_src2;
        vector<string>::const_iterator  m_itnames;
        vector<string>::const_iterator  m_itdescs;
    };

    /**********************************************************************
        MoveDB_XMLParser
    **********************************************************************/
    class MoveDB_XMLParser
    {
    public:
        typedef pair<vector<string>::iterator,vector<string>::iterator> range_t;

        MoveDB_XMLParser( MoveDB & out_mdb1, MoveDB & out_mdb2, range_t mnit, range_t mdit )
            :m_out1(out_mdb1), m_out2(out_mdb2), m_moveNames(mnit), m_moveDescs(mdit)
        {}

        void Parse( const string & srcdir ) 
        {
            try
            {
                Poco::DirectoryIterator itDirEnd;
                vector<string>          filelst;

                for( Poco::DirectoryIterator itDir(srcdir); itDir != itDirEnd; ++itDir )
                {
                    if( itDir->isFile() && Poco::Path(itDir.path()).getExtension() == "xml" )
                        filelst.push_back( (itDir.path().absolute().toString()) );
                }

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

        void ReadAllMoves( const vector<string> & files )
        {
            using namespace movesXML;

            //Check first file whether the format is for EoS data or EoTD
            xml_document doccheck;
            xml_parse_result loadres = doccheck.load_file(files.front().c_str());
            if( ! loadres )
            {
                stringstream sstr;
                sstr <<"Can't load XML document \"" <<files.front() <<"\"! Pugixml returned an error : \"" << loadres.description() <<"\"";
                throw std::runtime_error(sstr.str());
            }


            auto chkchilds = doccheck.first_child().children(NODE_Data.c_str());
            bool has2data  = std::distance( chkchilds.begin(), chkchilds.end() ) > 1; //If has 2 is EoS

            uint32_t cnt =0;
            for( auto & child : chkchilds )
                ++cnt;

            MoveDB result1;
            MoveDB result2;
            auto   itNames    = m_moveNames.first;
            auto   itNamesEnd = m_moveNames.second;
            auto   itDesc     = m_moveDescs.first;
            auto   itDescEnd  = m_moveDescs.second;

            result1.reserve(files.size());

            if(has2data)
                result2.reserve(files.size());

            //Parse files
            for( auto & mv : files )
            {
                if( itNames == itNamesEnd || itDesc == itDescEnd )
                {
                    stringstream sstrerr;
                    sstrerr << "Reached end of move name or move description list before all move were parsed "
                            << "while parsing file \"" <<mv <<"\"";
                    throw runtime_error( sstrerr.str() );
                }

                string & strname = *itNames;
                string & strdesc  = *itDesc;

                xml_document doc;
                xml_parse_result loadres = doc.load_file(mv.c_str());
                if( ! loadres )
                {
                    stringstream sstr;
                    sstr <<"Can't load XML document \"" <<mv <<"\"! Pugixml returned an error : \"" << loadres.description() <<"\"";
                    throw std::runtime_error(sstr.str());
                }

                if( has2data )
                    HandleMoveEoS( doc.first_child(), strname, strdesc, result1, result2 );
                else
                    HandleMoveEoTD( doc.first_child(), strname, strdesc, result1 );

                ++itNames;
                ++itDesc;
            }

            m_out1 = move( result1 );
            m_out2 = move( result2 );
        }

        void HandleMoveEoS( xml_node & pn, string & name, string & desc, MoveDB & result1, MoveDB & result2 )
        {
            using namespace movesXML;
            bool breadData1 = false; //This is to alternate between data entry 1 and 2
            for( auto & cnode : pn.children() )
            {
                if( cnode.name() == NODE_Strings )
                    ReadStrings( cnode, name, desc );
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

        void HandleMoveEoTD( xml_node & pn, string & name, string & desc, MoveDB & result1 )
        {
            using namespace movesXML;
            for( auto & cnode : pn.children() )
            {
                if( cnode.name() == NODE_Strings )
                    ReadStrings( cnode, name, desc );
                else if( cnode.name() == NODE_Data )
                    result1.push_back(ReadMoveData( cnode ));
            }
        }

        //MoveData ReadMove( xml_node & pn, string & name, string & desc )
        //{
        //    using namespace movesXML;
        //    MoveData md;

        //    for( auto & curnode : pn.children() )
        //    {
        //        if( curnode.name() == NODE_Strings )
        //        {
        //            ReadStrings( curnode, name, desc );
        //           
        //        }
        //        else if( curnode.name() == NODE_Data )
        //        {
        //            md = ReadMoveData( curnode );
        //        }
        //    }
        //    return move(md);
        //}

        void ReadStrings( xml_node & pn, string & name, string & desc )
        {
            using namespace movesXML;
            for( auto & curnode : pn.children() )
            {
                if( curnode.name() == PROP_Name )
                {
                    name = curnode.child_value();
                    name += "\\0"; //Add the trailing 0
                }
                else if( curnode.name() == PROP_Desc )
                {
                    desc = curnode.child_value();
                    desc += "\\0"; //Add the trailing 0
                }
            }
        }

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
        MoveDB & m_out1;
        MoveDB & m_out2;
        range_t  m_moveNames;
        range_t  m_moveDescs;
    };

//=================================================================================
//  Functions
//=================================================================================
    /**********************************************************************
        Export move data to XML files.
    **********************************************************************/
    void      ExportMovesToXML     ( const MoveDB                            & src1,
                                     const MoveDB                            & src2,
                                     std::vector<std::string>::const_iterator  itbegnames,
                                     std::vector<std::string>::const_iterator  itbegdesc,
                                     const std::string                       & destdir )
    {
        MoveDB_XMLWriter(src1,src2,itbegnames,itbegdesc).Write(destdir);
    }

    /**********************************************************************
        Import move data from xml files.
    **********************************************************************/
    void      ImportMovesFromXML   ( const std::string                  & srcdir, 
                                     MoveDB                             & out_mvdb1,
                                     MoveDB                             & out_mvdb2,
                                     std::vector<std::string>::iterator   itbegnames,
                                     std::vector<std::string>::iterator   itendnames,
                                     std::vector<std::string>::iterator   itbegdesc,
                                     std::vector<std::string>::iterator   itenddesc )
    {
        
        MoveDB_XMLParser( out_mvdb1, out_mvdb2, make_pair( itbegnames, itendnames ), make_pair( itbegdesc, itenddesc ) ).Parse( srcdir );
    }

};};