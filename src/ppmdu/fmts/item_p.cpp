#include "item_p.hpp"
#include <ppmdu/fmts/sir0.hpp>
#include <utils/poco_wrapper.hpp>
#include <utils/utility.hpp>
#include <sstream>
#include <iostream>
#include <cassert>

using namespace filetypes;
using namespace std;
using namespace utils;

namespace pmd2 {namespace filetypes 
{

    const uint8_t PaddingByte                 = 0xAA;
    const uint32_t ExclusiveItemBaseDataIndex = 444;

    /*
        EoSItemDataParser
    */
    class EoSItemDataParser
    {
    public:
        EoSItemDataParser( const std::string & pathBalanceDir )
            :m_pathBalanceDir(pathBalanceDir)
        {
        }

        operator stats::ItemsDB()
        {
            stats::ItemsDB result;

            //Figure out if a file is missing
            stringstream sstritem_p;
            stringstream sstritem_s;
            sstritem_p << utils::TryAppendSlash(m_pathBalanceDir) << ItemData_FName;
            sstritem_s << utils::TryAppendSlash(m_pathBalanceDir) << ExclusiveItemData_FName;
            const string item_p = sstritem_p.str();
            const string item_s = sstritem_s.str();

            if( utils::isFile(item_p) )
            {
                if( utils::isFile(item_s) )
                {
                    ParseItem_p  ( item_p, result );
                    ParseItem_s_p( item_s, result );
                }
                else
                {
                    stringstream sstrerr;
                    sstrerr << "EoSItemDataParser::operator stats::ItemsDB(): " <<ExclusiveItemData_FName <<" is missing!";
                    string strerr = sstrerr.str();
                    clog << strerr <<"\n";
                    throw runtime_error(sstrerr.str());
                }
            }
            else
            {
                ostringstream sstrerr;
                sstrerr << "EoSItemDataParser::operator stats::ItemsDB(): Couldn't find the \"" << item_p <<"\" file!";
                string strerr = sstrerr.str();
                clog << strerr <<"\n";
                throw runtime_error(strerr);
            }
            return std::move(result);
        }

    private:

        void ParseItem_p( const string & path, stats::ItemsDB & itemdat )
        {
            vector<uint8_t> data = utils::io::ReadFileToByteVector( path );

            //Parse header
            sir0_header hdr;
            hdr.ReadFromContainer(data.begin());

            const uint32_t NbEntries = (hdr.ptrPtrOffsetLst - hdr.subheaderptr) / stats::ItemDataLen_EoS;
            auto           itCur     = (data.begin() + hdr.subheaderptr);
            itemdat.resize(NbEntries);

            for( unsigned int cnt = 0; cnt < NbEntries; ++cnt )
                ParseItem_p_entry( itCur, itemdat[cnt] );
        }

        void ParseItem_p_entry( vector<uint8_t>::const_iterator & itread, stats::itemdata & item )
        {
            itread = ReadIntFromByteContainer( item.buyPrice,   itread );
            itread = ReadIntFromByteContainer( item.sellPrice,  itread );
            itread = ReadIntFromByteContainer( item.category,   itread );
            itread = ReadIntFromByteContainer( item.spriteID,   itread );
            itread = ReadIntFromByteContainer( item.itemID,     itread );
            itread = ReadIntFromByteContainer( item.param1,     itread );
            itread = ReadIntFromByteContainer( item.param2,     itread );
            itread = ReadIntFromByteContainer( item.param3,     itread );
            itread = ReadIntFromByteContainer( item.unk1,       itread );
            itread = ReadIntFromByteContainer( item.unk2,       itread );
            itread = ReadIntFromByteContainer( item.unk3,       itread );
            itread = ReadIntFromByteContainer( item.unk4,       itread );
        }

        void ParseItem_s_p( const string & path, stats::ItemsDB & itemdat )
        {
            vector<uint8_t> data = utils::io::ReadFileToByteVector( path );

            //Parse header
            sir0_header hdr;
            hdr.ReadFromContainer(data.begin());

            const uint32_t NbEntries = (hdr.ptrPtrOffsetLst - hdr.subheaderptr) / stats::ExclusiveItemDataLen; //Nb of entries in the exclusive item data file
            auto           itdatbeg  = data.begin() + hdr.subheaderptr;

            size_t cntitemID = ExclusiveItemBaseDataIndex; // cntitemID = Counter for the actual item id in the database (Exlusive items begin at a specific index)
            
            if( (itemdat.size() - ExclusiveItemBaseDataIndex) != NbEntries )
            {
                clog << "EoSItemDataParser::ParseItem_s_p(): Warning! Incoherences between item_s_p.bin and item_p.bin! One or the other contains too many or too little entries!\n";
            }
            
            //
            for( size_t cntExEntry = 0; cntExEntry < NbEntries && cntitemID < itemdat.size(); ++cntExEntry, ++cntitemID )
            {
                ParseItem_s_p_entry( itdatbeg, itemdat[cntitemID] );
            }
        }

        void ParseItem_s_p_entry( vector<uint8_t>::const_iterator & itread, stats::itemdata & item )
        {
            stats::exclusiveitemdata * ptrex = item.GetExclusiveItemData(); //Make the exclusive item data container

            if( ptrex == nullptr )
                ptrex = item.MakeExclusiveData();

            itread = ReadIntFromByteContainer( ptrex->type,  itread );
            itread = ReadIntFromByteContainer( ptrex->param, itread );
        }

        const std::string & m_pathBalanceDir;
    };

    /*
        ItemDataWriter
    */
    class EoSItemDataWriter
    {
    public:
        EoSItemDataWriter( const stats::ItemsDB & itemdata )
            :m_itemdata(itemdata)
        {
        }

        void Write( const std::string & pathBalanceDir )
        {
            stringstream sstritem_p;
            stringstream sstritem_s;
            sstritem_p << utils::TryAppendSlash(pathBalanceDir) << ItemData_FName;
            sstritem_s << utils::TryAppendSlash(pathBalanceDir) << ExclusiveItemData_FName;
            const string item_p = sstritem_p.str();
            const string item_s = sstritem_s.str();

            Write_item_p  (item_p);
            Write_item_s_p(item_s);
        }


    private:

        //
        //  Item_p
        //
        void Write_item_p( const string & path )
        {
            vector<uint8_t> data;
            auto            itbins = back_inserter(data);

            //Pre-alloc
            data.reserve( stats::ItemDataLen_EoS * m_itemdata.size() ); //add padding and ptr offset list.

            for( auto & item : m_itemdata )
            {
                itbins = Write_item_pEntry( item, itbins );
            }

            io::WriteByteVectorToFile( path, MakeSIR0Wrap( data, PaddingByte ) );
        }

        template<class _outit>
            _outit Write_item_pEntry( const stats::itemdata & item, _outit itout )
        {
            itout = WriteIntToByteContainer( item.buyPrice,   itout );
            itout = WriteIntToByteContainer( item.sellPrice,  itout );
            itout = WriteIntToByteContainer( item.category,   itout );
            itout = WriteIntToByteContainer( item.spriteID,   itout );
            itout = WriteIntToByteContainer( item.itemID,     itout );
            itout = WriteIntToByteContainer( item.param1,     itout );
            itout = WriteIntToByteContainer( item.param2,     itout );
            itout = WriteIntToByteContainer( item.param3,     itout );
            itout = WriteIntToByteContainer( item.unk1,       itout );
            itout = WriteIntToByteContainer( item.unk2,       itout );
            itout = WriteIntToByteContainer( item.unk3,       itout );
            itout = WriteIntToByteContainer( item.unk4,       itout );
            return itout;
        }

        //
        //  Item_s_p
        //
        void Write_item_s_p( const string & path )
        {
            vector<uint8_t> data;
            auto            itbins = back_inserter(data);

            for( auto & item : m_itemdata )
            {
                if( item.GetExclusiveItemData() != nullptr )
                    itbins = Write_item_s_pEntry( *(item.GetExclusiveItemData()), itbins );
            }

            io::WriteByteVectorToFile( path, MakeSIR0Wrap( data, PaddingByte ) );
        }

        template<class _outit>
            _outit Write_item_s_pEntry( const stats::exclusiveitemdata & item, _outit itout )
        {
            itout = WriteIntToByteContainer( item.type,  itout );
            itout = WriteIntToByteContainer( item.param, itout );
            return itout;
        }

    private:
        const stats::ItemsDB & m_itemdata;
    };


//
//
//

    stats::ItemsDB ParseItemsDataEoS( const std::string & pathBalanceDir )
    {
        return std::move( EoSItemDataParser(pathBalanceDir) );
    }

    void WriteItemsDataEoS( const std::string & pathBalanceDir, const stats::ItemsDB & itemdata )
    {
        EoSItemDataWriter(itemdata).Write(pathBalanceDir);
    }

    /*
        ParseItemsDataEoTD
            Parse the item data files from Explorers of Time/Darkness.

            * pathItemsdat: The path to the directory containing the item data files.
    */
    stats::ItemsDB ParseItemsDataEoTD( const std::string & pathBalanceDir )
    {
        assert(false);
        return stats::ItemsDB();
    }

    /*
        WriteItemsDataEoTD
            Write the item data files for Explorers of Time/Darkness.

            * pathItemsdat: The directory where the itemdata will be outputed to.
            * itemdata    : The item data to write the output files from.
    */
    void WriteItemsDataEoTD( const std::string & pathBalanceDir, const stats::ItemsDB & itemdata )
    {
        assert(false);
    }

};};