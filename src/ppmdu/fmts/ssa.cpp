#include "ssa.hpp"
#include <utils/utility.hpp>
#include <iostream>
using namespace std;

namespace filetypes
{
//=======================================================================================
//  Functions
//=======================================================================================

//=======================================================================================
//  SSDataParser
//=======================================================================================
    template<typename _init>
    class SSDataParser
    {
    public:
        typedef _init initer_t;

        SSDataParser( _init itbeg, _init itend, const std::string & origfname )
            :m_itbeg(itbeg), m_itcur(itbeg), m_itend(itend), m_origfname(origfname)
        {}


        pmd2::ScriptEntityData Parse()
        {
            //cerr << "SSDataParser not implemented!\n";
            return pmd2::ScriptEntityData();
        }

    private:

        void ParseHeader()
        {
        }

        void ParseToc()
        {
        }

    private:
        initer_t                m_itbeg;
        initer_t                m_itcur;
        initer_t                m_itend;
        std::string             m_origfname;
        pmd2::ScriptEntityData  m_out;
    };

//=======================================================================================
//  Functions
//=======================================================================================
    pmd2::ScriptEntityData ParseScriptData( const std::string & fpath )
    {
        vector<uint8_t> fdata( std::move(utils::io::ReadFileToByteVector(fpath)) );
        return std::move( SSDataParser<vector<uint8_t>::const_iterator>(fdata.begin(), fdata.end(), utils::GetFilename(fpath) ).Parse() );
    }

    void WriteScriptData( const std::string & fpath, const pmd2::ScriptEntityData & scrdat )
    {
    }


};