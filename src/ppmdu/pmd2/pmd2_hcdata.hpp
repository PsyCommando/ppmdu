#ifndef PMD2_HCDATA_HPP
#define PMD2_HCDATA_HPP
/*
pmd2_hcdata.hpp
2016/08/22
psycommando@gmail.com
Description:    Utilities for managing access to various pieces of hard-coded data from the PMD2 games.
                Also handles the dumping to loose files of said data so it can be loaded dynamically.
*/
#include <ppmdu/pmd2/pmd2.hpp>
#include <ppmdu/pmd2/pmd2_configloader.hpp>
#include <ppmdu/fmts/sir0.hpp>
#include <utils/utility.hpp>
#include <vector>
#include <iterator>
#include <fstream>
#include <cstdint>
#include <string>
#include <iterator>

namespace pmd2
{
//============================================================================================================
//  Parsing Utility Formats
//============================================================================================================
    
    /*
        ParseAndDumpLUTXML
            
    */
    //template<typename _structType, typename _init, typename _outstrm>
    //    void ParseAndDumpLUTXML( const uint32_t offset, const size_t nbentries, _init itbeg, _init itend, _outstrm & out, const string & headertext, const string & parenttagname, const uint32_t ptrDiff )
    //{
    //    typename _structType::Stats statisticslog;
    //    auto                        itfbeg        = itbeg; //Save iterator before advancing it
    //    std::advance( itbeg, offset );

    //    out << "<!--============================================================-->\n"
    //        <<"<!--" << headertext <<"-->\n"
    //        << "<!--============================================================-->\n"
    //        << "\n<!--";
    //    _structType::PrintHeader(out);
    //    out << "-->\n"
    //        <<"<" <<parenttagname <<">\n";
    //    for( size_t cntentries = 0; cntentries < nbentries; ++cntentries )
    //    {
    //        _structType curentry;
    //        itbeg = curentry.Read( itbeg, itend );
    //        curentry.PrintXML( out, itfbeg, itend, ptrDiff, cntentries );
    //        statisticslog.LogStats(curentry);
    //    }
    //    out << "\n</" <<parenttagname <<">\n";
    //}


    /*******************************************************************
        LevelEntry
            Single entry in the level list
    *******************************************************************/
    struct LevelEntry
    {
        int16_t     mapty      = 0;
        int16_t     unk2       = 0;
        int16_t     mapid      = 0;
        int16_t     unk4       = 0;
        uint32_t    ptrstring  = 0;
        std::string name;

        static const size_t Size = 12;

        template<typename _init>
            _init Read( _init itbeg, _init itend, _init itarm9beg, uint32_t arm9loadoffset = 0x02000000 )
        {
            using namespace std;
            using namespace utils;
            itbeg = ReadIntFromBytes( mapty,        itbeg, itend );
            itbeg = ReadIntFromBytes( unk2,         itbeg, itend );
            itbeg = ReadIntFromBytes( mapid,        itbeg, itend );
            itbeg = ReadIntFromBytes( unk4,         itbeg, itend );
            itbeg = ReadIntFromBytes( ptrstring,    itbeg, itend );

            name = std::move( FetchString( ptrstring - arm9loadoffset, itarm9beg, itend ) );
            return itbeg;
        }

        void Read( std::ifstream & istrm, uint32_t arm9loadoffset )
        {
            using namespace std;
            using namespace utils;
            istreambuf_iterator<char> itbeg(istrm);
            istreambuf_iterator<char> itend;
            itbeg = ReadIntFromBytes( mapty,        itbeg, itend );
            itbeg = ReadIntFromBytes( unk2,         itbeg, itend );
            itbeg = ReadIntFromBytes( mapid,        itbeg, itend );
            itbeg = ReadIntFromBytes( unk4,         itbeg, itend );
            itbeg = ReadIntFromBytes( ptrstring,    itbeg, itend );
            streampos befstring = istrm.tellg();
            istrm.seekg(0);
            name = std::move( FetchString( ptrstring - arm9loadoffset, istreambuf_iterator<char>(istrm), itend ) );
            istrm.seekg(befstring);
        }

        template<class _outfwdit, class _sir0wrapper>
            _outfwdit WriteEntry( uint32_t strpointer, _sir0wrapper & wrap, _outfwdit itw )
        {
            using namespace std;
            using namespace utils;
            //4 shorts, 1 string pointer
            itw = utils::WriteIntToBytes( mapty, itw );
            itw = utils::WriteIntToBytes( unk2,  itw );
            itw = utils::WriteIntToBytes( mapid, itw );
            itw = utils::WriteIntToBytes( unk4,  itw );
            //Write string pointer
            wrap.pushpointer(strpointer);
            return itw;
        }

        template<typename _outstrm, typename _init >
            void Print( _outstrm & out, _init itfbeg, _init itfend, const uint32_t ptrDiff )
        {
            using namespace std;
            using namespace utils;
            string fetchedstr = "NULL";
            if( ptrstring != 0 )
                fetchedstr = FetchString( ptrstring - ptrDiff, itfbeg, itfend );

            out << "-> " <<setfill(' ') <<setw(5) <<mapty
                << ", "  <<setfill(' ') <<setw(5) <<unk2
                << ", "  <<setfill(' ') <<setw(5) <<mapid         
                << ", "  <<setfill(' ') <<setw(5) <<unk4
                << ", \""  <<fetchedstr <<"\""
                <<"\n";
        }


        template<typename _outstrm, typename _init >
            void PrintXML( _outstrm & out, _init itfbeg, _init itfend, const uint32_t ptrDiff, size_t cntentry )
        {
            using namespace std;
            using namespace utils;
            //<Level name="S00P01A"  unk1=" 1"  unk2="  0" mapid="  1" unk4=" 1" />
            string fetchedstr = "NULL";
            if( ptrstring != 0 )
                fetchedstr = FetchString( ptrstring - ptrDiff, itfbeg, itfend );

            out <<"<Level _id=\"" <<setw(3) <<cntentry <<"\" name=\"" <<setfill(' ') <<setw(12) <<left  <<(fetchedstr +"\"")
                <<"mapty=\"" <<right <<setw(2) <<mapty <<"\"  unk2=\"" <<setw(3) <<unk2 <<"\" mapid=\""
                <<setw(3) <<mapid <<"\" unk4=\"" <<setw(2) <<unk4 <<"\" />\n"
                ;
        }

        template<typename _outstrm>
            static void PrintHeader( _outstrm & out )
        {
            out <<"mapty  "
                <<"unk2   "
                <<"mapid  "
                <<"Unk4   "
                <<"mapname"
                ;
        }

        operator level_info()const
        {
            level_info inf;
            inf.name  = name;
            inf.mapty = mapty;
            inf.unk2  = unk2;
            inf.mapid = mapid;
            inf.unk4  = unk4;
            return std::move(inf);
        }

        LevelEntry & operator=( const level_info & inf )
        {
            name  = inf.name;
            mapty = inf.mapty;
            unk2  = inf.unk2;
            mapid = inf.mapid;
            unk4  = inf.unk4;
            return *this;
        }
    };

    /*******************************************************************
        EntitySymbolListEntry
    *******************************************************************/
    struct EntitySymbolListEntry
    {
        int16_t  type       = 0;
        int16_t  entid      = 0;
        uint32_t ptrstring  = 0;
        uint16_t unk3       = 0;
        uint16_t unk4       = 0;
        std::string name;

        static const size_t Size = 12;

        template<typename _init>
            _init Read( _init itbeg, _init itend )
        {
            using namespace std;
            using namespace utils;
            itbeg = ReadIntFromBytes( type,         itbeg, itend );
            itbeg = ReadIntFromBytes( entid,        itbeg, itend );
            itbeg = ReadIntFromBytes( ptrstring,    itbeg, itend );
            itbeg = ReadIntFromBytes( unk3,         itbeg, itend );
            itbeg = ReadIntFromBytes( unk4,         itbeg, itend );
            return itbeg;
        }

        void Read( std::ifstream & istrm, uint32_t arm9loadoffset )
        {
            using namespace std;
            using namespace utils;
            istreambuf_iterator<char> itbeg(istrm);
            istreambuf_iterator<char> itend;
            itbeg = ReadIntFromBytes( type,         itbeg, itend );
            itbeg = ReadIntFromBytes( entid,        itbeg, itend );
            itbeg = ReadIntFromBytes( ptrstring,    itbeg, itend );
            itbeg = ReadIntFromBytes( unk3,         itbeg, itend );
            itbeg = ReadIntFromBytes( unk4,         itbeg, itend );
            streampos befstring = istrm.tellg();
            istrm.seekg(0);
            name = std::move( FetchString( ptrstring - arm9loadoffset, istreambuf_iterator<char>(istrm), itend ) );
            istrm.seekg(befstring);
        }

        template<class _outfwdit, class _sir0wrapper>
            _outfwdit WriteEntry( uint32_t strpointer, _sir0wrapper & wrap, _outfwdit itw )
        {
            using namespace std;
            using namespace utils;
            //2 shorts, 1 pointer, 2 shorts.
            itw = utils::WriteIntToBytes( type,  itw );
            itw = utils::WriteIntToBytes( entid, itw );
            //Write string pointer
            wrap.pushpointer(strpointer);
            itw = utils::WriteIntToBytes( unk3,  itw );
            itw = utils::WriteIntToBytes( unk4,  itw );
            return itw;
        }

        template<typename _outstrm, typename _init >
            void Print( _outstrm & out, _init itfbeg, _init itfend, const uint32_t ptrDiff  )
        {
            using namespace std;
            using namespace utils;
            string fetchedstr = "NULL";
            if( ptrstring != 0 )
                fetchedstr = FetchString( ptrstring - ptrDiff, itfbeg, itfend );

            out << "-> " <<setfill(' ') <<setw(5) <<type
                << ", "  <<setfill(' ') <<setw(9) <<entityid
                <<hex <<uppercase
                << ", "  <<setfill(' ') <<setw(6) <<NumberToHexString(unk3)
                << ", "  <<setfill(' ') <<setw(6) <<NumberToHexString(unk4)
                <<dec <<nouppercase
                << ", \""  <<fetchedstr <<"\""
                <<"\n";
        }

        template<typename _outstrm, typename _init >
            void PrintXML( _outstrm & out, _init itfbeg, _init itfend, const uint32_t ptrDiff, size_t cntentry  )
        {
            using namespace std;
            using namespace utils;
            //<Entity name="PLAYER"                 type="1" entid="   0" unk3="  0x0"  unk4="0x102" />
            string fetchedstr = "NULL";
            if( ptrstring != 0 )
                fetchedstr = FetchString( ptrstring - ptrDiff, itfbeg, itfend );

            out <<"<Entity _id=\"" <<setw(3) <<cntentry <<"\" name=\"" <<setfill(' ') <<setw(24) <<left <<(fetchedstr +"\"") <<"type=\"" <<right
                <<type <<"\" entid=\"" <<setfill(' ') <<setw(4) <<entid
                <<"\" unk3=\"" <<setw(5) 
                <<hex <<uppercase 
                <<NumberToHexString(unk3)
                <<"\"  unk4=\"" <<setw(5) 
                <<NumberToHexString(unk4)
                <<dec <<nouppercase
                <<"\" />\n"
                ;
        }

        template<typename _outstrm>
            static void PrintHeader( _outstrm & out )
        {
            out <<"Type   "
                <<"Entity Id  "
                <<"Unk3    "
                <<"Unk4    "
                <<"Symbol "
                ;
        }

        operator livesent_info()const
        {
            livesent_info inf;
            inf.name    = name;
            inf.type    = type;
            inf.entid   = entid;
            inf.unk3    = unk3;
            inf.unk4    = unk4;
            return std::move(inf);
        }

        EntitySymbolListEntry & operator=(const livesent_info & inf)
        {
            name  = inf.name;
            type  = inf.type;
            entid = inf.entid;
            unk3  = inf.unk3;
            unk4  = inf.unk4;
            return *this;
        }
    };


    /*******************************************************************
        ScriptVariablesEntry

    *******************************************************************/
    struct ScriptVariablesEntry
    {
        int16_t  type;
        int16_t  unk1;
        uint16_t memoffset;
        uint16_t bitshift;
        uint16_t unk3;
        uint16_t unk4;
        uint32_t ptrstring;
        std::string name;

        static const size_t Size = 16;

        template<typename _init>
            _init Read( _init itbeg, _init itend )
        {
            using namespace std;
            using namespace utils;
            itbeg = ReadIntFromBytes( type,             itbeg, itend );
            itbeg = ReadIntFromBytes( unk1,             itbeg, itend );
            itbeg = ReadIntFromBytes( memoffset,        itbeg, itend );
            itbeg = ReadIntFromBytes( bitshift,         itbeg, itend );
            itbeg = ReadIntFromBytes( unk3,             itbeg, itend );
            itbeg = ReadIntFromBytes( unk4,             itbeg, itend );
            itbeg = ReadIntFromBytes( ptrstring,        itbeg, itend );
            return itbeg;
        }

        void Read( std::ifstream & istrm, uint32_t arm9loadoffset )
        {
            using namespace std;
            using namespace utils;
            istreambuf_iterator<char> itbeg(istrm);
            istreambuf_iterator<char> itend;
            itbeg = ReadIntFromBytes( type,             itbeg, itend );
            itbeg = ReadIntFromBytes( unk1,             itbeg, itend );
            itbeg = ReadIntFromBytes( memoffset,        itbeg, itend );
            itbeg = ReadIntFromBytes( bitshift,         itbeg, itend );
            itbeg = ReadIntFromBytes( unk3,             itbeg, itend );
            itbeg = ReadIntFromBytes( unk4,             itbeg, itend );
            itbeg = ReadIntFromBytes( ptrstring,        itbeg, itend );
            streampos befstring = istrm.tellg();
            istrm.seekg(0);
            name = std::move( FetchString( ptrstring - arm9loadoffset, istreambuf_iterator<char>(istrm), itend ) );
            istrm.seekg(befstring);
        }

        template<class _outfwdit, class _sir0wrapper>
            _outfwdit WriteEntry( uint32_t strpointer, _sir0wrapper & wrap, _outfwdit itw )
        {
            using namespace std;
            using namespace utils;
            itw = WriteIntToBytes( type,             itw );
            itw = WriteIntToBytes( unk1,             itw );
            itw = WriteIntToBytes( memoffset,        itw );
            itw = WriteIntToBytes( bitshift,         itw );
            itw = WriteIntToBytes( unk3,             itw );
            itw = WriteIntToBytes( unk4,             itw );
            //Write string pointer
            wrap.pushpointer(strpointer);
            return itw;
        }


        template<typename _outstrm, typename _init >
            void Print( _outstrm & out, _init itfbeg, _init itfend, const uint32_t ptrDiff  )
        {
            using namespace std;
            using namespace utils;
            string fetchedstr = "NULL";
            if( ptrstring != 0 )
                fetchedstr = FetchString( ptrstring - ptrDiff, itfbeg, itfend );

            out << "-> " <<setfill(' ') <<setw(5) <<variabletype
                <<hex <<uppercase
                << ", "  <<setfill(' ') <<setw(6) <<NumberToHexString(unk1)
                << ", "  <<setfill(' ') <<setw(6) <<NumberToHexString(offset)
                << ", "  <<setfill(' ') <<setw(6) <<NumberToHexString(bitshift)
                << ", "  <<setfill(' ') <<setw(6) <<NumberToHexString(unk3)
                << ", "  <<setfill(' ') <<setw(6) <<NumberToHexString(unk4)
                <<dec <<nouppercase
                << ", \""  <<fetchedstr <<"\""
                <<"\n";
        }

        template<typename _outstrm, typename _init >
            void PrintXML( _outstrm & out, _init itfbeg, _init itfend, const uint32_t ptrDiff, size_t cntentry  )
        {
            using namespace std;
            using namespace utils;
            //<Entity name="PLAYER"                 type="1" entid="   0" unk3="  0x0"  unk4="0x102" />
            string fetchedstr = "NULL";
            if( ptrstring != 0 )
                fetchedstr = FetchString( ptrstring - ptrDiff, itfbeg, itfend );

            out <<"<GameVar _id=\"" <<setw(3) <<cntentry <<"\" =\"" <<setfill(' ') <<setw(24) <<left <<(fetchedstr +"\"") <<"type=\"" <<right
                <<type <<"\" entid=\"" <<setfill(' ') <<setw(4) <<entityid
                <<"\" unk3=\"" <<setw(5) 
                <<hex <<uppercase 
                <<NumberToHexString(unk3)
                <<"\"  unk4=\"" <<setw(5) 
                <<NumberToHexString(unk4)
                <<dec <<nouppercase
                <<"\" />\n"
                ;
        }

        template<typename _outstrm>
            static void PrintHeader( _outstrm & out )
        {
            out <<"Type     "
                <<"Unk1     "
                <<"Offset   "
                <<"bitshift "
                <<"Unk3     "
                <<"Unk4     "
                <<"Symbol   "
                ;
        }

        operator gamevariable_info()const
        {
            gamevariable_info inf;
            inf.type        = type;
            inf.unk1        = unk1;
            inf.memoffset   = memoffset;
            inf.bitshift    = bitshift;
            inf.unk3        = unk3;
            inf.unk4        = unk4;
            inf.name        = name;
            return std::move(inf);
        }

        ScriptVariablesEntry & operator=( const gamevariable_info & inf )
        {
            type        = inf.type;
            unk1        = inf.unk1;
            memoffset   = inf.memoffset;
            bitshift    = inf.bitshift;
            unk3        = inf.unk3;
            unk4        = inf.unk4;
            name        = inf.name;
            return *this;
        }
    };


/*
    ObjectFileListEntry
        Single entry in the object list
*/
struct ObjectFileListEntry
{
    int16_t  unk1       = 0;
    int16_t  unk2       = 0;
    uint32_t ptrstring  = 0;
    uint32_t unk3       = 0;
    std::string name;

    static const size_t Size = 12;

    template<typename _init>
        _init Read( _init itbeg, _init itend )
    {
        itbeg = ReadIntFromBytes( unk1,         itbeg, itend );
        itbeg = ReadIntFromBytes( unk2,         itbeg, itend );
        itbeg = ReadIntFromBytes( ptrstring,    itbeg, itend );
        itbeg = ReadIntFromBytes( unk3,         itbeg, itend );
        return itbeg;
    }

    void Read( std::ifstream & istrm, uint32_t arm9loadoffset )
    {
        using namespace std;
        using namespace utils;
        istreambuf_iterator<char> itbeg(istrm);
        istreambuf_iterator<char> itend;
        itbeg = ReadIntFromBytes( unk1,         itbeg, itend );
        itbeg = ReadIntFromBytes( unk2,         itbeg, itend );
        itbeg = ReadIntFromBytes( ptrstring,    itbeg, itend );
        itbeg = ReadIntFromBytes( unk3,         itbeg, itend );
        streampos befstring = istrm.tellg();
        istrm.seekg(0);
        name = std::move( FetchString( ptrstring - arm9loadoffset, istreambuf_iterator<char>(istrm), itend ) );
        istrm.seekg(befstring);
    }

    template<class _outfwdit, class _sir0wrapper>
        _outfwdit WriteEntry( uint32_t strpointer, _sir0wrapper & wrap, _outfwdit itw )
    {
        using namespace std;
        using namespace utils;
        itw = WriteIntToBytes( unk1, itw );
        itw = WriteIntToBytes( unk2, itw );
        //Write string pointer
        wrap.pushpointer(strpointer);
        itw = WriteIntToBytes( unk3, itw );
        return itw;
    }

    template<typename _outstrm, typename _init >
        void Print( _outstrm & out, _init itfbeg, _init itfend, const uint32_t ptrDiff  )
    {
        string fetchedstr = "NULL";
        if( ptrstring != 0 )
            fetchedstr = FetchString( ptrstring - ptrDiff, itfbeg, itfend );

        out << "-> " <<setfill(' ') <<setw(5) <<unk1
            << ", "  <<setfill(' ') <<setw(5) <<unk2
            << ", "  <<setfill(' ') <<setw(8) <<unk3         
            << ", \""  <<fetchedstr <<"\""
            <<"\n";
    }


    template<typename _outstrm >
        void PrintXML( _outstrm & out, size_t cntentry  )
    {
        //<Object _id="11"    unk1="11"   unk2="258"  unk3="0"  name="d01p11b1" />
        out <<"<Object _id=\"" <<setw(3) <<cntentry <<"\" unk1=\"" <<setw(2) <<unk1
            <<"\" unk2=\"" <<setw(3) <<unk2 <<"\" unk3=\"" <<unk3 <<"\" name=\""
            <<name <<"\" />\n"
            ;
    }

    template<typename _outstrm>
        static void PrintHeader( _outstrm & out )
    {
        out <<"Unk1   "
            <<"Unk2   "
            <<"Unk3      "
            <<"Symbol "
            ;
    }

    operator object_info()const
    {
        object_info inf;
        inf.unk1        = unk1;
        inf.unk2        = unk2;
        inf.unk3        = unk3;
        inf.name        = name;
        return std::move(inf);
    }

    ObjectFileListEntry & operator=( const object_info & inf )
    {
        unk1 = inf.unk1;
        unk2 = inf.unk2;
        unk3 = inf.unk3;
        name = inf.name;
        return *this;
    }
};

#if 0 
//============================================================================================================
//  Loader
//============================================================================================================

    /*****************************************************************
        HardCodedDataLoader
            
    *****************************************************************/
    class HardCodedDataLoader
    {
    public:
        HardCodedDataLoader( const std::string & romroot, ConfigLoader & cfg )
            :m_conf(&cfg), m_romroot(romroot)
        {}

        GameScriptData::lvlinf_t LoadLevelListSIR0()
        {
            assert(false);
            return ;
        }

        GameScriptData::lvlinf_t LoadLevelListSIR0(const std::string & srcfile)
        {
            assert(false);
            return GameScriptData::lvlinf_t();
        }

    private:
        ConfigLoader * m_conf;
        std::string    m_romroot;
    };



//============================================================================================================
//  Dumper
//============================================================================================================

    /*****************************************************************
        HardCodedDataDumper
            This interface is meant to be used for dumping 
            various hard-coded data tables from the PMD2 games.
            It can do so using several sources, such as the 
            XML data file, or just raw data.
    *****************************************************************/
    class HardCodedDataDumper
    {
    public:
        HardCodedDataDumper( const ConfigLoader & cfg )
            :m_conf(&cfg)
        {}

        //template<class _EntryTy>
        //    struct WriteEntryType_impl
        //{};

        /*
            WriteEntry
                Write entry for the level list into a SIR0 container.
        */
        //template<>
        //    struct WriteEntryType_impl<pmd2::level_info>
        //{
        //    template<class _outfwdit, class _sir0wrapper>
        //        static _outfwdit WriteEntry( const pmd2::level_info & inf, uint32_t strpointer, _sir0wrapper & wrap, _outfwdit itw )
        //    {
        //        //4 shorts, 1 string pointer
        //        itw = utils::WriteIntToBytes( inf.mapty, itw );
        //        itw = utils::WriteIntToBytes( inf.unk2,  itw );
        //        itw = utils::WriteIntToBytes( inf.mapid, itw );
        //        itw = utils::WriteIntToBytes( inf.unk4,  itw );
        //        //Write string pointer
        //        wrap.pushpointer(strpointer);
        //        return itw;
        //    }
        //};

        /*
            WriteEntry
                Write entry for the actor list into a SIR0 container.
        */
        //template<>
        //    struct WriteEntryType_impl<pmd2::livesent_info>
        //{
        //    template<class _outfwdit, class _sir0wrapper>
        //        static _outfwdit WriteEntry( const pmd2::livesent_info & inf, uint32_t strpointer, _sir0wrapper & wrap, _outfwdit itw )
        //    {
        //        //2 shorts, 1 pointer, 2 shorts.
        //        itw = utils::WriteIntToBytes( inf.type,  itw );
        //        itw = utils::WriteIntToBytes( inf.entid, itw );
        //        //Write string pointer
        //        wrap.pushpointer(strpointer);
        //        itw = utils::WriteIntToBytes( inf.unk3,  itw );
        //        itw = utils::WriteIntToBytes( inf.unk4,  itw );
        //        return itw;
        //    }
        //};


        /*
            DumpStringAndList
                Generic function for writing a SIR0 file containing a consecutive list of entries, and a consecutive 
                block of null terminated c strings referred to by each entry.
        */
        template<unsigned int _EntryLen, class _EntryType, class _TransType, class _infwdit, class _outfwdit>
            _outfwdit DumpStringAndList( _infwdit itbeg, _infwdit itend, _outfwdit itw, bool bputsubheader = true )const
        {
            using std::vector;
            using std::back_inserter;
            static_assert( std::is_same<_EntryType&, typename decltype(*itbeg)>::value ||
                           std::is_same<const _EntryType&, typename decltype(*itbeg)>::value, 
                           "HardCodedDataDumper::DumpStringAndList(): Iterators weren't iterators on expected type!!" );

            static const size_t entrysz = _EntryLen; //Size of an entry 

            // -----------------------------------------
            // === 1st pass pile up strings in order ===
            // -----------------------------------------
            ::filetypes::FixedSIR0DataWrapper<std::vector<uint8_t>> sir0anddat;
            const size_t                     nbentries = std::distance(itbeg, itend);
            vector<uint8_t>                & outdata   = sir0anddat.Data();
            vector<uint32_t>                 stroffsets;
            stroffsets.reserve(nbentries);
            sir0anddat.Data().reserve((nbentries * 8) + (nbentries * entrysz));

            auto itbackins = std::back_inserter(sir0anddat.Data());

            for( auto itstr = itbeg; itstr != itend; ++itstr)
            {
                const _EntryType & inf = *itstr;
                stroffsets.push_back(outdata.size());
                std::copy( inf.name.begin(), inf.name.end(), itbackins );
                outdata.push_back(0);
                utils::AppendPaddingBytes( itbackins, outdata.size(), 4, 0 );
            }
            utils::AppendPaddingBytes(itbackins, outdata.size(), 16, 0); //Pad the strings, because I'm a perfectionist

            // -----------------------------------------
            // === 2nd pass, write the table entries ===
            // -----------------------------------------
            uint32_t ptrdatatbl = outdata.size();

            size_t cntptr = 0;
            for( auto itentry = itbeg; itentry != itend; ++itentry, ++cntptr )
            {
                //WriteEntryType_impl<_EntryType>::WriteEntry(*itentry, stroffsets[cntptr], sir0anddat, itbackins);
                _TransType trans;
                trans = (*itentry);
                trans->WriteEntry<_EntryType>(stroffsets[cntptr], sir0anddat, itbackins);
            }

            utils::AppendPaddingBytes(itbackins, outdata.size(), 16, 0xAA);

            if(bputsubheader)
            {
                sir0anddat.SetDataPointerOffset(outdata.size());
                //Append a subheader
                sir0anddat.pushpointer(ptrdatatbl);
                itbackins = utils::WriteIntToBytes( static_cast<uint32_t>(nbentries),  itbackins );
                utils::AppendPaddingBytes(itbackins, outdata.size(), 16, 0xAA);
            }
            else
                sir0anddat.SetDataPointerOffset(ptrdatatbl);

            //Then write out!
            sir0anddat.Write(itw);
            return itw; 
        }

        /*
            DumpActorList
                Dump the actor list from the given source.
        */
        template<class _infwdit, class _outfwdit>
            inline _outfwdit DumpActorList( _infwdit itbeg, _infwdit itend, _outfwdit itw )const
        {
            static const size_t entrysz = 12;
            return DumpStringAndList<entrysz, pmd2::livesent_info>(itbeg, itend, itw);
        }

        /*
            DumpActorDataFromConf
                Dump the actor list currently loaded in the config loader.
        */
        void DumpActorDataFromConf( const std::string & outfpath )const 
        {
            try
            {
                std::ofstream out(outfpath, std::ios::out|std::ios::binary);
                out.exceptions(std::ios::badbit);
                DumpActorList( m_conf->GetGameScriptData().LivesEnt().begin(), 
                               m_conf->GetGameScriptData().LivesEnt().end(), 
                               std::ostreambuf_iterator<char>(out) );
            }
            catch(const std::exception &)
            {
                std::throw_with_nested( std::runtime_error("HardCodedDataDumper::DumpActorDataFromConf(): IO error writing to file " + outfpath) );
            }
        }


        /*
            DumpLevelList
                This dumps a range containing "level_info" structs into a sir0 wrapped representation of the data.
                    -itbeg : The beginning of the range.
                    -itend : The end of the range.
                    -itw   : The output iterator on values accepting assignement from bytes.
                Returns the new output position.
        */
        template<class _infwdit, class _outfwdit>
            inline _outfwdit DumpLevelList( _infwdit itbeg, _infwdit itend, _outfwdit itw )const
        {
            static const size_t entrysz = 12;
            return DumpStringAndList<entrysz, pmd2::level_info>(itbeg, itend, itw);
        }

        /*
            DumpLevelDataFromConf
                Dump the level list from the currently loaded configuration.
        */
        void DumpLevelDataFromConf( const std::string & fpath )const 
        {
            try
            {
                std::ofstream out(fpath, std::ios::out|std::ios::binary);
                out.exceptions(std::ios::badbit);
                DumpLevelList( m_conf->GetGameScriptData().LevelInfo().begin(), 
                               m_conf->GetGameScriptData().LevelInfo().end(), 
                               std::ostreambuf_iterator<char>(out) );
            }
            catch(const std::exception &)
            {
                std::throw_with_nested( std::runtime_error("HardCodedDataDumper::DumpLevelDataFromConf(): IO error writing to file " + fpath) );
            }
        }

    private:
        const ConfigLoader * m_conf;
    };
#endif
};

#endif