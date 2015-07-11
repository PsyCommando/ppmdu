#include "pmd2_audio_data.hpp"
#include <ppmdu/fmts/dse_common.hpp>
#include <ppmdu/fmts/dse_sequence.hpp>
#include <ppmdu/fmts/dse_interpreter.hpp>

#include <ppmdu/fmts/sedl.hpp>
#include <ppmdu/fmts/smdl.hpp>
#include <ppmdu/fmts/swdl.hpp>
#include <sstream>
#include <iomanip>

#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/DirectoryIterator.h>

#include <ppmdu/ext_fmts/adpcm.hpp>
#include <ppmdu/ext_fmts/sf2.hpp>

using namespace std;
using namespace DSE;

namespace pmd2 { namespace audio
{
//
//
//
    std::vector<pcm16s_t> RawBytesToPCM16Vec( std::vector<uint8_t> * praw )
    {
        std::vector<pcm16s_t> out;
        auto                  itread = praw->begin();
        auto                  itend  = praw->end();
        out.reserve(praw->size()/2);

        while( itread != itend )
            out.push_back( utils::ReadIntFromByteVector<pcm16s_t>( itread ) ); //Iterator is incremented

        return std::move(out);
    }

    std::vector<pcm16s_t> DecodeADPCMAndAppendLoopBuff( const std::vector<uint8_t> & adpcm, size_t loopbufpos, size_t loopbuflen )
    {
        std::vector<pcm16s_t> decompressed = std::move(::audio::DecodeADPCM_IMA(adpcm));
        decompressed.reserve( decompressed.size() + loopbuflen );
        std::copy_n( decompressed.begin() + loopbufpos, loopbuflen, back_inserter(decompressed) );
        return std::move(decompressed);
    }


    class TrackPlaybackState
    {
    public:

        TrackPlaybackState()
            :curpitch(0), curbpm(0), lastsilence(0), curvol(0), curexp(0), curpreset(0), curpan(0),lastpitchev(0)
        {}

        std::string printevent( const TrkEvent & ev )
        {
            stringstream outstr;
        
            //Print Delta Time
            if( ev.dt != 0 )
            {
                outstr <<dec <<static_cast<uint16_t>( DSE::TrkDelayCodeVals.at(ev.dt) )  <<" ticks-";
            }

            auto evinf = GetEventInfo( static_cast<eTrkEventCodes>(ev.evcode) );

            //Print Event Label
            if( evinf.first )
                outstr << evinf.second.evlbl << "-";
            else
                outstr << "INVALID-";

            //Print Parameters and Event Specifics
            if( evinf.second.nbreqparams == 1 )
            {
                if( evinf.second.evcodebeg == eTrkEventCodes::NoteOnBeg )
                {
                    outstr << "( vel:" <<dec << static_cast<unsigned short>(ev.evcode) <<", TrkPitch:";
                    uint8_t prevpitch = curpitch;
                    uint8_t pitchop   = (NoteEvParam1PitchMask & ev.params.front());

                    if( pitchop == static_cast<uint8_t>(eNotePitch::lower) ) 
                        curpitch -= 1;
                    else if( pitchop == static_cast<uint8_t>(eNotePitch::higher) ) 
                        curpitch += 1;
                    else if( pitchop == static_cast<uint8_t>(eNotePitch::reset) ) 
                        curpitch = lastpitchev;

                    outstr <<dec <<static_cast<short>(prevpitch) <<"->" <<dec <<static_cast<short>(curpitch);
                    
                    outstr <<", note: " <<DSE::NoteNames.at( (ev.params.front() & NoteEvParam1NoteMask) );
                    
                    //Ugly but just for debug...
                    //switch( static_cast<eNote>(ev.param1 & NoteEvParam1NoteMask) )
                    //{

                    //    case eNote::C:  outstr <<"C";  break;
                    //    case eNote::Cs: outstr <<"C#"; break;
                    //    case eNote::D:  outstr <<"D";  break;
                    //    case eNote::Ds: outstr <<"D#"; break;
                    //    case eNote::E:  outstr <<"E";  break;
                    //    case eNote::F:  outstr <<"F";  break;
                    //    case eNote::Fs: outstr <<"F#"; break;
                    //    case eNote::G:  outstr <<"G";  break;
                    //    case eNote::Gs: outstr <<"G#";  break;
                    //    case eNote::A:  outstr <<"A";  break;
                    //    case eNote::As: outstr <<"A#"; break;
                    //    case eNote::B:  outstr <<"B";  break;
                    //};
                    outstr <<dec <<static_cast<short>(curpitch) <<" )";
                }
                else if( evinf.second.evcodebeg == eTrkEventCodes::SetOctave )
                {
                    outstr <<"( TrkPitch: ";
                    uint8_t prevpitch = curpitch;
                    lastpitchev = ev.params.front();
                    curpitch    = ev.params.front();
                    outstr <<dec <<static_cast<short>(prevpitch) <<"->" <<dec <<static_cast<short>(curpitch) <<" )";
                }
                else if( evinf.second.evcodebeg == eTrkEventCodes::SetExpress )
                {
                    outstr <<"( TrkExp: ";
                    int8_t prevexp = curexp;
                    curexp = ev.params.front();
                    outstr <<dec <<static_cast<short>(prevexp) <<"->" <<dec <<static_cast<short>(curexp) <<" )";
                }
                else if( evinf.second.evcodebeg == eTrkEventCodes::SetTrkVol )
                {
                    outstr <<"( Vol: ";
                    int8_t prevvol = curvol;
                    curvol = ev.params.front();
                    outstr <<dec <<static_cast<short>(prevvol) <<"->" <<dec <<static_cast<short>(curvol) <<" )";
                }
                else if( evinf.second.evcodebeg == eTrkEventCodes::SetTrkPan )
                {
                    outstr <<"( pan: ";
                    int8_t prevpan = curpan;
                    curpan = ev.params.front();
                    outstr <<dec <<static_cast<short>(prevpan) <<"->" <<dec <<static_cast<short>(curpan) <<" )";
                }
                else if( evinf.second.evcodebeg == eTrkEventCodes::SetPreset )
                {
                    outstr <<"( Prgm: ";
                    uint8_t prevpreset = curpreset;
                    curpreset = ev.params.front();
                    outstr <<dec <<static_cast<unsigned short>(prevpreset) <<"->" <<dec <<static_cast<unsigned short>(curpreset) <<" )";
                }
                else if( evinf.second.evcodebeg == eTrkEventCodes::SetTempo )
                {
                    outstr <<"( tempo: ";
                    int8_t prevbpm = curbpm;
                    curbpm = ev.params.front();
                    outstr <<dec <<static_cast<short>(prevbpm) <<"->" <<dec <<static_cast<short>(curbpm) <<" )";
                }
                else
                    outstr << "( param1: 0x" <<hex <<static_cast<unsigned short>(ev.params.front()) <<" )" <<dec;
            }

            //Print Event with 2 params or a int16 as param
            if( ( evinf.second.nbreqparams == 2 ) )
            {
                if( evinf.second.evcodebeg == eTrkEventCodes::LongPause )
                {
                    uint16_t duration = ( static_cast<uint16_t>(ev.params[1] << 8) | static_cast<uint16_t>(ev.params.front()) );
                    outstr << "( duration: 0x" <<hex <<duration <<" )" <<dec;
                }
                else
                    outstr << "( param1: 0x"  <<hex <<static_cast<unsigned short>(ev.params[0]) <<" , param2: 0x" <<static_cast<unsigned short>(ev.params[1]) <<" )" <<dec;
            }

            //if( evinf.second.evcodebeg == eTrkEventCodes::NoteOnBeg  )
            if( ev.params.size() > 2 )
            {
                //const uint8_t nbextraparams = (ev.params.front() & NoteEvParam1NbParamsMask) >> 6; // Get the two highest bits (1100 0000)

                outstr << "( ";

                for( unsigned int i = 0; i < ev.params.size(); ++i )
                {
                    outstr << "param" <<dec <<i <<": 0x" <<hex <<static_cast<unsigned short>(ev.params[i]) <<dec;

                    if( i != (ev.params.size()-1) )
                        outstr << ",";
                    else
                        outstr <<" )";
                }
                
            }

            return outstr.str();
        }

    private:
        //ittrk_t m_beg;
        //ittrk_t m_loop;
        //ittrk_t m_cur;
        //ittrk_t m_end;

        int8_t   curvol;
        int8_t   curpan;
        int8_t   curexp;
        uint8_t  curpitch;
        uint8_t  lastpitchev;
        uint8_t  curbpm;
        uint8_t  curpreset;
        uint16_t lastsilence;
    };

    std::string MusicSequence::tostr()const
    {
        stringstream sstr;
        int cnt = 0;
        for( const auto & track : m_tracks )
        {
            sstr <<"==== Track" <<cnt << " ====\n\n";
            TrackPlaybackState st;
            for( const auto & ev : track )
            {
                sstr << st.printevent( ev ) << "\n";
            }
            ++cnt;
        }

        return move(sstr.str());
    }

    std::string MusicSequence::printinfo()const
    {
        stringstream sstr;
        sstr << " ==== " <<m_meta.fname <<" ==== \n"
             << "CREATE ITME : " <<m_meta.createtime <<"\n"
             << "NB TRACKS   : " <<m_tracks.size()   <<"\n"
             << "TPQN        : " <<m_meta.tpqn       <<"\n";

        return sstr.str();
    }

//
//  BatchAudioLoader
//

    BatchAudioLoader::BatchAudioLoader( const std::string & mbank )
        :m_mbankpath(mbank)
    {
    }
    
    void BatchAudioLoader::LoadMasterBank()
    {
        m_master = move( LoadSwdBank( m_mbankpath ) );
    }

    void BatchAudioLoader::LoadMasterBank( const std::string & mbank )
    {
        m_mbankpath = mbank;
        LoadMasterBank();
    }

    void BatchAudioLoader::LoadSmdSwdPair( const std::string & smd, const std::string & swd )
    {
        m_pairs.push_back( move( pmd2::audio::LoadSmdSwdPair( smd, swd ) ) );
    }

    /*
        This builds a merged list of all instruments presets from all the swd files loaded.
        vector< vector<InstrumentInfo*>[F][I]  Where F is a swd file in the same order as in the m_pair vector, and I is an instrument in that file.

        It also builds a list where duplicate
    */
    BatchAudioLoader::mergedInstData BatchAudioLoader::PrepareMergedInstrumentTable()const
    {
        std::vector< std::vector<InstrumentInfo*> > merged( m_master.metadata().nbprgislots );
        
        //List of what slot the instruments were put into for each SMD+SWD pair
        std::vector< std::map<uint16_t,uint16_t> > smdlPresLocation (m_pairs.size()); 

        for( size_t cntpair = 0; cntpair < m_pairs.size(); ++cntpair ) //Iterate through all SWDs
        {
            const auto & apair      = m_pairs[cntpair];
            auto         ptrinstinf = apair.second.instbank().lock();
            
            if( ptrinstinf != nullptr )
            {
                const auto & curinstlist = ptrinstinf->instinfo();

                for( size_t cntinst = 0; cntinst < curinstlist.size();  ++cntinst ) //Test all the individual instruments and add them to their slot
                {
                    //Check all presets from all swd files, and see if we have duplicates
                    if( curinstlist[cntinst] != nullptr )
                    {
                        auto founddup = find_if( merged[cntinst].begin(), 
                                                 merged[cntinst].end(), 
                                                 [&]( const InstrumentInfo * inf )->bool
                        { 
                            if( inf != nullptr )
                                return (inf->isSimilar( *(curinstlist[cntinst].get()) ) != InstrumentInfo::eCompareRes::different );
                            else
                            {
                                throw std::exception("BatchAudioLoader::PrepareMergedInstrumentTable(): Null instrument pointer?!");
                                return false;
                            }
                        });

                        if( founddup != merged[cntinst].end() )
                        {
                            smdlPresLocation[cntpair].insert( make_pair( cntinst, distance( merged[cntinst].begin(), founddup) ) );
                        }
                        else
                        {
                            //Push if nothing similar found
                            merged[cntinst].push_back( curinstlist[cntinst].get() );
                            smdlPresLocation[cntpair].insert( make_pair( cntinst, merged[cntinst].size()-1 ) );
                        }
                    }
                    //Don't mark the position of null entries
                }
            }
        }

        return std::move(mergedInstData{ std::move(merged), std::move(smdlPresLocation) });
    }


    /*
        DSEInstrumentToSf2Instrument
            Turns a DSE Preset's "Instrument" into a SF2 Instrument. And add it to the Soundfont!
            - dseinst       : The instruemnt to convert.
            - smplIdConvTbl : A map mapping the Sample IDs from the DSE swd, to their new ID within the Soundfont file!
            - pres          : The SF2 Preset this instrument shall be added to.
    */
    void DSEInstrumentToSf2Instrument( const InstrumentInfo::PrgiSmplEntry & dseinst, 
                                       const std::map<uint16_t,size_t>     & smplIdConvTbl, 
                                       sf2::Preset                         & pres,
                                       const vector<bool>                  & loopedsmpls )
    {
        using namespace sf2;
        //Make the instrument's name
        std::array<char,20> insame;
        sprintf_s( insame.data(), 20, "%s.Inst#%i", pres.GetName().c_str(), dseinst.id ); //Had to use this, as stringstreams are just too slow for this..

        Instrument inst( string( insame.begin(), insame.end() ) );

        //Set the generators
        inst.SetKeyRange( dseinst.lowkey, dseinst.hikey ); //Key range in first

        //#TODO: Add more generators here.

        if( loopedsmpls[dseinst.smplid] )
        {
            inst.SetSmplMode( eSmplMode::loop );
        }

        //Sample ID in last
        inst.SetSampleId( smplIdConvTbl.at(dseinst.smplid) );
        pres.AddInstrument( std::move(inst) );
    }

    /*
        DSEPresetToSf2Preset
            Turns a DSE Preset into a SF2 Preset. And add it to the Soundfont!
            - dsePres       : The Preset to convert.
            - smplIdConvTbl : A map mapping the Sample IDs from the DSE swd, to their new ID within the Soundfont file!
    */
    void DSEPresetToSf2Preset( const std::string               & presname, 
                               uint16_t                          bankno, 
                               const InstrumentInfo            & dsePres, 
                               const std::map<uint16_t,size_t> & smplIdConvTbl, 
                               sf2::SoundFont                  & sf,
                               uint16_t                        & instidcnt,
                               const vector<bool>              & loopedsmpls )
    {
        using namespace sf2;

        Preset pre(presname, dsePres.m_hdr.id, bankno );

        //#1 - Setup Generators
        pre.SetInitAtt( (0x7F - dsePres.m_hdr.insvol) ); //Use the difference between full volume and the current volume to attenuate the preset's volume
                
        // Range of DSE Pan : 0x00 - 0x40 - 0x7F
        // (curpresinf.m_hdr.inspan - 64) * 7.8125f (64 fits 7.8125 times within 500, and 500 is the maximum pan value for soundfont generators)
        double convpan = ((dsePres.m_hdr.inspan - 0x40) * 7.8125); // Remove 64(0x40) to bring the middle to 0.  
        pre.SetPan( lround(convpan) ); 

        //#2 - Setup Modulators

        //#3 - Handle Table 1
        //  #TODO: Handle the data that's in there!!

        //#4 - Convert instruments
        for( uint16_t cntinst = 0; cntinst < dsePres.m_mappedsmpls.size(); ++cntinst )
        {
            const auto & curinst = dsePres.m_mappedsmpls[cntinst];
            DSEInstrumentToSf2Instrument( curinst, smplIdConvTbl, pre, loopedsmpls );

            //Instrument IDs in last
            pre.SetInstrumentId(instidcnt);
            ++instidcnt; //Increase the instrument count
        }

        sf.AddPreset( std::move(pre) );
    }


    BatchAudioLoader::mergedInstData BatchAudioLoader::ExportSoundfont( const std::string & destf )const
    {
        using namespace sf2;
        //First build a master instrument list from all our pairs, with multiple entries per instruments
        mergedInstData        merged = std::move( PrepareMergedInstrumentTable() ); //MSVC is too dumb to trust with implicit move constructor calls..
        SoundFont             sf( m_master.metadata().fname ); 
        map<uint16_t,size_t>  swdsmplofftosf; //Make a map with as key a the sample id in the Wavi table, and as value the sample id in the sounfont!
        

        //Prepare samples list
        auto samples = m_master.smplbank().lock();
        vector<bool>            loopedsmpls( samples->NbSlots(), false ); //Keep track of which samples are looped

        for( size_t cntsmslot = 0; cntsmslot < samples->NbSlots(); ++cntsmslot )
        {
            if( samples->sampleInfo(cntsmslot) != nullptr ) 
            {
                const auto & cursminf = *(samples->sampleInfo(cntsmslot));

                Sample::loadfun_t   loadfun;
                Sample::smplcount_t smpllen = 0;
                Sample::smplcount_t loopbeg = 0;
                Sample::smplcount_t loopend = 0;

                if( cursminf.smplfmt == static_cast<uint16_t>( WavInfo::eSmplFmt::ima_adpcm ) )
                {
                    loadfun = std::move( std::bind( ::audio::DecodeADPCM_IMA, std::ref( samples->sample(cntsmslot) ), 1 ) );
                    smpllen = ::audio::ADPCMSzToPCM16Sz(samples->sample(cntsmslot).size());
                    loopbeg = cursminf.loopspos * 2 + cursminf.looplen * 2;
                    loopend = smpllen;

                    //if( (loopend - loopbeg) > 32 && loopbeg >= 8 )
                    //    loadfun = std::move( std::bind( DecodeADPCMAndAppendLoopBuff, std::ref( samples->sample(cntsmslot) ), cursminf.loopspos * 2, cursminf.looplen * 2 ) );
                    //else
                    //    loadfun = std::move( std::bind( ::audio::DecodeADPCM_IMA, std::ref( samples->sample(cntsmslot) ), 1 ) );
                }
                else if( cursminf.smplfmt == static_cast<uint16_t>( WavInfo::eSmplFmt::pcm ) )
                {
                    loadfun = std::move( std::bind( &RawBytesToPCM16Vec, &(samples->sample(cntsmslot)) ) );
                    smpllen = samples->sample(cntsmslot).size()/2;
                    loopbeg = cursminf.loopspos/2 + cursminf.looplen/2;
                    loopend = smpllen;
                }
                else
                {
                    stringstream sstrerr;
                    sstrerr << "Unknown sample format (0x" <<hex <<uppercase <<cursminf.smplfmt <<nouppercase <<dec  <<") encountered !";
                    throw std::runtime_error( sstrerr.str() );
                }

#ifdef _DEBUG
                if( loopbeg > smpllen || loopend > smpllen )
                    assert(false);
#endif

                Sample sm( std::move( loadfun ), smpllen );
                sm.SetName( "smpl#" + to_string(cntsmslot) );
                sm.SetSampleRate ( cursminf.smplrate );
                sm.SetOriginalKey( cursminf.rootkey );
                sm.SetSampleType ( Sample::eSmplTy::monoSample ); //#TODO: Mono samples only for now !

                if( (loopend - loopbeg) > 32 && loopbeg >= 8 ) //SF2 min loop len
                {
                    sm.SetLoopBounds ( loopbeg, loopend );
                    loopedsmpls[cntsmslot].flip();
                }
                
                swdsmplofftosf.emplace( cntsmslot, sf.AddSample( std::move(sm) ) );
            }
        }

//                if( cursminf.smplfmt == static_cast<uint16_t>( WavInfo::eSmplFmt::ima_adpcm ) )
//                {
//                    //Since our sample is ADPCM, substract the preamble, and multiply by 2!
//                    uint32_t smplsize = ::audio::ADPCMSzToPCM16Sz(samples->sample(cntsmslot).size());// (samples->sample(cntsmslot).size() - 4) * 2; 
//
//
//                    Sample sm( std::bind( ::audio::DecodeADPCM_IMA, std::ref( samples->sample(cntsmslot) ), 1 ), 
//                               smplsize );
//                    
//                    //Make sample info
//                    sm.SetName( "smpl#" + to_string(cntsmslot) );
//                    
//                    //Because the sample is ADPCM encoded, we need to multiply by 2 the sample bounds!!
//                    uint32_t curloopbeg = cursminf.looplen * 2;//cursminf.looplen * 2;//cursminf.loopspos * 2;
//                    uint32_t curloopend = sm.GetDataSampleLength(); //curloopbeg + (cursminf.loopspos * 4);//curloopbeg + (cursminf.loopspos * 2);//( sm.GetDataByteLength()*2 - (cursminf.looplen  * 2) );
//
//                    //#TODO: Re-work validation logic once we fix the logic issues behind Samples..
//
//                    if( /*cursminf.looplen >= 64*/ (curloopend - curloopbeg) >= 64 ) //64 bytes is the minimum loop len
//                    {
//                        //if( (cursminf.loopspos + cursminf.looplen) > sm.GetDataByteLength() )
//                        //{
//                        //    stringstream sstrerr;
//                        //    sstrerr << "BatchAudioLoader::ExportSoundfont(): The loop points of the ADPCM sound sample #" <<cntsmslot <<" are out of range of the sample's data!";
//                        //    throw std::out_of_range(sstrerr.str());
//                        //}
//
//#ifdef _DEBUG
//                        if( curloopbeg > smplsize || curloopend > smplsize )
//                            assert(false);
//#endif
//
//                        sm.SetLoopBounds(
//                            curloopbeg, //beg 
//                            curloopend //end
//                        );
//
//                        //sm.SetLoopBounds(
//                        //(sm.GetDataByteLength() - (cursminf.loopspos + cursminf.looplen) ), //beg 
//                        //(sm.GetDataByteLength() - cursminf.loopspos) //end
//                        //);
//
//                        //sm.SetLoopBounds( cursminf.loopspos, cursminf.loopspos + cursminf.looplen );
//                        loopedsmpls[cntsmslot].flip();
//                    }
//                    
//                    sm.SetSampleType( Sample::eSmplTy::monoSample ); //#TODO: Mono samples only for now !
//                    sm.SetSampleRate( cursminf.smplrate );
//
//                    swdsmplofftosf.emplace( cntsmslot, sf.AddSample( std::move(sm) ) );
//                }
//                else if( cursminf.smplfmt == static_cast<uint16_t>( WavInfo::eSmplFmt::pcm ) )
//                {
//                    Sample sm( std::bind( &RawBytesToPCM16Vec, &(samples->sample(cntsmslot)) ), 
//                               samples->sample(cntsmslot).size() );
//                    
//                    //Make sample info
//                    sm.SetName( "smpl#" + to_string(cntsmslot) );
//
//                    uint32_t curloopbeg = cursminf.looplen;          //Use the value as-is, PCM size never changes, like war ;)
//                    uint32_t curloopend = sm.GetDataSampleLength();
//
//                    //#TODO: Re-work validation logic once we fix the logic issues behind Samples..
//
//                    //if( cursminf.looplen >= 64 ) //64 bytes is the minimum loop len
//                    //{
//                        //if( (cursminf.loopspos + cursminf.looplen) > sm.GetDataByteLength() )
//                        //{
//                        //    stringstream sstrerr;
//                        //    sstrerr << "BatchAudioLoader::ExportSoundfont(): The loop points of the PCM sound sample #" <<cntsmslot <<" are out of range of the sample's data!";
//                        //    throw std::out_of_range(sstrerr.str());
//                        //}
//
//                        //sm.SetLoopBounds(
//                        //(sm.GetDataByteLength() - (cursminf.loopspos + cursminf.looplen) ), //beg
//                        //(sm.GetDataByteLength() - cursminf.loopspos) //end
//                        //);
//#ifdef _DEBUG
//                        if( curloopbeg > samples->sample(cntsmslot).size() || curloopend > samples->sample(cntsmslot).size() )
//                            assert(false);
//#endif
//
//                        sm.SetLoopBounds( curloopbeg, curloopend );
//
//                        loopedsmpls[cntsmslot].flip();
//                    //}
//
//                    sm.SetOriginalKey( cursminf.rootkey );
//                    sm.SetSampleType( Sample::eSmplTy::monoSample ); //#TODO: Mono samples only for now !
//                    sm.SetSampleRate( cursminf.smplrate );
//
//                    swdsmplofftosf.emplace( cntsmslot, sf.AddSample( std::move(sm) ) );
//                }
//                else
//                    cerr <<"Unknown sample format (0x" <<hex <<uppercase <<cursminf.smplfmt <<nouppercase <<dec  <<") encountered !\n" ;
        //    }
        //}

        //Now build the Preset and instrument list !
        auto & presets      = merged.mergedpresets;
        uint16_t instsf2cnt = 0; //Used to assign unique instrument IDs

        for( size_t cntpres = 0; cntpres < presets.size(); ++cntpres )
        {
            for( size_t cntbank = 0; cntbank < presets[cntpres].size(); ++cntbank )
            {
                if( presets[cntpres][cntbank] != nullptr )
                {
                    DSEPresetToSf2Preset( "Preset#" + to_string(cntpres), 
                                          cntbank, 
                                          *(presets[cntpres][cntbank]), 
                                          swdsmplofftosf, 
                                          sf, 
                                          instsf2cnt,
                                          loopedsmpls );
                }
            }

        }

        //Then send that to the Soundfont writing function.

        //Write the soundfont
        sf.Write( destf );

        return std::move(merged);
    }

    void BatchAudioLoader::ExportSoundfontAndMIDIs( const std::string & destdir )const
    {
        //Export the soundfont first
        Poco::Path outsoundfont(destdir);
        outsoundfont.append("bgm.sf2").makeFile();
        cerr<<"Currently exporting main bank to " <<outsoundfont.toString() <<"\n";
        mergedInstData merged = std::move( ExportSoundfont( outsoundfont.toString() ) );

        //Then the MIDIs
        for( size_t i = 0; i < m_pairs.size(); ++i )
        {
            Poco::Path fpath(destdir);
            fpath.append(m_pairs[i].first.metadata().fname);
            fpath.makeFile();
            fpath.setExtension("mid");

            cerr<<"Currently exporting smd to " <<fpath.toString() <<"\n";
            DSE::SequenceToMidi( fpath.toString(), m_pairs[i].first, merged.filetopreset[i] );
        }
    }

    void BatchAudioLoader::ExportSoundfontAsGM( const std::string                               & destf, 
                                                const std::map< std::string, std::vector<int> > & dsetogm )const
    {
    }


//===========================================================================================
//  Functions
//===========================================================================================
    
    std::pair< PresetBank, std::vector<std::pair<MusicSequence,PresetBank>> > LoadBankAndPairs( const std::string & bank, const std::string & smdroot, const std::string & swdroot )
    {
        PresetBank                                  mbank  = DSE::ParseSWDL( bank );
        vector<std::pair<MusicSequence,PresetBank>> seqpairs;

        //We can't know for sure if they're always in the same directory!
        //vector<string> smdfnames;
        //vector<string> swdfnames;
        Poco::DirectoryIterator dirend;
        Poco::File              dirswd( swdroot );
        Poco::File              dirsmd( smdroot );
        vector<Poco::File>      cntdirswd;
        vector<Poco::File>      cntdirsmd;

        //Fill up file lists
        dirswd.list(cntdirswd);
        dirsmd.list(cntdirsmd);
        
        //Find and load all smd/swd pairs!
        for( const auto & smd : cntdirsmd )
        {
            const string smdbasename = Poco::Path(smd.path()).getBaseName();

            //Find matching swd
            vector<Poco::File>::const_iterator itfound = cntdirswd.begin();
            for( ; itfound != cntdirswd.end(); ++itfound )
            {
                if( smdbasename == Poco::Path(itfound->path()).getBaseName() )
                    break;
            }

            if( itfound == cntdirswd.end() )
            {
                cerr<<"Skipping " <<smdbasename <<".smd, because its corresponding " <<smdbasename <<".swd can't be found!\n";
                continue;
            }

            //Parse the files, and push_back the pair
            seqpairs.push_back( make_pair( DSE::ParseSMDL( smd.path() ), DSE::ParseSWDL( itfound->path() ) ) );
        }

        return make_pair( std::move(mbank), std::move(seqpairs) );
    }

    std::pair<MusicSequence,PresetBank> LoadSmdSwdPair( const std::string & smd, const std::string & swd )
    {
        PresetBank    bank = DSE::ParseSWDL( swd );
        MusicSequence seq  = DSE::ParseSMDL( smd );
        return std::make_pair( std::move(seq), std::move(bank) );
    }

    PresetBank LoadSwdBank( const std::string & file )
    {
        return DSE::ParseSWDL( file );
    }
        
    MusicSequence LoadSequence( const std::string & file )
    {
        return DSE::ParseSMDL( file );
    }


};};