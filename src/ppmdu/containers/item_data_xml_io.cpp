#include "item_data.hpp"
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
#include <Poco/DirectoryIterator.h>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <cassert>

using namespace std;
using namespace pugi;
using namespace pugixmlutils;
using namespace utils;

namespace pmd2 { namespace stats 
{
//=================================================================================
//  Constant
//=================================================================================
    namespace itemxml
    {
        const string ROOT_Item        = "Item";

        const string NODE_Strings     = "Strings";
        const string PROP_Name        = "Name";
        const string CMT_Name         = "The name of the item.";
        const string PROP_ShortDesc   = "ShortDescription";
        const string CMT_ShortDesc    = "The short 1 line description that appears in the little bar while browsing items.";
        const string PROP_LongDesc    = "LongDescription";
        const string CMT_LongDesc     = "The long description that appears in the full item info menu.";

        const string NODE_Data        = "Data";
        const string CMT_Data         = "The data for each items, see http://projectpokemon.org/wiki/Pmd2_item_p.bin for reference.";
        const string PROP_BuyPrice    = "BuyPrice";
        const string PROP_SellPrice   = "SellPrice";
        const string PROP_Category    = "Category";
        const string PROP_SpriteID    = "SpriteID";
        const string PROP_ItemID      = "ItemID";
        const string PROP_Param1      = "Param1";
        const string PROP_Param2      = "Param2";
        const string PROP_Param3      = "Param3";
        const string PROP_Unk1        = "Unk1";
        const string PROP_Unk2        = "Unk2";
        const string PROP_Unk3        = "Unk3";
        const string PROP_Unk4        = "Unk4";

        const string NODE_ExData      = "ExclusiveData";
        const string CMT_ExData       = "Data for exclusive items. See http://projectpokemon.org/wiki/Pmd2_item_s_p.bin for reference.";
        const string CMT_Type         = "The type is a value from 0 to 10(0xA) that indicates the kind of exclusive item it is. http://projectpokemon.org/wiki/Pmd2_item_s_p.bin#Item_Type";
        const string PROP_Type        = "Type";
        const string CMT_Param        = "The parameter contains an integer value that is used differently depending on the type. http://projectpokemon.org/wiki/Pmd2_item_s_p.bin#Item_Parameter";
        const string PROP_Param       = "Parameter";

        const string CMT_EoS          = "Pokemon Mystery Dungeon: Explorers of Sky item data";
        const string CMT_EoTD         = "Pokemon Mystery Dungeon: Explorers of Time/Darkness item data";
        const string CMT_EoTDData     = "Data exclusive to Pokemon Mystery Dungeon: Explorers of Time/Darkness items";

        const string ATTR_GameVer     = "GameVersion";
        const string ATTR_GameVerEoS  = "EoS";
        const string ATTR_GameVerEoTD = "EoTD";
    };

//=================================================================================
//  Classes
//=================================================================================

    /**********************************************************************
        ItemXMLWriter
    **********************************************************************/
    class ItemXMLWriter
    {
    public:
        ItemXMLWriter( const ItemsDB &src, const GameText * pgtext = nullptr )
            :m_items(src), m_pgametext(pgtext), m_bNoStrings(false)
        {}


        void Write( const string & destdir )
        {
            if( !m_pgametext || !m_pgametext->AreStringsLoaded() )
            {
                clog<<"<!>- ItemXMLWriter::Write(): No string loaded! Ignoring text! (Names, descriptions, etc..)\n";
                m_bNoStrings = true;
            }

            using namespace itemxml;
            if( !utils::isFolder(destdir) )
            {
                stringstream sstr;
                sstr << "ItemXMLWriter::Write(): Invalid directory \"" <<destdir <<"\"!";
                throw runtime_error( sstr.str() );
            }
            
            const string dirprefix = utils::TryAppendSlash( destdir );
            stringstream fname;

            for( size_t cntitem = 0; cntitem < m_items.size(); ++cntitem )
            {
                fname.str(string()); //Purge stringstream
                xml_document doc;
                xml_node     itemdata = doc.append_child( ROOT_Item.c_str() );
                bool         isEoS    = m_items[cntitem].Get_EoTD_ItemData() == nullptr;

                if( isEoS )
                {
                    AppendAttribute( itemdata, ATTR_GameVer, ATTR_GameVerEoS );
                    WriteCommentNode( itemdata, CMT_EoS );
                }
                else
                {
                    AppendAttribute( itemdata, ATTR_GameVer, ATTR_GameVerEoTD );
                    WriteCommentNode( itemdata, CMT_EoTD );
                }

                if(!m_bNoStrings)
                    WriteStrings( itemdata, cntitem );
                WriteCommentNode( itemdata, CMT_Data );
                _WriteItemData( itemdata, m_items[cntitem] );

                MakeFilename( fname, dirprefix, cntitem );

                if( ! doc.save_file( fname.str().c_str() ) )
                {
                    stringstream strerr;
                    strerr << "ItemXMLWriter::Write(): Pugixml couldn't write file \"" <<fname.str() <<"\"!";
                    throw runtime_error(strerr.str());
                }
            }

        }

    private:

        stringstream & MakeFilename( stringstream & out_fname, const string & outpathpre, unsigned int cntitem )
        {
            const string * pfstr = nullptr;
            if( !m_bNoStrings && (pfstr = m_pgametext->begin()->second.GetStringInBlock( eStringBlocks::ItemNames, cntitem )) )
            {
                out_fname <<outpathpre <<setw(4) <<setfill('0') <<cntitem <<"_" 
                          <<PrepareItemFName(*pfstr, m_pgametext->begin()->first) <<".xml";
            }
            else
                out_fname <<outpathpre <<setw(4) <<setfill('0') <<cntitem <<".xml";

            return out_fname;
        }


        //Length of the conversion buffer
        //static const int                          CBuffSZ = (sizeof(int)*8+1);
        ////Conversion buffers. Used for faster value conversion. (Don't need all the extra locale stuff from stringstream, as all values are raw data)
        //array<char,CBuffSZ>                      m_convBuff;
        //array<char,CBuffSZ>                      m_secConvbuffer;

        ////Returns a pointer to the buffer passed as argument
        //template<class T>
        //    inline const char * FastTurnIntToHexCStr( T value )
        //{
        //    sprintf_s( m_convBuff.data(), CBuffSZ, "0x%s", itoa( value, m_secConvbuffer.data(), 16 ) );
        //    return m_convBuff.data();
        //}

        template<class T>
            inline string TurnIntToHexStr( T value )
        {
            stringstream sstr;
            sstr << "0x" <<hex <<uppercase <<value;
            return sstr.str();
        }

        template<>
            inline string TurnIntToHexStr( uint8_t value )
        {
            stringstream sstr;
            sstr << "0x" <<hex <<uppercase <<static_cast<unsigned short>(value);
            return sstr.str();
        }

        inline string PrepareItemFName( const string & name, eGameLanguages glang )
        {
            return utils::CleanFilename( name.substr( 0, name.find("\\0",0 ) ), std::locale( *m_pgametext->GetLocaleString(glang)) ); //Remove ending "\0" and remove illegal characters for filesystem
        }

        inline void WriteStringNode( xml_node & strnode, const string & nodename, const string * value )
        {
            if( value )
                WriteNodeWithValue( strnode, nodename, utils::StrRemoveAfter( *value, "\\0" ) ); //remove ending \0
        }

        void WriteStrings( xml_node & in, unsigned int cntitem )
        {
            using namespace itemxml;
            WriteCommentNode( in, "In-game text" );
            xml_node strnode = in.append_child( NODE_Strings.c_str() );

            //Add all loaded languages
            for( const auto & alang : *m_pgametext )
            {
                xml_node langnode = strnode.append_child( GetGameLangName(alang.first).c_str() );
                //Write Name
                WriteStringNode( langnode, PROP_Name,      alang.second.GetStringInBlock(eStringBlocks::ItemNames, cntitem) );
                //Write Description
                WriteStringNode( langnode, PROP_ShortDesc, alang.second.GetStringInBlock(eStringBlocks::ItemDescS, cntitem) );
                //Write Description
                WriteStringNode( langnode, PROP_LongDesc,  alang.second.GetStringInBlock(eStringBlocks::ItemDescL, cntitem) );
            }
        }

        //void _WriteStrings( xml_node & pn, uint16_t cntitem )
        //{
        //    using namespace itemxml;
        //    xml_node strnode = pn.append_child( NODE_Strings.c_str() );
        //    WriteNodeWithValue( strnode, PROP_Name,      utils::StrRemoveAfter( name,      "\\0" ).c_str() ); //remove trailling \0
        //    WriteNodeWithValue( strnode, PROP_ShortDesc, utils::StrRemoveAfter( shortdesc, "\\0" ).c_str() ); //remove trailling \0
        //    WriteNodeWithValue( strnode, PROP_LongDesc,  utils::StrRemoveAfter( longdesc,  "\\0" ).c_str() ); //remove trailling \0
        //}

        void _WriteItemData( xml_node & pn, const stats::itemdata & item )
        {
            using namespace itemxml;
            xml_node datnode = pn.append_child( NODE_Data.c_str() );

            WriteNodeWithValue( datnode, PROP_BuyPrice,  item.buyPrice  );
            WriteNodeWithValue( datnode, PROP_SellPrice, item.sellPrice );
            WriteNodeWithValue( datnode, PROP_Category,  item.category  );
            WriteNodeWithValue( datnode, PROP_SpriteID,  item.spriteID  );
            WriteNodeWithValue( datnode, PROP_ItemID,    item.itemID    );
            WriteNodeWithValue( datnode, PROP_Param1,    TurnIntToHexStr(item.param1)    );
            WriteNodeWithValue( datnode, PROP_Param2,    TurnIntToHexStr(item.param2)    );
            WriteNodeWithValue( datnode, PROP_Param3,    TurnIntToHexStr(item.param3)    );
            WriteNodeWithValue( datnode, PROP_Unk1,      TurnIntToHexStr(item.unk1  )    );
            WriteNodeWithValue( datnode, PROP_Unk2,      TurnIntToHexStr(item.unk2  )    );
            WriteNodeWithValue( datnode, PROP_Unk3,      TurnIntToHexStr(item.unk3  )    );
            WriteNodeWithValue( datnode, PROP_Unk4,      TurnIntToHexStr(item.unk4  )    );

            const itemdata_EoTD * pEoTD = item.Get_EoTD_ItemData();
            if( pEoTD != nullptr )
            {
                WriteCommentNode( pn, CMT_EoTDData );
                assert(false);
            }

            const exclusiveitemdata * pEx = item.GetExclusiveItemData();
            if( pEx != nullptr )
            {
                WriteCommentNode( pn, CMT_ExData );
                xml_node exnode = pn.append_child( NODE_ExData.c_str() );
                WriteNodeWithValue( exnode, PROP_Type,  pEx->type  );
                WriteNodeWithValue( exnode, PROP_Param, TurnIntToHexStr(pEx->param) );
            }
        }

    private:
        const ItemsDB                & m_items;
        const GameText               * m_pgametext;
        bool                           m_bNoStrings;
    };

    /**********************************************************************
        ItemXMLParser
    **********************************************************************/
    class ItemXMLParser
    {
    public:
        ItemXMLParser( GameText* pgtext )
            :m_pgametext(pgtext), m_bNoStrings(false)
        {}

        ItemsDB Parse( const string & srcdir )
        {
            if( !m_pgametext || m_pgametext->AreStringsLoaded() )
            {
                clog << "<!>- ItemXMLParser::Parse(): No strings loaded, ignoring everything text related! (Names, descriptions, etc..)\n";
                m_bNoStrings = true;
            }

            ItemsDB mydb;
            try
            {
                Poco::DirectoryIterator itDirEnd;
                vector<string>          filelst;

                for( Poco::DirectoryIterator itDir(srcdir); itDir != itDirEnd; ++itDir )
                {
                    if( itDir->isFile() && Poco::Path(itDir.path()).getExtension() == "xml" )
                        filelst.push_back( (itDir.path().absolute().toString()) );
                }

                return std::move( _ParseAllItems(filelst) );
            }
            catch( exception & e )
            {
                stringstream sstr;
                sstr <<"ItemXMLParser::Parse(): Got Exception while parsing XML from directory \"" <<srcdir <<"\" : " << e.what();
                throw runtime_error( sstr.str() );
            }
        }

    private:

        ItemsDB _ParseAllItems( const vector<string> filelst )
        {
            using namespace itemxml;

            //Check if we
            xml_document     doccheck;
            xml_parse_result loadres = doccheck.load_file(filelst.front().c_str());
            if( ! loadres )
            {
                stringstream sstr;
                sstr <<"Can't load XML document \"" <<filelst.front() <<"\"! Pugixml returned an error : \"" << loadres.description() <<"\"";
                throw std::runtime_error(sstr.str());
            }

            ItemsDB resitems;

            resitems.resize(filelst.size());
            auto ititem = resitems.begin();

            for( auto & item : filelst )
            {
                xml_document     doc;
                xml_parse_result loadres = doc.load_file( item.c_str() );

                _ParseItem( doc.first_child(), *ititem, item );

                ++ititem;
            }

            return std::move( resitems );
        }

        void ReadStrings( xml_node & strnode, uint32_t itemID )
        {
            using namespace itemxml;
            for( auto & curnode : strnode.children() )
            {
                eGameLanguages glang = StrToGameLang(curnode.name());
                if( glang != eGameLanguages::Invalid )
                {
                    //Parse multi-language strings
                    ReadLangStrings(curnode, glang, itemID);
                }
                else
                {
                    //If the game isn't multi-lingual, just parse the strings for english
                    clog<<"<!>- ItemXMLParser::ReadStrings() : Found a non language named node!\n";
                    ReadLangStrings(curnode, eGameLanguages::english, itemID);
                }
            }
        }

        void ReadLangStrings( xml_node & langnode, eGameLanguages lang, uint32_t itemID )
        {
            StringAccessor * plangstr = m_pgametext->GetStrings(lang);
            if( !plangstr )
            {
                clog<<"<!>- ItemXMLParser::ReadLangStrings(): Found strings for " <<GetGameLangName(lang) <<", but the language was not loaded for editing! Skipping!\n";
                return;
            }

            using namespace itemxml;
            for( auto & curnode : langnode.children() )
            {
                if( curnode.name() == PROP_Name )
                {
                    string itemname = curnode.child_value();
                    itemname += "\\0"; //put back the \0
                    *(plangstr->GetStringInBlock( eStringBlocks::ItemNames, itemID )) = itemname;
                }
                else if( curnode.name() == PROP_ShortDesc )
                {
                    string itemdescsh = curnode.child_value();
                    itemdescsh += "\\0"; //put back the \0
                    *(plangstr->GetStringInBlock( eStringBlocks::ItemDescS, itemID )) = itemdescsh;
                }
                else if( curnode.name() == PROP_LongDesc )
                {
                    string itemdescl = curnode.child_value();
                    itemdescl += "\\0"; //put back the \0
                    *(plangstr->GetStringInBlock( eStringBlocks::ItemDescL, itemID )) = itemdescl;
                }
            }
        }

        void _ParseItem( const pugi::xml_node & itemnode, stats::itemdata & item, const string & itemname )
        {
            using namespace itemxml;
            //Check the game version of the data
            xml_attribute gv = itemnode.attribute( ATTR_GameVer.c_str() );
            const string  gvs= gv.as_string(); 

            if( !gvs.empty() ) //If the attribute exists
            {
                if( gvs == ATTR_GameVerEoS )
                {
                    _ParseEoSData( itemnode, item );
                }
                else if( gvs == ATTR_GameVerEoTD )
                {
                    _ParseEoTDData( itemnode, item );
                }
                else
                {
                    stringstream sstrerr;
                    sstrerr << "ItemXMLParser::_ParseData(): Invalid GameVersion string \"" <<gvs <<"\", for item " <<itemname <<" !";
                    throw runtime_error(sstrerr.str());
                }
            }
            else
            {
                stringstream sstr;
                sstr << "ItemXMLParser::_ParseData(): Game version attribute for item " <<itemname <<" is missing!";
                throw runtime_error(sstr.str());
            }
        }

        void _ParseEoSData( const pugi::xml_node & itemnode, stats::itemdata & item )
        {
            using namespace itemxml;
            //xml_node        strnode;

            for( auto & curnode : itemnode.children() )
            {
                //if( curnode.name() == NODE_Strings )
                //{
                //    strnode = curnode; //Mark the position where the strings are
                //}
                //else 
                if( curnode.name() == NODE_Data )
                {
                    for( auto & itemprop : curnode.children() )
                    {
                        _ParsePropertiesCommon   ( itemprop, item );
                    }
                }
                else if( curnode.name() == NODE_ExData )
                {
                    for( auto & itemprop : curnode.children() )
                    {
                        _ParsePropertiesExclusive( itemprop, item );
                    }
                }
            }

            if(m_bNoStrings)
                return;
            //Once we got our itemID, do the strings!
            for( auto & curnode : itemnode.children() )
            {
                if( curnode.name() == NODE_Strings )
                    ReadStrings( curnode, item.itemID );
            }
        }

        void _ParsePropertiesCommon( const pugi::xml_node & itemprop, stats::itemdata & item )
        {
            using namespace itemxml;

            if( itemprop.name() == PROP_BuyPrice )
                utils::parseHexaValToValue( itemprop.child_value(), item.buyPrice );
            else if( itemprop.name() == PROP_SellPrice )
                utils::parseHexaValToValue( itemprop.child_value(), item.sellPrice );
            else if( itemprop.name() == PROP_Category )
                utils::parseHexaValToValue( itemprop.child_value(), item.category );
            else if( itemprop.name() == PROP_SpriteID )
                utils::parseHexaValToValue( itemprop.child_value(), item.spriteID );
            else if( itemprop.name() == PROP_ItemID )
                utils::parseHexaValToValue( itemprop.child_value(), item.itemID );
            else if( itemprop.name() == PROP_Param1 )
                utils::parseHexaValToValue( itemprop.child_value(), item.param1 );
            else if( itemprop.name() == PROP_Param2 )
                utils::parseHexaValToValue( itemprop.child_value(), item.param2 );
            else if( itemprop.name() == PROP_Param3 )
                utils::parseHexaValToValue( itemprop.child_value(), item.param3 );
            else if( itemprop.name() == PROP_Unk1 )
                utils::parseHexaValToValue( itemprop.child_value(), item.unk1 );
            else if( itemprop.name() == PROP_Unk2 )
                utils::parseHexaValToValue( itemprop.child_value(), item.unk2 );
            else if( itemprop.name() == PROP_Unk3 )
                utils::parseHexaValToValue( itemprop.child_value(), item.unk3 );
            else if( itemprop.name() == PROP_Unk4 )
                utils::parseHexaValToValue( itemprop.child_value(), item.unk4 );
        }

        void _ParsePropertiesExclusive( const pugi::xml_node & itemprop, stats::itemdata & item )
        {
            using namespace itemxml;

            if( itemprop.name() == PROP_Type )
            {
                stats::exclusiveitemdata * pEx = item.GetExclusiveItemData();
                if( pEx == nullptr )
                    pEx = item.MakeExclusiveData();
                utils::parseHexaValToValue( itemprop.child_value(), pEx->type );
            }
            else if( itemprop.name() == PROP_Param )
            {
                stats::exclusiveitemdata * pEx = item.GetExclusiveItemData();
                if( pEx == nullptr )
                    pEx = item.MakeExclusiveData();
                utils::parseHexaValToValue( itemprop.child_value(), pEx->param );
            }
        }

        void _ParseEoTDData( const pugi::xml_node & itemnode, stats::itemdata & item )
        {
            //#TODO: Finish _ParseEoTDData !
            cerr <<"\nExplorers of Time and Darkness data parsing not implemented yet!!!\n";
            assert(false);

            using namespace itemxml;
            xml_node        strnode;

            for( auto & curnode : itemnode.children() )
            {
                if( curnode.name() == NODE_Strings )
                {
                    strnode = curnode;
                }
                else if( curnode.name() == NODE_Data )
                {
                    assert(false);
                }
            }

            //
            ReadStrings( strnode, item.itemID );
        }

    private:
        //vector<string>::iterator m_begitemnames;
        //vector<string>::iterator m_enditemnames;
        //vector<string>::iterator m_begitemsdesc;
        //vector<string>::iterator m_enditemsdesc;
        //vector<string>::iterator m_begitemldesc;
        //vector<string>::iterator m_enditemldesc;

        GameText * m_pgametext;
        bool       m_bNoStrings;
    };

//=================================================================================
//  Functions
//=================================================================================

    /*
        Export item data to XML files.
    */
    //void      ExportItemsToXML     ( const ItemsDB                           & srcitems,
    //                                 std::vector<std::string>::const_iterator  itbegitemnames,
    //                                 std::vector<std::string>::const_iterator  itenditemnames,
    //                                 std::vector<std::string>::const_iterator  itbegitemdesc,
    //                                 std::vector<std::string>::const_iterator  itenditemdesc,
    //                                 std::vector<std::string>::const_iterator  itbegitemlongdesc,
    //                                 std::vector<std::string>::const_iterator  itenditemlongdesc,
    //                                 const std::string                       & destdir )
    //{
    //    ItemXMLWriter( srcitems, 
    //                   itbegitemnames,    itenditemnames, 
    //                   itbegitemdesc,     itenditemdesc,
    //                   itbegitemlongdesc, itenditemlongdesc ).Write(destdir);
    //}
    void      ExportItemsToXML     ( const ItemsDB                           & srcitems,
                                     const GameText                          * pgametext,
                                     const std::string                       & destdir )
    {
        ItemXMLWriter(srcitems, pgametext).Write(destdir);
    }


    /*
        Import item data from xml files.
    */
    //ItemsDB    ImportItemsFromXML   ( const std::string                  & srcdir, 
    //                                 std::vector<std::string>::iterator  itbegitemnames,
    //                                 std::vector<std::string>::iterator  itenditemnames,
    //                                 std::vector<std::string>::iterator  itbegitemdesc,
    //                                 std::vector<std::string>::iterator  itenditemdesc,
    //                                 std::vector<std::string>::iterator  itbegitemlongdesc,
    //                                 std::vector<std::string>::iterator  itenditemlongdesc )
    //{
    //    return std::move( ItemXMLParser( itbegitemnames,    itenditemnames, 
    //                                     itbegitemdesc,     itenditemdesc,
    //                                     itbegitemlongdesc, itenditemlongdesc ).Parse(srcdir) );
    //}


    ItemsDB   ImportItemsFromXML   ( const std::string                  & srcdir, 
                                     GameText                           * pgametext )
    {
        return std::move( ItemXMLParser(pgametext).Parse(srcdir) );
    }

};};