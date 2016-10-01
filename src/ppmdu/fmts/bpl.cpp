#include "bpl.hpp"

using namespace std;

namespace filetypes
{
//============================================================================================
//  BPLParser
//============================================================================================

    template<class _fwdinit>
        class BPLParser
    {
        typedef _fwdinit init_t;
    public:
        BPLParser(pmd2::TilesetPalette & destpal)
            :m_destpal(destpal)
        {}

        void operator()( init_t itbeg, init_t itend )
        {
            bpl_header hdr;
            
        }

    private:
        

    private:
        pmd2::TilesetPalette & m_destpal;
    };

//============================================================================================
//  BPLWriter
//============================================================================================

    class BPLWriter
    {
    public:
        BPLWriter()
        {
        }
    };

//============================================================================================
//  Functions
//============================================================================================
    pmd2::TilesetPalette ParseBPL(const std::string & fpath)
    {
        return pmd2::TilesetPalette();
    }

    void WriteBPL( const std::string & destfpath, const pmd2::TilesetPalette & srcpal )
    {
    }
};
