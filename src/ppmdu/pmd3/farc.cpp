#include "farc.hpp"

using namespace std;

namespace filetypes
{
    //Input
    

    //Output

        //void LoadFARC( const std::string & path );

        //template<class _init>
        //    void LoadFARC( _init beg, _init end )
        //{
        //    //Load header
        //    beg = m_hdr.ReadFromContainer(beg);

        //    //Validate
        //    if( m_hdr.magicn != MagicNumberFARC )
        //        throw std::runtime_error("FARC::LoadFARC() : The data to load is not a FARC archive!");

        //    //
        //    const size_t nbentries = m_hdr.ftblsz / 2;
        //    m_filestbl.reserve(nbentries);

        //    //Load File Table
        //    for( size_t i = 0; i < nbentries; ++i )
        //    {
        //        uint32_t offset = 0;
        //        uint32_t len    = 0;
        //        beg = utils::ReadIntFromByteContainer( offset, beg );
        //        beg = utils::ReadIntFromByteContainer( len,    beg );
        //        m_filestbl.push_back( std::move( std::make_pair( offset, len ) ) );
        //    }
        //}


        //void WriteFARC( const std::string & path );

        //template<class _backinsit>
        //    _backinsit WriteFARC( _backinsit itout )
        //{
        //    return itout;
        //}

    //Functions



};