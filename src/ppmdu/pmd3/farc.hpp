#ifndef FARC_HPP
#define FARC_HPP
/*
farc.hpp
2015/09/23
psycommando@gmail.com
Description: Utilities to handle FARC archives.
*/
#include <cstdint>
#include <string>
#include <vector>
#include <utility>
#include <functional>
#include <utils/utility.hpp>


namespace filetypes
{
    //This is used when writing or reading the content of a FARC. 
    // Instead of storing all the data in-memory, the content is loaded on-demand via a function returning a byte vector!
    typedef std::function<std::vector<uint8_t()>> farccontent_t;

    const uint32_t MagicNumberFARC  = 0x46415243; //"FARC"
    const size_t   MaxSaneFARCTblSz = 1024;       //Value for validating the FARC size in the FARC header to avoid allocating a ton of space for no reasons, with a bad header..

//
//  Header
//

    /*
        farc_hdr
            Header for the FARC format, minus the file offset table.
    */
    struct farc_hdr
    {
        static const unsigned int MinLength = 36; //bytes

        uint32_t            magicn;
        uint16_t            unk1;  
        uint16_t            unk11; 
        uint32_t            unk2;   
        uint16_t            unk3;   
        uint16_t            unk10;  
        uint16_t            unk4;   
        uint16_t            unk5;   
        uint32_t            unk6;   
        uint32_t            unk7;   
        uint16_t            unk8;   
        uint16_t            unk9;  
        uint32_t            ftblsz; 

        //
        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToByteContainer( MagicNumberFARC,  itwriteto,  false );
            itwriteto = utils::WriteIntToByteContainer( unk1,             itwriteto );
            itwriteto = utils::WriteIntToByteContainer( unk11,            itwriteto );
            itwriteto = utils::WriteIntToByteContainer( unk2,             itwriteto ); 
            itwriteto = utils::WriteIntToByteContainer( unk3,             itwriteto );  
            itwriteto = utils::WriteIntToByteContainer( unk10,            itwriteto );  
            itwriteto = utils::WriteIntToByteContainer( unk4,             itwriteto );   
            itwriteto = utils::WriteIntToByteContainer( unk5,             itwriteto );   
            itwriteto = utils::WriteIntToByteContainer( unk6,             itwriteto );  
            itwriteto = utils::WriteIntToByteContainer( unk7,             itwriteto );  
            itwriteto = utils::WriteIntToByteContainer( unk8,             itwriteto );  
            itwriteto = utils::WriteIntToByteContainer( unk9,             itwriteto );  
            itwriteto = utils::WriteIntToByteContainer( ftblsz,           itwriteto );
            return itwriteto;
        }

        //
        template<class _init>
            _init ReadFromContainer( _init itReadfrom )
        {
            itReadfrom = utils::ReadIntFromByteContainer( magicn, itReadfrom, false );
            itReadfrom = utils::ReadIntFromByteContainer( unk1,   itReadfrom );
            itReadfrom = utils::ReadIntFromByteContainer( unk11,  itReadfrom );
            itReadfrom = utils::ReadIntFromByteContainer( unk2,   itReadfrom ); 
            itReadfrom = utils::ReadIntFromByteContainer( unk3,   itReadfrom );  
            itReadfrom = utils::ReadIntFromByteContainer( unk10,  itReadfrom );  
            itReadfrom = utils::ReadIntFromByteContainer( unk4,   itReadfrom );   
            itReadfrom = utils::ReadIntFromByteContainer( unk5,   itReadfrom );   
            itReadfrom = utils::ReadIntFromByteContainer( unk6,   itReadfrom );  
            itReadfrom = utils::ReadIntFromByteContainer( unk7,   itReadfrom );  
            itReadfrom = utils::ReadIntFromByteContainer( unk8,   itReadfrom );  
            itReadfrom = utils::ReadIntFromByteContainer( unk9,   itReadfrom );  
            itReadfrom = utils::ReadIntFromByteContainer( ftblsz, itReadfrom );
            return itReadfrom;
        }
    };


//
//  Class
//
    /*
        FARC
            
    */
    class FARC
    {
        typedef std::pair<uint32_t,uint32_t>      fentry_t;     //Contains an offset and a size
        typedef std::vector<fentry_t>             ftable_t;

        typedef std::pair<farccontent_t,uint32_t> fdataentry_t; //Contains a function and a data size
        typedef std::vector<fdataentry_t>         fdattable_t;

    public:

        //-----------------------------------------------
        //
        //-----------------------------------------------
        void AddFile( farccontent_t datasrc, size_t datalen );



    private:
        farc_hdr    m_hdr;
        ftable_t    m_filestbl;
        fdattable_t m_filedatatbl;
    };


//
//  Helper Functions
//

    //
    FARC ReadFARC ( const std::string & file );
    void WriteFARC( const std::string & file, const FARC & content );
};

#endif