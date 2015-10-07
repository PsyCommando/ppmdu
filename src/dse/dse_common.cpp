#include "dse_common.hpp"

#include <ppmdu/fmts/smdl.hpp> //#TODO #MOVEME

#include <sstream>
#include <iomanip>
using namespace std;

namespace DSE
{
//=================================================================================================
//  Constants
//=================================================================================================
    const std::array<eDSEChunks, NB_DSEChunks> DSEChunksList 
    {{
        eDSEChunks::wavi, 
        eDSEChunks::prgi, 
        eDSEChunks::kgrp, 
        eDSEChunks::pcmd,
        eDSEChunks::trk,
        eDSEChunks::seq,
        eDSEChunks::bnkl,
        eDSEChunks::mcrl,
        eDSEChunks::eoc,
        eDSEChunks::eod,
    }};

    const std::array<int16_t,128> Duration_Lookup_Table =
    {
        0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 
        0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F, 
        0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 
        0x0018, 0x0019, 0x001A, 0x001B, 0x001C, 0x001D, 0x001E, 0x001F, 
        0x0020, 0x0023, 0x0028, 0x002D, 0x0033, 0x0039, 0x0040, 0x0048, 
        0x0050, 0x0058, 0x0062, 0x006D, 0x0078, 0x0083, 0x0090, 0x009E, 
        0x00AC, 0x00BC, 0x00CC, 0x00DE, 0x00F0, 0x0104, 0x0119, 0x012F, 
        0x0147, 0x0160, 0x017A, 0x0196, 0x01B3, 0x01D2, 0x01F2, 0x0214, 
        0x0238, 0x025E, 0x0285, 0x02AE, 0x02D9, 0x0307, 0x0336, 0x0367, 
        0x039B, 0x03D1, 0x0406, 0x0442, 0x047E, 0x04C4, 0x0500, 0x0546, 
        0x058C, 0x0622, 0x0672, 0x06CC, 0x071C, 0x0776, 0x07DA, 0x0834, 
        0x0898, 0x0906, 0x096A, 0x09D8, 0x0A50, 0x0ABE, 0x0B40, 0x0BB8, 
        0x0C3A, 0x0CBC, 0x0D48, 0x0DDE, 0x0E6A, 0x0F00, 0x0FA0, 0x1040, 
        0x10EA, 0x1194, 0x123E, 0x12F2, 0x13B0, 0x146E, 0x1536, 0x15FE, 
        0x16D0, 0x17A2, 0x187E, 0x195A, 0x1A40, 0x1B30, 0x1C20, 0x1D1A, 
        0x1E1E, 0x1F22, 0x2030, 0x2148, 0x2260, 0x2382, 0x2710, 0x7FFF
    };
    

    const std::array<int32_t,128> Duration_Lookup_Table_NullMulti =
    {
        0x00000000, 0x00000004, 0x00000007, 0x0000000A, 
        0x0000000F, 0x00000015, 0x0000001C, 0x00000024, 
        0x0000002E, 0x0000003A, 0x00000048, 0x00000057, 
        0x00000068, 0x0000007B, 0x00000091, 0x000000A8, 
        0x00000185, 0x000001BE, 0x000001FC, 0x0000023F, 
        0x00000288, 0x000002D6, 0x0000032A, 0x00000385, 
        0x000003E5, 0x0000044C, 0x000004BA, 0x0000052E, 
        0x000005A9, 0x0000062C, 0x000006B5, 0x00000746, 
        0x00000BCF, 0x00000CC0, 0x00000DBD, 0x00000EC6, 
        0x00000FDC, 0x000010FF, 0x0000122F, 0x0000136C, 
        0x000014B6, 0x0000160F, 0x00001775, 0x000018EA, 
        0x00001A6D, 0x00001BFF, 0x00001DA0, 0x00001F51, 
        0x00002C16, 0x00002E80, 0x00003100, 0x00003395, 
        0x00003641, 0x00003902, 0x00003BDB, 0x00003ECA, 
        0x000041D0, 0x000044EE, 0x00004824, 0x00004B73, 
        0x00004ED9, 0x00005259, 0x000055F2, 0x000059A4, 
        0x000074CC, 0x000079AB, 0x00007EAC, 0x000083CE, 
        0x00008911, 0x00008E77, 0x000093FF, 0x000099AA, 
        0x00009F78, 0x0000A56A, 0x0000AB80, 0x0000B1BB, 
        0x0000B81A, 0x0000BE9E, 0x0000C547, 0x0000CC17, 
        0x0000FD42, 0x000105CB, 0x00010E82, 0x00011768, 
        0x0001207E, 0x000129C4, 0x0001333B, 0x00013CE2, 
        0x000146BB, 0x000150C5, 0x00015B02, 0x00016572, 
        0x00017015, 0x00017AEB, 0x000185F5, 0x00019133, 
        0x0001E16D, 0x0001EF07, 0x0001FCE0, 0x00020AF7, 
        0x0002194F, 0x000227E6, 0x000236BE, 0x000245D7, 
        0x00025532, 0x000264CF, 0x000274AE, 0x000284D0, 
        0x00029536, 0x0002A5E0, 0x0002B6CE, 0x0002C802, 
        0x000341B0, 0x000355F8, 0x00036A90, 0x00037F79, 
        0x000394B4, 0x0003AA41, 0x0003C021, 0x0003D654, 
        0x0003ECDA, 0x000403B5, 0x00041AE5, 0x0004326A, 
        0x00044A45, 0x00046277, 0x00047B00, 0x7FFFFFFF
    };

    inline eDSEChunks IntToChunkID( uint32_t value )
    {
        eDSEChunks valcompare = static_cast<eDSEChunks>(value);
        
        for( auto cid : DSEChunksList )
        {
            if( valcompare == cid )
                return valcompare;
        }

        return eDSEChunks::invalid;
    }
    
    inline uint32_t ChunkIDToInt( eDSEChunks id )
    {
        return static_cast<uint32_t>(id);
    }

//=================================================================================================
//  DurationLookupTable stuff
//=================================================================================================

    //#FIXME: MOST LIKELY INNACURATE ! The duration for envelope phases is based around the DSE tick counter. So it may or may not be affected by tempo.
    int32_t DSEEnveloppeDurationToMSec( int8_t param, int8_t multiplier )
    { 
        //A rate taken from VGM Trans. It might be close to what we need to convert tick rate into time..
       // static const double MysteryRate = 1.0/ 192.0; 

        //The value from the table is multiplied by 1,000
        static const uint32_t UnitSwitch  = 1000;
        //..then divided by 10,000, to give us a tick quantity
        static const uint32_t UnitDivisor = 10000;

#if 0
        if( multiplier == 0 )
            return (Duration_Lookup_Table_NullMulti[labs(param)]);
        else
            return (Duration_Lookup_Table[labs(param)] * multiplier);
#else
        if( multiplier == 0 )
            return /*lround(*/ ( (Duration_Lookup_Table_NullMulti[param] * UnitSwitch) / UnitDivisor ) * 35  /*)*/; //25 was good
        else
            return /*lround(*/ ( ( (Duration_Lookup_Table[param] * multiplier) * UnitSwitch) / UnitDivisor ) * 35 /*)*/; //25 was good
#endif
    }

    int32_t DSEEnveloppeVolumeTocB( int8_t param )
    {
        assert(false);
        return 0;
    }


//==========================================================================================
//  StreamOperators
//==========================================================================================
    //Global stream operator
    std::ostream & operator<<(std::ostream &os, const DateTime &obj )
    {
        os << static_cast<unsigned long>(obj.year) <<"/" 
           <<static_cast<unsigned long>(obj.month) <<"/" 
           <<static_cast<unsigned long>(obj.day) <<"-" 
           <<static_cast<unsigned long>(obj.hour) <<"h" 
           <<static_cast<unsigned long>(obj.minute) <<"m" 
           <<static_cast<unsigned long>(obj.second) <<"s";
        return os;
    }

    std::ostream & operator<<( std::ostream &  strm, const KeyGroup & other )
    {
        strm <<"\t== Keygroup ==\n"
            << "\tID        : " << other.id                           <<"\n"
            << "\tPolyphony : " << static_cast<short>(other.poly)     <<"\n"
            << "\tPriority  : " << static_cast<short>(other.priority) <<"\n"
            << "\tVc.Low    : " << static_cast<short>(other.vclow)    <<"\n"
            << "\tVc.High   : " << static_cast<short>(other.vchigh)   <<"\n"
            << "\tunk50     : " << static_cast<short>(other.unk50)    <<"\n"
            << "\tunk51     : " << static_cast<short>(other.unk51)    <<"\n";

        return strm;
    }

    std::ostream & operator<<( std::ostream &  strm, const ProgramInfo & other )
    {
        strm << "\t== ProgramInfo ==\n"
             << "\tID        : " << other.m_hdr.id     <<"\n"
             << "\tNbSplits  : " << other.m_hdr.nbsplits <<"\n"
             << "\tVol       : " << static_cast<short>(other.m_hdr.insvol) <<"\n"
             << "\tPan       : " << static_cast<short>(other.m_hdr.inspan) <<"\n"
             << "\tUnk3      : " << other.m_hdr.unk3 <<"\n"
             << "\tUnk4      : " << other.m_hdr.unk4 <<"\n"
             << "\tUnk5      : " << static_cast<short>(other.m_hdr.unk5) <<"\n"
             << "\tnblfos    : " << static_cast<short>(other.m_hdr.nblfos) <<"\n"
             << "\tpadbyte   : " << static_cast<short>(other.m_hdr.padbyte) <<"\n"
             << "\tUnk7      : " << static_cast<short>(other.m_hdr.unk7) <<"\n"
             << "\tUnk8      : " << static_cast<short>(other.m_hdr.unk8) <<"\n"
             << "\tUnk9      : " << static_cast<short>(other.m_hdr.unk9) <<"\n";

        //Write the LFOs
        int cntlfo = 0;
        for( const auto & lfoen : other.m_lfotbl )
        {
            strm << "\t-- LFO #" <<cntlfo <<" --\n"
                << "\tUnk34        : " << static_cast<short>(lfoen.unk34)     <<"\n"
                << "\tUnk52        : " << static_cast<short>(lfoen.unk52)     <<"\n"
                << "\tUnk26        : " << static_cast<short>(lfoen.unk26)     <<"\n"
                << "\tUnk27        : " << static_cast<short>(lfoen.unk27)     <<"\n"
                << "\tUnk28        : " << lfoen.unk28     <<"\n"
                << "\tUnk29        : " << lfoen.unk29     <<"\n"
                << "\tUnk30        : " << lfoen.unk30     <<"\n"
                << "\tUnk31        : " << lfoen.unk31     <<"\n"
                << "\tUnk32        : " << lfoen.unk32     <<"\n"
                << "\tUnk33        : " << lfoen.unk33     <<"\n";
            ++cntlfo;
        }

        //Write the Splits
        int cntsplits = 0;
        for( const auto & split : other.m_splitstbl )
        {
            strm << "\t-- Split #" <<cntlfo <<" --\n"
                << "\tUnk10        : " << static_cast<short>(split.unk10)     <<"\n"
                << "\tID           : " << static_cast<short>(split.id)        <<"\n"
                << "\tUnk11        : " << static_cast<short>(split.unk11)     <<"\n"
                << "\tUnk25        : " << static_cast<short>(split.unk25)     <<"\n"
                << "\tlowkey       : " << static_cast<short>(split.lowkey)    <<"\n"
                << "\thikey        : " << static_cast<short>(split.hikey)     <<"\n"
                << "\tlovel        : " << static_cast<short>(split.lovel)     <<"\n"
                << "\thivel        : " << static_cast<short>(split.hivel)     <<"\n"
                << "\tunk14        : " << static_cast<short>(split.unk14)     <<"\n"
                << "\tunk47        : " << static_cast<short>(split.unk47)     <<"\n"
                << "\tunk15        : " << static_cast<short>(split.unk15)     <<"\n"
                << "\tunk48        : " << static_cast<short>(split.unk48)     <<"\n"
                << "\tunk16        : " << split.unk16     <<"\n"
                << "\tunk17        : " << split.unk17     <<"\n"
                << "\tsmplid       : " << static_cast<short>(split.smplid)     <<"\n"
                << "\ttune         : " << static_cast<short>(split.tune)     <<"\n"
                << "\tcutgrp       : " << static_cast<short>(split.cutgrp)     <<"\n"
                << "\trootkey      : " << static_cast<short>(split.rootkey)   <<"\n"
                << "\tctune        : " << static_cast<short>(split.ctune)     <<"\n"
                << "\tsmplvol      : " << static_cast<short>(split.smplvol)   <<"\n"
                << "\tsmplpan      : " << static_cast<short>(split.smplpan)   <<"\n"
                << "\tkgrpid       : " << static_cast<short>(split.kgrpid)  <<"\n"
                << "\tunk22        : " << static_cast<short>(split.unk22)     <<"\n"
                << "\tunk23        : " << split.unk23     <<"\n"
                << "\tunk24        : " << split.unk24     <<"\n"
                << "\tenvon        : " << static_cast<short>(split.envon)     <<"\n"
                << "\tenvmult      : " << static_cast<short>(split.envmult)     <<"\n"
                << "\tunk37        : " << static_cast<short>(split.unk37)     <<"\n"
                << "\tunk38        : " << static_cast<short>(split.unk38)     <<"\n"
                << "\tunk39        : " << split.unk39     <<"\n"
                << "\tunk40        : " << split.unk40     <<"\n"
                << "\tatkvol       : " << static_cast<short>(split.atkvol)     <<"\n"
                << "\tattack       : " << static_cast<short>(split.attack)     <<"\n"
                << "\tdecay        : " << static_cast<short>(split.decay)     <<"\n"
                << "\tsustain      : " << static_cast<short>(split.sustain)     <<"\n"
                << "\thold         : " << static_cast<short>(split.hold)     <<"\n"
                << "\tdecay2       : " << static_cast<short>(split.decay2)     <<"\n"
                << "\trelease      : " << static_cast<short>(split.release)     <<"\n"
                << "\trx           : " << static_cast<short>(split.rx)     <<"\n"
                ;
            ++cntsplits;
        }

        return strm;
    }

    //SMDL
    std::ostream & operator<<( std::ostream &  strm, const TrkEvent & ev )
    {
        auto info = GetEventInfo( static_cast<eTrkEventCodes>(ev.evcode) );

        //strm
        //     <<setfill(' ') <<setw(6) <<right << static_cast<unsigned short>(ev.dt) <<"t : ";

        if( info.first )
        {
            strm <<"(0x" <<setfill('0') <<setw(2) <<hex <<uppercase <<right <<static_cast<unsigned short>(ev.evcode) <<") : "
                 <<nouppercase <<dec <<setfill(' ') <<setw(16) <<left << info.second.evlbl;

            if( !ev.params.empty() )
            {
                strm <<hex <<uppercase <<"( ";
                for( size_t i = 0; i < ev.params.size(); ++i ) 
                {
                    strm <<"0x" <<setfill('0') <<setw(2) <<right << static_cast<unsigned short>(ev.params[i]);
                    if( i != (ev.params.size() - 1) )
                        strm << ", ";
                } 

                strm <<dec <<nouppercase <<" )";
            }
            else if( ev.evcode >= static_cast<uint8_t>(eTrkEventCodes::Delay_HN) && ev.evcode <= static_cast<uint8_t>(eTrkEventCodes::Delay_64N) )
            {
                strm <<dec << static_cast<unsigned short>(TrkDelayCodeVals.at( ev.evcode )) <<" ticks";
            }
            else
            {
                strm <<"N/A";
            }
        }
        else
        {
            strm <<"ERROR EVENT CODE " <<uppercase <<hex <<static_cast<unsigned short>(ev.evcode) <<dec <<nouppercase;
        }

        strm << "\n" /*<< noshowbase*/;

        return strm;
    }

};