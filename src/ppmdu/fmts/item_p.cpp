#include "item_p.hpp"
#include <ppmdu/fmts/sir0.hpp>
#include <ppmdu/utils/poco_wrapper.hpp>
#include <ppmdu/utils/utility.hpp>
#include <sstream>
#include <iostream>


using namespace std;

namespace pmd2 {namespace filetypes 
{

    class ItemDataParser
    {
    public:
        ItemDataParser( const std::string & pathBalanceDir )
            :m_pathBalanceDir(pathBalanceDir)
        {
        }

        operator stats::ItemsDB()
        {
            stats::ItemsDB result;
            //Figure out if we parse the 2  item files, or only one
            stringstream sstritem_p;
            stringstream sstritem_s;
            sstritem_p << utils::AppendTraillingSlashIfNotThere(m_pathBalanceDir) << ItemData_FName;
            sstritem_s << utils::AppendTraillingSlashIfNotThere(m_pathBalanceDir) << ExclusiveItemData_FName;
            string item_p = sstritem_p.str();
            string item_s = sstritem_s.str();

            if( utils::isFile(item_p) )
            {
                
                if( utils::isFile(item_s) )
                {
                    cout <<ExclusiveItemData_FName <<" is present, parsing exclusive item extra data..\n";

                    result.resize(stats::ItemDataNbEntry_EoS);
                    ParseItem_s_p(item_s);
                    ParseItem_pEoS(item_p);
                    
                }
                else
                {
                    cout <<ExclusiveItemData_FName <<" is not present, ignoring exclusive item extra data.. Assuming the data is from Explorers of Time/Darkness!\n";
                    result.resize(stats::ItemDataNbEntry_EoTD);
                    ParseItem_pEoTD(item_p);
                }
            }
            else
            {
                ostringstream sstrerr;
                sstrerr << "ERROR: Couldn't find the \"" << item_p <<"\" file!";
                string strerr = sstrerr.str();
                clog << strerr <<"\n";
                throw runtime_error(strerr);
            }
            return std::move(result);
        }

    private:

        void ParseItem_pEoS( const string & path )
        {
            vector<uint8_t> data = utils::io::ReadFileToByteVector( path );

            //Parse header
            sir0_header hdr;
            hdr.ReadFromContainer(data.begin());

            const uint32_t NbEntries = (hdr.ptrPtrOffsetLst - hdr.subheaderptr) / stats::ItemDataLen;

            for( unsigned int cnt = 0; cnt < NbEntries; ++cnt )
            {
            }
        }

        void ParseItem_pEoTD( const string & path )
        {
        }

        void ParseItem_s_p( const string & path )
        {
            vector<uint8_t> data = utils::io::ReadFileToByteVector( path );

            //Parse header
            sir0_header hdr;
            hdr.ReadFromContainer(data.begin());

            const uint32_t NbEntries = (hdr.ptrPtrOffsetLst - hdr.subheaderptr) / stats::ExclusiveItemDataLen;
            auto           itdatbeg  = data.begin() + hdr.subheaderptr;

            for( unsigned int cnt = 0; cnt < NbEntries; ++cnt )
            {
                ParseItem_entry(itdatbeg);
            }
        }

        stats::itemdata ParseItem_entry( vector<uint8_t>::const_iterator & itread )
        {

        }

        const std::string & m_pathBalanceDir;
    };

    class ItemDataWriter
    {
    public:

    private:
    };

    /*
        * pathItemsdat: The path to the directory containing either a single item_p.bin or both item_p.bin and item_s_p.bin !
    */
    stats::ItemsDB ParseItemsData( const std::string & pathBalanceDir )
    {
    }

    /*
        * pathItemsdat: The directory where the itemdata will be outputed to, in the form of at least a item_p.bin 
                        and, if there are any PMD:EoS items, possibly also an item_s_p.bin.
        * itemdata    : The item data to write the output files from.
    */
    void WriteItemsData( const std::string & pathBalanceDir, const stats::ItemsDB & itemdata )
    {
    }

};};