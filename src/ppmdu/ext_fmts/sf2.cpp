#include "sf2.hpp"
#include <ppmdu/ext_fmts/riff.hpp>

#include <iostream>
#include <fstream>
#include <array>
#include <functional>
#include <cassert>
using namespace std;

namespace sf2
{
    static const uint32_t    RIFF_HeaderTotalLen = 12; //bytes The length of the header + the format tag
    static const size_t      SfMinSampleZeroPad  = 46; //Minimum amount of bytes of zero padding to append a sample
    static const uint32_t    SfEntrySHDR_Len     = 46;
    static const uint32_t    SfEntryPHDR_Len     = 38;
    static const uint32_t    SfEntryPBag_Len     = 4;
    static const uint32_t    SfEntryPMod_Len     = 10;
    static const uint32_t    SfEntryPGen_Len     = 4;
    static const uint32_t    SfEntryInst_Len     = 22;
    static const uint32_t    SfEntryIBag_Len     = 4;
    static const uint32_t    SfEntryIMod_Len     = 10;
    static const uint32_t    SfEntryIGen_Len     = 4;
    static const std::string SF_DefIsng          = "EMU8000";

    /*
        Format tags for the chunks used in the SF2 format.
    */
    enum struct eSF2Tags : uint32_t
    {
        //Main Tags
        sfbk = 0x7366626B, //"sfbk" format tag. Identify the RIFF chunk as being a sounfont
        INFO = 0x494E464F, //"INFO" fmt tag. Identify the INFO list chunk.
        sdta = 0x73647461, //"sdta" fmt tag. Identify the sdta list chunk.
        pdta = 0x70647461, //"pdta" fmt tag. Identify the pdta list chunk.

        //Mandatory INFO
        ifil = 0x6966696C, //"ifil" This chunk identify the soundont version this soundfont complies to.
        isng = 0x69736E67, //"isng" 
        INAM = 0x494E414D, //"INAM"

        //Mandatory sdta
        smpl = 0x736D706C, //"smpl"

        //Optionals INFO
        irom = 0x69726F6D, //"irom"
        iver = 0x69766572, //"iver"
        ICRD = 0x49435244, //"ICRD"
        IENG = 0x49454E47, //"IENG"
        IPRD = 0x49505244, //"IPRD"
        ICOP = 0x49434F50, //"ICOP"
        ICMT = 0x49434D54, //"ICMT"
        ISFT = 0x49534654, //"ISFT"

        //Madatory pdta
        phdr = 0x70686472, //"phdr"
        pbag = 0x70626167, //"pbag"
        pmod = 0x706D6F64, //"pmod"
        pgen = 0x7067656E, //"pgen"
        inst = 0x696E7374, //"inst"
        ibag = 0x69626167, //"ibag"
        imod = 0x696D6F64, //"imod"
        igen = 0x6967656E, //"igen"
        shdr = 0x73686472, //"shdr"
    };

//=========================================================================================
//  Structs
//=========================================================================================

    /*
        ifilDat
            Chunk data idicating the SoundFont specification revision that the file 
            complies to.
    */
    struct ifilDat
    {
       static const uint32_t SIZE = 4;//bytes
       ifilDat( uint16_t maj = 2, uint16_t min = 1 )
           :major(maj), minor(min)
       {}

       uint16_t major; //Soundfont 2.01 by default
       uint16_t minor;

        template<class _outit> _outit Write( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToByteVector( major, itwriteto );
            itwriteto = utils::WriteIntToByteVector( minor, itwriteto );
            return itwriteto;
        }


        template<class _init> _init Read( _init itReadfrom )
        {
            itReadfrom = utils::ReadIntFromByteContainer( major, itReadfrom );
            itReadfrom = utils::ReadIntFromByteContainer( minor, itReadfrom );
            return itReadfrom;
        }
    };

//
//  Constants
//
    static const ifilDat SF_VersChnkData( 2, 1 );


//=========================================================================================
//  Utility
//=========================================================================================

    /*
        PrepareSampleData
            Prepare a sample's data for being inserted into a sdata-list chunk.

            If its looped:
                - Copy the sample right before the loop begin sample. Insert it 4 times before 
                  the loop start sample, then 4 times after the loop end sample.
                - The provided loop points position are also ajusted

            For any samples:
                - Adds at least 46 zeros samples after the sample. More if the total nb of 
                  bytes is odd. (Which is hardly even possible with 16 bits pcm)

            Update the loop points passed by reference.
    */
    ///*std::vector<std::pair<uint32_t, uint32_t>>*/void PrepareSampleData( uint32_t              & loopbeg, 
    //                                                                      uint32_t              & loopend,
    //                                                                      std::vector<uint16_t> & samples )
    //{
    //    if( loopend != 0 )
    //    {
    //        vector<uint16_t> newvec;
    //        auto             newvecins = back_inserter( newvec );
    //        size_t           newvecsz = ((SfMinSampleZeroPad + 8 + samples.size()) % 2 != 0)?  //If we get an odd byte count, add one to make it even
    //                                        ((SfMinSampleZeroPad + 8 + samples.size()) + 1) :
    //                                        (SfMinSampleZeroPad + 8 + samples.size());
    //        newvec.reserve( newvecsz );

    //        //Copy the 4 copied samples at the end and at the beginning + the old vector's data
    //        std::fill_n( newvecins, 4, samples[loopbeg] );
    //        std::copy( samples.begin(), samples.end(), newvecins );
    //        std::fill_n( newvecins, 4, samples[loopbeg] );

    //        //Then add the ending zeros
    //        newvec.resize( (newvecsz - newvec.size()), 0 );

    //        samples = std::move(newvec);

    //        //Shift the loop points
    //        loopbeg += 4;
    //        loopend += 4;
    //    }
    //    else
    //    {
    //        //Just append the zeros then
    //        if( samples.size() % 2 == 0 )
    //            samples.resize( samples.size() + SfMinSampleZeroPad, 0 );
    //        else
    //            samples.resize( (samples.size() + SfMinSampleZeroPad + 1), 0 );
    //    }
    //}

    /*
        PrepareSampleLoopPoints
            This method simply copy 8 times the first sample point of the loop. Puts for of those copies
            before loopbeg, and 4 after loopend.
    */
    std::vector<pcm16s_t> PrepareSampleLoopPoints( std::vector<pcm16s_t> && sample, uint32_t loopbeg, uint32_t loopend )
    {
        vector<pcm16s_t> newvec;
        auto             newvecins = back_inserter( newvec );
        size_t           newvecsz  = 8 + sample.size();
        newvec.reserve(newvecsz);

        //Copy the 4 copied samples at the end and at the beginning + the old vector's data
        std::fill_n( newvecins, 4, sample[loopbeg] );
        std::copy( sample.begin(), sample.end(), newvecins );
        std::fill_n( newvecins, 4, sample[loopbeg] );

        //Then add the ending zeros
        /*newvec.resize( (newvecsz - newvec.size()), 0 );*/

        //Shift the loop points accordingly
        //loopbeg += 4;
        //loopend += 4;
        
        //Move over the new smaple data 
        return std::move(newvec);
    }

//=========================================================================================
//  SounFontRIFFWriter
//=========================================================================================
    class SounFontRIFFWriter
    {
        typedef std::function<std::ofstream::streampos()> listmethodfun_t;
        //typedef std::ofstream::streampos(SounFontRIFFWriter::*listmethodfun_t)();
    public:
        SounFontRIFFWriter( const SoundFont & sf )
            :m_sf(sf)
        {}

        //output : stream open in binary mode
        std::ofstream Write( std::ofstream && output )
        {
            m_out = std::move(output);

            //Save pre-write pos to go back to for writing the header later
            const std::ofstream::streampos prewriteoffset = m_out.tellp();

            //Skip the first 8 bytes for the RIFF header
            std::fill_n( std::ostreambuf_iterator<char>(m_out), riff::ChunkHeader::SIZE, 0 );

            //Write fmt tag
            utils::WriteIntToByteVector( static_cast<uint32_t>(eSF2Tags::sfbk), ostreambuf_iterator<char>(m_out), false );

            //Write the content
            std::ofstream::streampos datasz = 4; //Count the 4 bytes of the fmt tag
            datasz += WriteListChunk( static_cast<uint32_t>(eSF2Tags::INFO), std::bind(&SounFontRIFFWriter::WriteInfoList,  this ) );

            //Build and write the sample data
            datasz += WriteListChunk( static_cast<uint32_t>(eSF2Tags::sdta), listmethodfun_t( std::bind(&SounFontRIFFWriter::WriteSdataList, this ) ) );

            //Build and write the HYDRA
            datasz += WriteListChunk( static_cast<uint32_t>(eSF2Tags::pdta), listmethodfun_t( std::bind(&SounFontRIFFWriter::WritePdataList, this ) ) );

            //Seek back to start
            //const std::ofstream::streampos postwrite = m_out.tellp();
            m_out.seekp(prewriteoffset);

            //Write header
            riff::ChunkHeader riffhdr;
            riffhdr.chunk_id = static_cast<uint32_t>(riff::eChunkIDs::RIFF);
            riffhdr.length   = static_cast<uint32_t>(datasz); 
            riffhdr.Write( ostreambuf_iterator<char>(m_out) );

            //Seek back to end before giving back the stream
            m_out.seekp(0, std::ios::end);

            //And done !
            return std::move(m_out);
        }

    private:

        inline std::ofstream::streampos GetCurTotalNbByWritten()
        {
            return (m_out.tellp() - m_prewrite);
        }

    //----------------------------------------------------------------
    //  Chunk Header Writing
    //----------------------------------------------------------------
        /*
            WriteListChunk
                This method writes the header surrounding a list chunk to the stream.
                - fmttag : The fmt tag of the list chunk
                - method : The method to execute to fill up the data of the chunk

                #FIXME: I'm not sure why I couldn't find better than this to wrap each list chunks and re-use the list making code..
                        But writing directly into a stream with this format is a pain in the butt..
        */
        ofstream::streampos WriteListChunk( uint32_t fmttag, listmethodfun_t method )
        {
            //Save pre-write pos
            const std::ofstream::streampos prewrite = m_out.tellp(); //Must be Tellp because we're seeking with this value
            std::ostreambuf_iterator<char> itout(m_out);

            //Skip header size
            itout = std::fill_n( itout, riff::ChunkHeader::SIZE, 0 );
#ifdef _DEBUG
            m_out.flush();
#endif

            //Write format tag
            itout = utils::WriteIntToByteVector( static_cast<uint32_t>(fmttag), itout, false );
#ifdef _DEBUG
            m_out.flush();
#endif

            //Write the sub-chunks
            ofstream::streampos datalen = sizeof(fmttag) + method(); //Count the format tag
#ifdef _DEBUG
            m_out.flush();
#endif

            //Save Post-write pos
            const std::ofstream::streampos postwrite = m_out.tellp();

            //Seek back to start
            m_out.seekp(prewrite);

            //Write header
            riff::ChunkHeader listhdr;
            listhdr.chunk_id = static_cast<uint32_t>(riff::eChunkIDs::LIST);
            listhdr.length   = static_cast<uint32_t>(postwrite - prewrite) - riff::ChunkHeader::SIZE; //Don't count the header itself

            itout = listhdr.Write(itout);

            //Seek back to end
            m_out.seekp(postwrite);

            //Return our total
            return (postwrite - prewrite);
        }

        /*
            WriteChunk
                This method writes the header surrounding a chunk to the stream.
                - tag    : The header tag of the chunk
                - method : The method to execute to fill up the data of the chunk

                #FIXME: I'm not sure why I couldn't find better than this to wrap each list chunks and re-use the list making code..
                        But writing directly into a stream with this format is a pain in the butt..
        */
        ofstream::streampos WriteChunk( eSF2Tags tag, listmethodfun_t method )
        {
            //Save pre-write pos
            const std::ofstream::streampos prewrite = m_out.tellp(); //Must be Tellp because we're seeking with this value
            std::ostreambuf_iterator<char> itout(m_out);

            //Skip header size
            std::fill_n( std::ostreambuf_iterator<char>(m_out), riff::ChunkHeader::SIZE, 0 );

            //Write the sub-chunks
            ofstream::streampos datalen = method();

            //Save Post-write pos
            const std::ofstream::streampos postwrite = m_out.tellp();

            //Seek back to start
            m_out.seekp(prewrite);

            //Write header
            riff::ChunkHeader listhdr;
            listhdr.chunk_id = static_cast<uint32_t>(tag);
            listhdr.length   = static_cast<uint32_t>(postwrite - prewrite) - riff::ChunkHeader::SIZE; //Don't count the header itself

            itout = listhdr.Write(itout);

            //Seek back to end
            m_out.seekp(postwrite);

            //Return our total
            return (postwrite - prewrite);
        }

    //----------------------------------------------------------------
    //  INFO-list
    //----------------------------------------------------------------
        //Write the INFO-list chunk. Returns the nb of bytes written
        ofstream::streampos WriteInfoList()
        {
            //Save pre-write pos
            const std::ofstream::streampos prewrite = m_out.tellp();

            //Write the sub-chunks
            WriteifilChunk();
            WriteisngChunk();
            WriteINAMChunk();

            //Return our total
            return (m_out.tellp() - prewrite);
        }

        /*
            WriteifilChunk
        */
        void WriteifilChunk()
        {
            ostreambuf_iterator<char> itout(m_out);
            riff::ChunkHeader ifilchnk;
            ifilchnk.chunk_id = static_cast<uint32_t>( eSF2Tags::ifil );
            ifilchnk.length   = 4; //Always 4 bytes
            itout = ifilchnk.Write( itout );
            itout = SF_VersChnkData.Write( itout );
        }

        void WriteisngChunk()
        {
            auto              itout = ostreambuf_iterator<char>(m_out);
            riff::ChunkHeader isngchnk;
            isngchnk.chunk_id = static_cast<uint32_t>( eSF2Tags::isng );

            if( m_out.tellp() % 2 != 0 )
                isngchnk.length = SF_DefIsng.size() + 2; //For the extra padding 0 byte
            else
                isngchnk.length = SF_DefIsng.size() + 1;

            //Write the chunk header
            isngchnk.Write( itout );

            //Write the string
            std::copy( SF_DefIsng.begin(), SF_DefIsng.end(), itout );

            //Terminate it with a null character
            m_out.put(0);
            
            //Add extra Zero if ends on non-even byte count
            if( m_out.tellp() % 2 != 0 )
                m_out.put(0);
        }

        void WriteINAMChunk()
        {
            auto              itout = ostreambuf_iterator<char>(m_out);
            riff::ChunkHeader INAMchnk;
            INAMchnk.chunk_id = static_cast<uint32_t>( eSF2Tags::INAM );

            if( m_out.tellp() % 2 != 0 )
                INAMchnk.length = m_sf.GetName().size() + 2; //For the extra padding 0 byte
            else
                INAMchnk.length = m_sf.GetName().size() + 1;

            //Write the chunk header
            INAMchnk.Write( itout );

            //Write the string
            std::copy( m_sf.GetName().begin(), m_sf.GetName().end(), itout );

            //Terminate it with a null character
            m_out.put(0);
            
            //Add extra Zero if ends on non-even byte count
            if( m_out.tellp() % 2 != 0 )
                m_out.put(0);
        }

    //----------------------------------------------------------------
    //  SData-list
    //----------------------------------------------------------------
        //Write the sdta-list chunk. Returns the nb of bytes written
        ofstream::streampos WriteSdataList()
        {
            const std::ofstream::streampos prewrite = GetCurTotalNbByWritten();

            WriteChunk( eSF2Tags::smpl, listmethodfun_t( std::bind(&SounFontRIFFWriter::WriteSmplChunk,  this ) ) );

            return (GetCurTotalNbByWritten() - prewrite);
        }

        ofstream::streampos WriteSmplChunk()
        {
            const std::ofstream::streampos prewrite = GetCurTotalNbByWritten();

            m_smplswritepos.resize(0);
            m_smplswritepos.reserve(m_sf.GetNbSamples());

            //Write the samples
            for( const auto & smpl : m_sf.GetSamples() )
            {
                //Write down position
                std::streampos smplstart = (GetCurTotalNbByWritten() - prewrite);
                
                auto                      loopbounds = smpl.GetLoopBounds();
                vector<pcm16s_t>          data(std::move( PrepareSampleLoopPoints( smpl, loopbounds.first, loopbounds.second ) ));
                ostreambuf_iterator<char> itout(m_out);

                //Write sample data
                for( const auto & point : data )
                {
                    itout = utils::WriteIntToByteVector( static_cast<char>(point),    itout );
                    itout = utils::WriteIntToByteVector( static_cast<char>(point>>8), itout );
                }

                //Save the begining and end position within the sdata chunk before padding
                m_smplswritepos.push_back( make_pair( smplstart, (GetCurTotalNbByWritten() - prewrite) ) );

                if( (m_out.tellp() - prewrite) % 2 == 0 )
                    itout = std::fill_n( itout, SfMinSampleZeroPad, 0 ); //if byte count is even, just add 46 zeros
                else
                    itout = std::fill_n( itout, SfMinSampleZeroPad+1, 0 ); //If byte count is odd, make it even by adding 1
            }

            return (GetCurTotalNbByWritten() - prewrite);
        }

    //----------------------------------------------------------------
    //  HYDRA (PData-List)
    //----------------------------------------------------------------

        //Write the pdta-list chunk. Returns the nb of bytes written
        ofstream::streampos WritePdataList()
        {
            const std::ofstream::streampos prewrite = GetCurTotalNbByWritten();

            //Write the sub-chunks

            //Write Presets
            WriteHYDRAPresets();

            //Write Instruments
            WriteHYDRAInstruments();

            //Write Sample Headers 
            WriteChunk( eSF2Tags::shdr, std::bind(&SounFontRIFFWriter::WriteHYDRASampleHeaders,  this ) );

            return (GetCurTotalNbByWritten() - prewrite);
        }

        ofstream::streampos WriteHYDRAPresets()
        {
            const std::ofstream::streampos prewrite = GetCurTotalNbByWritten();

            //Write phdr chunk
            WriteChunk( eSF2Tags::phdr, listmethodfun_t( std::bind(&SounFontRIFFWriter::WritePHDRChunk,  this ) ) );

            //Write pbag
            WriteChunk( eSF2Tags::pbag, listmethodfun_t(  std::bind(&SounFontRIFFWriter::WritePBagChunk,  this ) ) );

            //Write pmod
            WriteChunk( eSF2Tags::pmod, listmethodfun_t(  std::bind(&SounFontRIFFWriter::WritePModChunk,  this ) ) );

            //Write pgen
            WriteChunk( eSF2Tags::pgen, listmethodfun_t(  std::bind(&SounFontRIFFWriter::WritePGenChunk,  this ) ) );

            return (GetCurTotalNbByWritten() - prewrite);
        }

        ofstream::streampos WriteHYDRAInstruments()
        {
            const std::ofstream::streampos prewrite = GetCurTotalNbByWritten();

            //Write inst chunk
            WriteChunk( eSF2Tags::inst, listmethodfun_t(  std::bind(&SounFontRIFFWriter::WriteInstChunk,  this ) ) );

            //Write ibag chunk
            WriteChunk( eSF2Tags::ibag, listmethodfun_t(  std::bind(&SounFontRIFFWriter::WriteIBagChunk,  this ) ) );

            //Write imod chunk
            WriteChunk( eSF2Tags::imod, listmethodfun_t(  std::bind(&SounFontRIFFWriter::WriteIModChunk,  this ) ) );

            //Write igen chunk
            WriteChunk( eSF2Tags::igen, listmethodfun_t(  std::bind(&SounFontRIFFWriter::WriteIGenChunk,  this ) ) );

            return (GetCurTotalNbByWritten() - prewrite);
        }

        ofstream::streampos WriteHYDRASampleHeaders()
        {
            const std::ofstream::streampos prewrite = GetCurTotalNbByWritten();
            ostreambuf_iterator<char>      itout(m_out);

            //Shift the loop begining and end by 4 !

            for( size_t i = 0; i < m_sf.GetNbSamples(); ++i )
            {
                const auto & smpl        = m_sf.GetSamples()[i];
                const size_t charstocopy = std::min( smpl.GetName().size(), (ShortNameLen-1) ); 
                const size_t charstozero = ShortNameLen - charstocopy;                          //Nb of zeros to append
                auto         loop        = smpl.GetLoopBounds();
                
               
                //Calculate those first, to avoid casting.. 
                const uint32_t smplbeg = static_cast<uint32_t>(m_smplswritepos[i].first);
                const uint32_t smplend = static_cast<uint32_t>(m_smplswritepos[i].second);
                const uint32_t loopbeg = smplbeg + static_cast<uint32_t>(loop.first)  + 4; //Shift by 4, because of the extra data points added for looping
                const uint32_t loopend = smplbeg + static_cast<uint32_t>(loop.second) + 4; //Shift by 4, because of the extra data points added for looping

                //Put sample name + following zeros
                itout = std::copy_n( smpl.GetName().begin(), charstocopy, itout );
                itout = std::fill_n( itout, charstozero, 0 );
                //Put sample start
                itout = utils::WriteIntToByteVector( smplbeg, itout );
                //Put sample end
                itout = utils::WriteIntToByteVector( smplend, itout );
                //Put the sample loop beginning
                itout = utils::WriteIntToByteVector( loopbeg, itout );
                //Put the sample loop end
                itout = utils::WriteIntToByteVector( loopend, itout );
                //Put Sample Rate
                itout = utils::WriteIntToByteVector( smpl.GetSampleRate(), itout );
                //Put Original Pitch
                itout = utils::WriteIntToByteVector( smpl.GetOriginalKey(), itout );
                //Put Pitch Correction
                itout = utils::WriteIntToByteVector( smpl.GetPitchCorrection(), itout );
                //Put Sample Link
                itout = utils::WriteIntToByteVector( smpl.GetLinkedSample(), itout );
                //Put Sample Type
                itout = utils::WriteIntToByteVector( static_cast<uint16_t>(smpl.GetSampleType()), itout );
            }

            //End the list with a zeroed out entry
            static const std::array<char,4> EOSMarker{{'E','O','S',0}};
            itout = std::copy  ( EOSMarker.begin(), EOSMarker.end(),itout );
            itout = std::fill_n( itout, (SfEntrySHDR_Len - EOSMarker.size()), 0 );

            return (GetCurTotalNbByWritten() - prewrite);
        }

    //-----------------------------
    //  Presets
    //-----------------------------
        ofstream::streampos WritePHDRChunk()
        {
            const std::ofstream::streampos prewrite = GetCurTotalNbByWritten();
            ostreambuf_iterator<char>      itout(m_out);

            for( size_t i = 0; i < m_sf.GetNbPresets(); ++i )
            {
                const auto & preset = m_sf.GetPresets()[i];
                const size_t charstocopy = std::min( preset.GetName().size(), (ShortNameLen-1) ); 
                const size_t charstozero = ShortNameLen - charstocopy;                          //Nb of zeros to append

                //Write Name
                itout = std::copy_n( preset.GetName().begin(), charstocopy, itout );
                itout = std::fill_n( itout, charstozero, 0 );
                //Write Preset #
                itout = utils::WriteIntToByteVector( preset.GetPresetNo(), itout );
                //Write Bank #
                itout = utils::WriteIntToByteVector( preset.GetBankNo(), itout );
                //Write Preset Bag Index
                itout = utils::WriteIntToByteVector( static_cast<uint16_t>(i), itout );
                //Write Library 
                itout = utils::WriteIntToByteVector( preset.GetLibrary(), itout );
                //Write Genre
                itout = utils::WriteIntToByteVector( preset.GetGenre(), itout );
                //Write Morphology
                itout = utils::WriteIntToByteVector( preset.GetMorpho(), itout );
            }

            //End the list with a zeroed out entry
            static const std::array<char,4> EOPMarker{{'E','O','P',0}};
            itout = std::copy  ( EOPMarker.begin(), EOPMarker.end(),itout );
            itout = std::fill_n( itout, (SfEntryPHDR_Len - EOPMarker.size()), 0 );

            return (GetCurTotalNbByWritten() - prewrite);
        }

        ofstream::streampos WritePBagChunk()
        {
            const std::ofstream::streampos prewrite = GetCurTotalNbByWritten();
            ostreambuf_iterator<char>      itout(m_out);
            uint16_t                       pgenndx = 0;
            uint16_t                       pmodndx = 0;

            //Write the indexed of each preset's modulator and generators list in the pgen and pmod chunks
            for( size_t i = 0; i < m_sf.GetNbPresets(); ++i )
            {
                const auto & preset = m_sf.GetPresets()[i];

                itout = utils::WriteIntToByteVector( pgenndx, itout );
                itout = utils::WriteIntToByteVector( pmodndx, itout );

                pgenndx += preset.GetNbGenerators();
                pmodndx += preset.GetNbModulators();
            }

            //End the list with a zeroed out entry
            std::fill_n( itout, SfEntryPBag_Len, 0 );

            return (GetCurTotalNbByWritten() - prewrite);
        }

        ofstream::streampos WritePModChunk()
        {
            const std::ofstream::streampos prewrite = GetCurTotalNbByWritten();
            ostreambuf_iterator<char>      itout(m_out);

            //Write the indexed of each preset's modulator and generators list in the pgen and pmod chunks
            for( size_t i = 0; i < m_sf.GetNbPresets(); ++i )
            {
                const auto & preset = m_sf.GetPresets()[i];

                for( size_t cntmod = 0; cntmod < preset.GetNbModulators(); ++cntmod )
                {
                    const auto & modu = preset.GetModulator(cntmod);

                    itout = utils::WriteIntToByteVector( static_cast<uint16_t>(modu.ModSrcOper),    itout );
                    itout = utils::WriteIntToByteVector( static_cast<uint16_t>(modu.ModDestOper),   itout );
                    itout = utils::WriteIntToByteVector( modu.modAmount,                            itout );
                    itout = utils::WriteIntToByteVector( static_cast<uint16_t>(modu.ModAmtSrcOper), itout );
                    itout = utils::WriteIntToByteVector( static_cast<uint16_t>(modu.ModTransOper),  itout );
                }
            }

            //End the list with a zeroed out entry
            std::fill_n( itout, SfEntryPMod_Len, 0 );

            return (GetCurTotalNbByWritten() - prewrite);
        }

        ofstream::streampos WritePGenChunk()
        {
            const std::ofstream::streampos prewrite = GetCurTotalNbByWritten();
            ostreambuf_iterator<char>      itout(m_out);

            //Write the indexed of each preset's modulator and generators list in the pgen and pmod chunks
            for( size_t i = 0; i < m_sf.GetNbPresets(); ++i )
            {
                const auto & preset = m_sf.GetPresets()[i];

                for( const auto & gene : preset.GetGenerators() )
                {
                    itout = utils::WriteIntToByteVector( static_cast<uint16_t>(gene.first), itout );
                    itout = utils::WriteIntToByteVector( gene.second.uword,                 itout );
                }
            }

            //End the list with a zeroed out entry
            std::fill_n( itout, SfEntryPGen_Len, 0 );

            return (GetCurTotalNbByWritten() - prewrite);
        }
    
    //-----------------------------
    //  Instruments
    //-----------------------------
        ofstream::streampos WriteInstChunk()
        {
            const std::ofstream::streampos prewrite = GetCurTotalNbByWritten();
            ostreambuf_iterator<char>      itout(m_out);

            //Write the indexed of each preset's modulator and generators list in the pgen and pmod chunks
            for( size_t i = 0; i < m_sf.GetNbPresets(); ++i )
            {
                const auto & preset = m_sf.GetPreset(i);

                for( size_t cntinst = 0; cntinst < preset.GetNbInstruments(); ++cntinst )
                {
                    const auto & inst = preset.GetInstument(cntinst);

                    const size_t charstocopy = std::min( inst.GetName().size(), (ShortNameLen-1) ); 
                    const size_t charstozero = ShortNameLen - charstocopy;                          //Nb of zeros to append

                    //Write Name
                    itout = std::copy_n( inst.GetName().begin(), charstocopy, itout );
                    itout = std::fill_n( itout, charstozero, 0 );
                    //Write bag index
                    itout = utils::WriteIntToByteVector( static_cast<uint16_t>(cntinst), itout );
                }
            }

            //End the list with a zeroed out entry
            static const std::array<char,4> EOIMarker{{'E','O','I',0}};
            itout = std::copy  ( EOIMarker.begin(), EOIMarker.end(),itout );
            itout = std::fill_n( itout, (SfEntryInst_Len - EOIMarker.size()), 0 );

            return (GetCurTotalNbByWritten() - prewrite);
        }

        ofstream::streampos WriteIBagChunk()
        {
            const std::ofstream::streampos prewrite = GetCurTotalNbByWritten();
            ostreambuf_iterator<char>      itout(m_out);
            uint16_t                       igenndx = 0;
            uint16_t                       imodndx = 0;

            //Write the indexed of each preset's modulator and generators list in the pgen and pmod chunks
            for( size_t i = 0; i < m_sf.GetNbPresets(); ++i )
            {
                const auto & preset = m_sf.GetPreset(i);

                for( size_t cntinst = 0; cntinst < preset.GetNbInstruments(); ++cntinst )
                {
                    const auto & inst = preset.GetInstument(cntinst);

                    itout = utils::WriteIntToByteVector( igenndx, itout );
                    itout = utils::WriteIntToByteVector( imodndx, itout );

                    igenndx += inst.GetNbGenerators();
                    imodndx += inst.GetNbModulators();
                }
            }

            //End the list with a zeroed out entry
            std::fill_n( itout, SfEntryIBag_Len, 0 );

            return (GetCurTotalNbByWritten() - prewrite);
        }

        ofstream::streampos WriteIModChunk()
        {
            const std::ofstream::streampos prewrite = GetCurTotalNbByWritten();
            ostreambuf_iterator<char>      itout(m_out);


            //Write each Modulators for each instruments one after the other
            for( size_t i = 0; i < m_sf.GetNbPresets(); ++i )
            {
                const auto & preset = m_sf.GetPreset(i);

                for( size_t cntinst = 0; cntinst < preset.GetNbInstruments(); ++cntinst )
                {
                    for( const auto & modu : preset.GetInstument(cntinst).GetModulators() )
                    {
                        itout = utils::WriteIntToByteVector( static_cast<uint16_t>(modu.ModSrcOper),    itout );
                        itout = utils::WriteIntToByteVector( static_cast<uint16_t>(modu.ModDestOper),   itout );
                        itout = utils::WriteIntToByteVector( modu.modAmount,                            itout );
                        itout = utils::WriteIntToByteVector( static_cast<uint16_t>(modu.ModAmtSrcOper), itout );
                        itout = utils::WriteIntToByteVector( static_cast<uint16_t>(modu.ModTransOper),  itout );
                    }
                }
            }

            //End the list with a zeroed out entry
            std::fill_n( itout, SfEntryIMod_Len, 0 );

            return (GetCurTotalNbByWritten() - prewrite);
        }

        ofstream::streampos WriteIGenChunk()
        {
            const std::ofstream::streampos prewrite = GetCurTotalNbByWritten();
            ostreambuf_iterator<char>      itout(m_out);

            //Write the generators for every instruments one after the other
            for( size_t i = 0; i < m_sf.GetNbPresets(); ++i )
            {
                const auto & preset = m_sf.GetPreset(i);

                for( size_t cntinst = 0; cntinst < preset.GetNbInstruments(); ++cntinst )
                {
                    for( const auto & gene : preset.GetInstument(cntinst).GetGenerators() )
                    {
                        itout = utils::WriteIntToByteVector( static_cast<uint16_t>(gene.first), itout );
                        itout = utils::WriteIntToByteVector( gene.second.uword,                 itout );
                    }
                }
            }

            //End the list with a zeroed out entry
            std::fill_n( itout, SfEntryIGen_Len, 0 );

            return (GetCurTotalNbByWritten() - prewrite);
        }


        //
        SounFontRIFFWriter( const SounFontRIFFWriter & )           = delete;
        SounFontRIFFWriter& operator=(const SounFontRIFFWriter & ) = delete;        

    private:
        const SoundFont         & m_sf;
        std::ofstream             m_out;
        std::ofstream::streampos  m_prewrite; //The position in the stream before we began writing the entire Soundfont

        //Keep track of where the samples where written, from the beginning of the sounfont structure
        std::vector<std::pair<std::ofstream::streampos,std::ofstream::streampos>> m_smplswritepos;
    };

//=========================================================================================
//  SounFont
//=========================================================================================

    SoundFont::SoundFont()
    {}
    
    SoundFont::SoundFont( const std::string & sfname )
        :m_sfname(sfname)
    {}

    size_t SoundFont::AddSample( Sample && smpl )
    {
        m_samples.push_back( std::move(smpl) );
        return (m_samples.size()-1);
    }

    size_t SoundFont::AddPreset( Preset && preset )
    {
        m_presets.push_back( std::move(preset) );
        return (m_presets.size()-1);
    }

    std::vector<Preset> & SoundFont::GetPresets()
    {
        return m_presets;
    }

    const std::vector<Preset> & SoundFont::GetPresets()const
    {
        return m_presets;
    }

    Preset & SoundFont::GetPreset( size_t index )
    {
        return m_presets[index];
    }

    const Preset & SoundFont::GetPreset( size_t index )const
    {
        return m_presets[index];
    }

    std::vector<Sample> & SoundFont::GetSamples()
    {
        return m_samples;
    }

    const std::vector<Sample> & SoundFont::GetSamples()const
    {
        return m_samples;
    }

    Sample & SoundFont::GetSample( size_t index )
    {
        return m_samples[index];
    }

    size_t SoundFont::Write( const std::string & sf2path )
    {
        ofstream output( sf2path, std::ios::out | std::ios::binary );
        SounFontRIFFWriter sfw(*this);
        output = std::move( sfw.Write( std::move(output) ) );
        return static_cast<size_t>( output.tellp() ); //Stop warning #4244
    }

    size_t SoundFont::Read( const std::string & sf2path )
    {
        assert(false);
        return 0;
    }

//=========================================================================================
//  Sample
//=========================================================================================
    Sample::Sample( const std::string & fpath, size_t begoff, size_t endoff )
        :m_loadty(eLoadType::DelayedFile), m_fpath(fpath), m_begoff(begoff), m_endoff(endoff), m_loopbeg(begoff), m_loopend(endoff)
    {}
    
    Sample::Sample( std::vector<uint8_t> * prawvec, size_t begoff, size_t endoff )
        :m_loadty(eLoadType::DelayedRawVec), m_pRawVec(prawvec), m_begoff(begoff), m_endoff(endoff), m_loopbeg(begoff), m_loopend(endoff)
    {}
    
    Sample::Sample( uint8_t * praw, size_t begoff, size_t endoff )
        :m_loadty(eLoadType::DelayedRaw), m_pRaw(praw), m_begoff(begoff), m_endoff(endoff), m_loopbeg(begoff), m_loopend(endoff)
    {}
    
    Sample::Sample( std::weak_ptr<std::vector<pcm16s_t>> ppcmvec, size_t begoff, size_t endoff )
        :m_loadty(eLoadType::DelayedPCMVec), m_pPcmVec(ppcmvec), m_begoff(begoff), m_endoff(endoff), m_loopbeg(begoff), m_loopend(endoff)
    {}
    
    Sample::Sample( std::weak_ptr<pcm16s_t> ppcm, size_t begoff, size_t endoff )
        :m_loadty(eLoadType::DelayedPCM), m_pPcm(ppcm), m_begoff(begoff), m_endoff(endoff), m_loopbeg(begoff), m_loopend(endoff)
    {}

    Sample::Sample( loadfun_t && funcload, size_t begoff, size_t endoff )
        :m_loadty(eLoadType::DelayedFunc), m_loadfun(std::move(funcload)), m_begoff(begoff), m_endoff(endoff), m_loopbeg(begoff), m_loopend(endoff)
    {}

    /*
        Obtain the data from the sample.
    */
    Sample::operator std::vector<pcm16s_t>()const
    {
        return std::move(Data());
    }

    /*
        Obtain the data from the sample.
        Wrapper over delayed read operations
    */
    std::vector<pcm16s_t> Sample::Data()const
    {
        vector<pcm16s_t> pcmdata;

        switch( m_loadty )
        {
            case eLoadType::DelayedFile:
            {
                const unsigned int datsz = (m_endoff - m_begoff);

                if( datsz % 2 != 0 )
                    throw std::runtime_error("Sample::operator std::vector<pcm16s_t>(): Raw sample data is not a multiple of 2 !");

                ifstream infile( m_fpath, ios::in | ios::binary );
                infile.seekg( m_begoff );

                pcmdata.resize(datsz);

                for( auto  & sample : pcmdata )
                {
                    sample = static_cast<pcm16s_t>( infile.get() );
                    sample |= static_cast<pcm16s_t>( infile.get() ) << 8;
                }
                break;
            }
            case eLoadType::DelayedFunc:
            {
                if( m_begoff == 0 && m_endoff == pcmdata.size() )
                    return std::move( m_loadfun() );
                else
                {
                    pcmdata = std::move( m_loadfun() );

                    if( m_endoff != pcmdata.size() )
                        return std::vector<pcm16s_t>( (pcmdata.begin() + m_begoff), (pcmdata.begin() + m_endoff) );
                    else
                        return std::vector<pcm16s_t>( (pcmdata.begin() + m_begoff), pcmdata.end() );
                }
            }
            case eLoadType::DelayedPCM:
            {
                auto ptr = m_pPcm.lock();

                if( ptr )
                    return std::vector<pcm16s_t>( (ptr.get() + m_begoff), (ptr.get() + m_endoff) );
                else
                {
                    throw std::runtime_error("Sample::operator std::vector<pcm16s_t>(): DelayedPCM loading failed! Pointer has expired, or is null!");
                }
            }
            case eLoadType::DelayedPCMVec:
            {
                auto ptr = m_pPcmVec.lock();

                if( ptr )
                    return std::vector<pcm16s_t>( (ptr.get()->begin() + m_begoff), (ptr.get()->begin() + m_endoff) );
                else
                {
                    throw std::runtime_error("Sample::operator std::vector<pcm16s_t>(): DelayedPCMVec loading failed! Pointer has expired, or is null!");
                }
            }
            case eLoadType::DelayedRaw:
            {
                if( m_pRaw )
                {
                    const size_t datsz = (m_endoff - m_begoff);

                    if( datsz % 2 != 0 )
                        throw std::runtime_error("Sample::operator std::vector<pcm16s_t>(): Raw sample data is not a multiple of 2 !");

                    pcmdata.resize( (datsz / 2) );

                    size_t cntby = 0;
                    for( auto & sample : pcmdata )
                    {
                        sample  = static_cast<pcm16s_t>( *(m_pRaw + cntby) );
                        ++cntby;
                        sample |= static_cast<pcm16s_t>( *(m_pRaw + cntby) ) << 8;
                        ++cntby;
                    }
                }
                else
                {
                    throw std::runtime_error("Sample::operator std::vector<pcm16s_t>(): DelayedRaw loading failed! Pointer has expired, or is null!");
                }
                break;
            }
            case eLoadType::DelayedRawVec:
            {
                if( m_pRawVec )
                {
                    const size_t datsz = (m_endoff - m_begoff);

                    if( datsz % 2 != 0 )
                        throw std::runtime_error("Sample::operator std::vector<pcm16s_t>(): Raw sample data is not a multiple of 2 !");

                    pcmdata.resize( (datsz / 2) );

                    size_t cntby = 0;
                    for( auto & sample : pcmdata )
                    {
                        sample  = static_cast<pcm16s_t>( (*m_pRawVec)[cntby] );
                        ++cntby;
                        sample |= static_cast<pcm16s_t>( (*m_pRawVec)[cntby] ) << 8;
                        ++cntby;
                    }
                }
                else
                {
                    throw std::runtime_error("Sample::operator std::vector<pcm16s_t>(): DelayedRaw loading failed! Pointer has expired, or is null!");
                }
                break;
            }
            default:
            {
                throw std::runtime_error("Sample::operator std::vector<pcm16s_t>(): Unknown loading scheme!");
            }
        };

        return std::move(pcmdata);
    }

    void Sample::SetLoopBounds( size_t beg, size_t end )
    {
        if( (m_begoff + beg) < m_endoff )
            m_loopbeg = beg;
        else
            throw std::out_of_range( "Sample::SetLoopBounds(): Loop beginning position is out of range !" );

        if( (m_begoff + end) <= m_endoff )
            m_loopend = end;
        else
            throw std::out_of_range( "Sample::SetLoopBounds(): Loop end position is out of range !" );
    }


//=========================================================================================
//  Preset
//=========================================================================================

    //---------------
    //  Contructors
    //---------------
    Preset::Preset()
        :m_presetNo(0), m_bankNo(0), m_genre(0), m_library(0), m_morpho(0)
    {}
    
    Preset::Preset( const std::string & name, uint16_t presetno, uint16_t bankno, uint32_t lib, uint32_t genre, uint32_t morpho )
        :m_name(name), m_presetNo(presetno), m_bankNo(bankno), m_library(lib), m_genre(genre), m_morpho(morpho)
    {}

    size_t Preset::AddInstrument( Instrument && inst )
    {
        size_t index = m_instruments.size();
        m_instruments.push_back( std::move(inst) );
        return index;
    }

//=========================================================================================
//  Instrument
//=========================================================================================

    Instrument::Instrument()
    {}
    
    Instrument::Instrument( const std::string & name )
        :m_name(name)
    {}

//=========================================================================================
//  BaseGeneratorUser
//=========================================================================================

    BaseGeneratorUser::genpriority_t BaseGeneratorUser::GetGenPriority( eSFGen gen )const
    {
        if( gen == eSFGen::keyRange )
            return HighPriority;
        else if( gen == eSFGen::velRange )
            return HighPriority-1;
        else if( gen == eSFGen::sampleID )
            return LowPriority + 1; // +1 because an instrument generator makes everything that comes after be ignored
        else if( gen == eSFGen::instrument)
            return LowPriority;
        else
            return DefaultPriority;
    }


    /*
        AddGenerator
            Add a generator value to this instrument.

            A "generator" is mainly an attribute so to speak.
            A sample Id is a generator for instance. 
                
            There can only be a single generator of a given type per instrument.
            Any generator with the same generator type as an existing generator
            will overwrite the later.

            This will also sort the generators in the standard required order !
                
            -> KeyRange Generators always first.
            -> Velocity Range generator can only be preceeded by a Keyrange generator.
            -> SampleID Generators always last.
            -> InstrumentID Generators always last.
    */
    void BaseGeneratorUser::AddGenerator( eSFGen gen, genparam_t value )
    {
        auto empres = m_gens.emplace( make_pair(gen,value) );

        //If insertion failed, overwrite the existing value
        if( !empres.second )
            empres.first->second = value;
    }

    /*
        GetGenerator
            Return a pointer to the specified generator's value, 
            or null if it doesn't exist.
    */
    BaseGeneratorUser::genparam_t * BaseGeneratorUser::GetGenerator( eSFGen gen )
    {
        auto itfound = m_gens.find( gen );

        if( itfound != m_gens.end() )
            return &(itfound->second);
        else
            return nullptr;
    }

    const BaseGeneratorUser::genparam_t * BaseGeneratorUser::GetGenerator( eSFGen gen )const
    {
        auto itfound = m_gens.find( gen );

        if( itfound != m_gens.end() )
            return &(itfound->second);
        else
            return nullptr;
    }

    /*
        GetGenerator
            Return a reference to the specified generator's value.
    */
    BaseGeneratorUser::genparam_t & BaseGeneratorUser::GetGenerator( size_t index )
    {
        if( index < m_gens.size() )
        {
            auto pos = m_gens.begin();
            std::advance( pos, index );
            return pos->second;
        }
        else
            throw std::out_of_range("BaseGeneratorUser::GetGenerator() : Index out of bound !");
    }

    const BaseGeneratorUser::genparam_t & BaseGeneratorUser::GetGenerator( size_t index )const
    {
        if( index < m_gens.size() )
        {
            auto pos = m_gens.begin();
            std::advance( pos, index );
            return pos->second;
        }
        else
            throw std::out_of_range("BaseGeneratorUser::GetGenerator() : Index out of bound !");
    }

    /*
        GetNbGenerators
    */
    size_t BaseGeneratorUser::GetNbGenerators()const
    {
        return m_gens.size();
    }

    //---------------------
    //  Common Generators
    //---------------------
    /*
        Get or Set the sample used by this instrument.
            sampleid : sample index into the SHDR sub-chunk
    */
    void BaseGeneratorUser::SetSampleId( size_t sampleid )
    {
        genparam_t val;
        val.uword = static_cast<uint16_t>(sampleid);
        AddGenerator( eSFGen::sampleID, val );
    }

    std::pair<bool,size_t> BaseGeneratorUser::GetSampleId()const
    {
        auto res = GetGenerator( eSFGen::sampleID );

        if( res != nullptr )
            return make_pair(true, res->uword);
        else
            return make_pair(false, 0);
    }

    /*
        Get or Set the instrument ID used by this preset
            instrumentid : instrument index into the INST sub-chunk

            Returns a pair made of a boolean, and the value of the generator.
            If the boolean is false, there is currently no such generator, and 
            the value returned is thus invalid.
    */
    void BaseGeneratorUser::SetInstrumentId( size_t instrumentid )
    {
        genparam_t val;
        val.uword = static_cast<uint16_t>(instrumentid);
        AddGenerator( eSFGen::instrument, val );
    }

    std::pair<bool,size_t> BaseGeneratorUser::GetInstrumentId()const
    {
        auto res = GetGenerator( eSFGen::instrument );

        if( res != nullptr )
            return make_pair(true, res->uword);
        else
            return make_pair(false, 0);
    }

    /*
        Set or Get the MIDI key range covered by this instrument.
            (def: 0-127, min: 0, max: 127)
    */
    void BaseGeneratorUser::SetKeyRange( int8_t lokey, int8_t hikey )
    {
        genparam_t val;
        val.twosby.by1 = lokey; //MSB is lowest key
        val.twosby.by2 = hikey;
        AddGenerator( eSFGen::keyRange, val );
    }
    
    MidiKeyRange BaseGeneratorUser::GetKeyRange()const
    {
        MidiKeyRange kr;
        auto         res = GetGenerator( eSFGen::keyRange );

        if( res != nullptr )
        {
            kr.lokey = res->twosby.by1;
            kr.hikey = res->twosby.by2;
        }

        return std::move(kr);
    }

    /*
        Set or Get the velocity range covered by this instrument.
            (def: 0-127, min: 0, max: 127)
    */
    void BaseGeneratorUser::SetVelRange( int8_t lokvel, int8_t hivel )
    {
        genparam_t val;
        val.twosby.by1 = lokvel; //MSB is lowest key
        val.twosby.by2 = hivel;
        AddGenerator( eSFGen::velRange, val );
    }

    MidiVeloRange BaseGeneratorUser::GetVelRange()const
    {
        MidiVeloRange vr;
        auto         res = GetGenerator( eSFGen::velRange );

        if( res != nullptr )
        {
            vr.lovel = res->twosby.by1;
            vr.hivel = res->twosby.by2;
        }

        return std::move(vr);
    }

    /*
        Set or Get the volume envelope.
            delay  : timecent (def:-12000 [ < 1ms], min:-12000 [   1ms], max:5000 [ 20sec] )
            attack : timecent (def:-12000 [ < 1ms], min:-12000 [   1ms], max:8000 [100sec] )
            hold   : timecent (def:-12000 [ < 1ms], min:-12000 [   1ms], max:5000 [ 20sec] )
            decay  : timecent (def:-12000 [ < 1ms], min:-12000 [   1ms], max:8000 [100sec] )
            sustain: centibel (def:     0 [   0dB], min:     0 [   0dB], max:1440 [ 144dB] )
            release: timecent (def:-12000 [ < 1ms], min:-12000 [   1ms], max:8000 [100sec] )
    */
    void BaseGeneratorUser::SetVolEnvelope( const Envelope & env )
    {
        genparam_t param;
        param.word = env.delay;
        AddGenerator( eSFGen::delayVolEnv,  param );
        param.word = env.attack;
        AddGenerator( eSFGen::attackVolEnv, param );
        param.word = env.hold;
        AddGenerator( eSFGen::holdVolEnv,   param );
        param.word = env.decay;
        AddGenerator( eSFGen::decayVolEnv,  param );
        param.word = env.sustain;
        AddGenerator( eSFGen::sustainVolEnv,param );
        param.word = env.release;
        AddGenerator( eSFGen::releaseVolEnv,param );
    }

    Envelope BaseGeneratorUser::GetVolEnvelope()const
    {
        auto delay   = GetGenerator( eSFGen::delayVolEnv   );
        auto attack  = GetGenerator( eSFGen::attackVolEnv  );
        auto hold    = GetGenerator( eSFGen::holdVolEnv    );
        auto decay   = GetGenerator( eSFGen::decayVolEnv   );
        auto sustain = GetGenerator( eSFGen::sustainVolEnv );
        auto release = GetGenerator( eSFGen::releaseVolEnv );
        Envelope result;

        if( delay != nullptr )
            result.delay   = delay->word;
        if( attack != nullptr )
            result.attack  = attack->word;
        if( hold != nullptr )
            result.hold    = hold->word;
        if( decay != nullptr )
            result.decay   = decay->word;
        if( sustain != nullptr )
            result.sustain = sustain->word;
        if( release != nullptr )
            result.release = release->word;

        return result;
    }

    /*
        Set or Get the modulation envelope.
            delay  : timecent (def:-12000 [ < 1ms], min:-12000 [   1ms], max:5000 [ 20sec] )
            attack : timecent (def:-12000 [ < 1ms], min:-12000 [   1ms], max:8000 [100sec] )
            hold   : timecent (def:-12000 [ < 1ms], min:-12000 [   1ms], max:5000 [ 20sec] )
            decay  : timecent (def:-12000 [ < 1ms], min:-12000 [   1ms], max:8000 [100sec] )
            sustain:  	-0.1% (def:     0 [  100%], min:     0 [  100%], max:1000 [    0%] )
            release: timecent (def:-12000 [ < 1ms], min:-12000 [   1ms], max:8000 [100sec] )
    */
    void BaseGeneratorUser::SetModEnvelope( const Envelope & env )
    {
        genparam_t param;
        param.word = env.delay;
        AddGenerator( eSFGen::delayModEnv,  param );
        param.word = env.attack;
        AddGenerator( eSFGen::attackModEnv, param );
        param.word = env.hold;
        AddGenerator( eSFGen::holdModEnv,   param );
        param.word = env.decay;
        AddGenerator( eSFGen::decayModEnv,  param );
        param.word = env.sustain;
        AddGenerator( eSFGen::sustainModEnv,param );
        param.word = env.release;
        AddGenerator( eSFGen::releaseModEnv,param );
    }

    Envelope BaseGeneratorUser::GetModEnvelope()const
    {
        auto delay   = GetGenerator( eSFGen::delayModEnv   );
        auto attack  = GetGenerator( eSFGen::attackModEnv  );
        auto hold    = GetGenerator( eSFGen::holdModEnv    );
        auto decay   = GetGenerator( eSFGen::decayModEnv   );
        auto sustain = GetGenerator( eSFGen::sustainModEnv );
        auto release = GetGenerator( eSFGen::releaseModEnv );
        Envelope result;

        if( delay != nullptr )
            result.delay   = delay->word;
        if( attack != nullptr )
            result.attack  = attack->word;
        if( hold != nullptr )
            result.hold    = hold->word;
        if( decay != nullptr )
            result.decay   = decay->word;
        if( sustain != nullptr )
            result.sustain = sustain->word;
        if( release != nullptr )
            result.release = release->word;

        return result;
    }


    /*
        Set or Get the coarse tune.
            (def:0, min:-120, max:120)
            Pitch offset in semitones to be applied to the note.
            Positive means a higher pitch, negative, a lower.
            Its additive with the "FineTune" generator.
    */
    void BaseGeneratorUser::SetCoarseTune( int16_t tune )
    {
        genparam_t val;
        val.word = tune; 
        AddGenerator( eSFGen::coarseTune, val );
    }
    
    int16_t BaseGeneratorUser::GetCoarseTune()const
    {
        auto ctune = GetGenerator( eSFGen::coarseTune );

        if( ctune != nullptr )
            return ctune->word;
        else
            return 0;
    }

    /*
        Set or Get the fine tune.
            (def:0, min:-99, max:99)
            Pitch offset in cents which should be applied to the note. 
            Positive means a higher pitch, negative, a lower.
            Its additive with the "CoarseTune" generator.
                
    */
    void BaseGeneratorUser::SetFineTune( int16_t tune )
    {
        genparam_t val;
        val.word = tune; 
        AddGenerator( eSFGen::fineTune, val );
    }

    int16_t BaseGeneratorUser::GetFineTune()const
    {
        auto ftune = GetGenerator( eSFGen::fineTune );

        if( ftune != nullptr )
            return ftune->word;
        else
            return 0;
    }

    /*
        Set or Get the sample mode. (sample looping)
            (def:0, no loop)
    */
    void BaseGeneratorUser::SetSmplMode( eSmplMode mode )
    {
        genparam_t val;
        val.word = static_cast<uint16_t>(mode); 
        AddGenerator( eSFGen::sampleMode, val );
    }

    eSmplMode BaseGeneratorUser::GetSmplMode()const
    {
        auto smpmode = GetGenerator( eSFGen::sampleMode );

        if( smpmode != nullptr )
            return static_cast<eSmplMode>(smpmode->word);
        else
            return eSmplMode::noloop;
    }

    /*
        Set or Get the Scale Tuning
            (def:100, min:0, max:1200)
            0   : means MIDI key numbers has no effect on pitch.
            100 : means the MIDI key number have full effect on the pitch.

    */
    void BaseGeneratorUser::SetScaleTuning( uint16_t scale )
    {
        genparam_t val;
        val.uword = scale; 
        AddGenerator( eSFGen::scaleTuning, val );
    }

    uint16_t BaseGeneratorUser::GetScaleTuning()const
    {
        auto scaletune = GetGenerator( eSFGen::scaleTuning );

        if( scaletune != nullptr )
            return scaletune->uword;
        else
            return 100;
    }

    /*
        Set or Get the Initial Attenuation
            (def:0, min:0, max:1440)
            The attenuation in centibels applied to the note.
            0  == no attenuation
            60 == attenuated by 6dB
    */
    void BaseGeneratorUser::SetInitAtt( uint16_t att )
    {
        genparam_t val;
        val.uword = att; 
        AddGenerator( eSFGen::initialAttenuation, val );
    }

    uint16_t BaseGeneratorUser::GetInitAtt()const
    {
        auto initatt = GetGenerator( eSFGen::initialAttenuation );

        if( initatt != nullptr )
            return initatt->uword;
        else
            return 0;
    }

    /*
        Set or Get the Pan
            (def:0 center, min:-500 left, max: 500 right)
            The pan in 0.1% applied to the note. 
    */
    void BaseGeneratorUser::SetPan( int16_t pan )
    {
        genparam_t val;
        val.word = pan; 
        AddGenerator( eSFGen::pan, val );
    }

    int16_t BaseGeneratorUser::GetPan()const
    {
        auto pan = GetGenerator( eSFGen::pan );

        if( pan != nullptr )
            return pan->word;
        else
            return 0;
    }

    /*
        Set or Get the Exclusive Class id
            (def:0, min:0, max:127)
            Basically, instruments  within the same
            Preset, with the same Exclusive Class Id cut eachother
            when they play.
    */
    void BaseGeneratorUser::SetExclusiveClass( uint16_t id )
    {
        genparam_t val;
        val.uword = id; 
        AddGenerator( eSFGen::exclusiveClass, val );
    }

    uint16_t BaseGeneratorUser::GetExclusiveClass()const
    {
        auto exclass = GetGenerator( eSFGen::exclusiveClass );

        if( exclass != nullptr )
            return exclass->uword;
        else
            return 0;
    }

    /*
        Set or Get the Reverb Send
            (def:0, min:0, max:1000)
            The amount of reverb effect in 0.1% applied to the note. 
            1000 == 100%
            http://www.pjb.com.au/midi/sfspec21.html#g16
    */
    void BaseGeneratorUser::SetReverbSend( uint16_t send )
    {
        genparam_t val;
        val.uword = send; 
        AddGenerator( eSFGen::reverbEffectsSend, val );
    }

    uint16_t BaseGeneratorUser::GetReverbSend()const
    {
        auto send = GetGenerator( eSFGen::reverbEffectsSend );

        if( send != nullptr )
            return send->uword;
        else
            return 0;
    }

    /*
        Set or Get the Chorus Send
            (def:0, min:0, max:1000)
            The amount of chorus effect in 0.1% applied to the note. 
            1000 == 100%
            http://www.pjb.com.au/midi/sfspec21.html#g15
    */
    void BaseGeneratorUser::SetChorusSend( uint16_t send )
    {
        genparam_t val;
        val.uword = send; 
        AddGenerator( eSFGen::chorusEffectsSend, val );
    }

    uint16_t BaseGeneratorUser::GetChorusSend()const
    {
        auto send = GetGenerator( eSFGen::chorusEffectsSend );

        if( send != nullptr )
            return send->uword;
        else
            return 0;
    }

//=========================================================================================
//  BaseModulatorUser
//=========================================================================================

    /*
        AddModulator
            Return modulator index in this instrument's list.
    */
    size_t BaseModulatorUser::AddModulator( SFModZone && mod )
    {
        //Search for a modulator with the same set of 
        // ModSrcOper, ModDestOper, and ModSrcAmtOper
        for( size_t i = 0; i < m_mods.size(); ++i )
        {
            auto & exmod = m_mods[i];

            //If we find a modulator with the same 3 defining parameters
            if( exmod.ModSrcOper    == mod.ModSrcOper  &&
                exmod.ModDestOper   == mod.ModDestOper &&
                exmod.ModAmtSrcOper == mod.ModAmtSrcOper )
            {
                //Then just affect the other values, as we can't have more than once the same combination of defining paramenters
                exmod.modAmount    = mod.modAmount;
                exmod.ModTransOper = mod.ModTransOper;
                return i;
            }
        }

        //If we didn't get a match, add it to the list
        m_mods.push_back( std::move(mod) );
        return (m_mods.size()-1);
    }

    /*
        GetModulator
            Return the modulator at the index specified.
    */
    SFModZone & BaseModulatorUser::GetModulator( size_t index )
    {
        return m_mods[index];
    }

    const SFModZone & BaseModulatorUser::GetModulator( size_t index )const
    {
        return m_mods[index];
    }

    /*
        GetNbModulators
    */
    size_t BaseModulatorUser::GetNbModulators()const
    {
        return m_mods.size();
    }


    /*
        template<class _init>
            _init ReadIGENEntries( _init itbeg, _init itend )const
        {
            bool bgotkeyrange = false;
            bool isglobalzone = false; 
            assert(false); //#TODO: Need to write code to detect if this is a global zone !

            while( itbeg != itend )
            {
                eSFGen     gen   = utils::ReadIntFromByteContainer<uint16_t>(itbeg);
                genparam_t param = utils::ReadIntFromByteContainer<uint16_t>(itbeg);

                //Any Key Range generator must be the first in the list! Or be ignored.
                if( (gen == eSFGen::keyRange) )
                {
                    if( m_gens.size() > 0 )
                    {
                        cerr << "<!>- Ignored Key Range generator not at the top of the generator list!\n";
                        continue;
                    }
                    else
                        bgotkeyrange = true;
                }

                //Any present Velocity Range generator must be only preceeded by a Key Range generator ! Or be ignored.
                if( (gen == eSFGen::velRange) && (m_gens.size() > 1) && !bgotkeyrange )
                {
                    cerr << "<!>- Ignored Velocity Range generator not at the top, or after a Key Range generator, in the generator list!\n";
                    continue;
                }

                //Non-global zone must end with a SampleID generator, or be ignored!
                if( itbeg == itend && !isglobalzone && gen != eSFGen::sampleID )
                {
                    throw std::runtime_error("Instrument::ReadIGENEntries(): Non-global Instrument generator zone doesn't end with a sample ID generator!\n");
                }

                if( m_gens.find(gen) != m_gens.end() )
                    m_gens.insert( gen, param );
                else
                    throw std::runtime_error( "Instrument::ReadIGENEntries(): Duplicate Generator entry encountered!" );
            }
            return itbeg;
        }

        template<class _init>
            _init ReadIMODEntries( _init itbeg, _init itend )const
        {
            while( itbeg != itend )
            {
                SFModZone curzone = SFModZone(); //Refresh object state after move
                curzone.ModSrcOper    = utils::ReadIntFromByteContainer<uint16_t>(itbeg); //iter incremented
                curzone.ModDestOper   = utils::ReadIntFromByteContainer<uint16_t>(itbeg); //iter incremented
                curzone.modAmount     = utils::ReadIntFromByteContainer<int16_t> (itbeg); //iter incremented
                curzone.ModAmtSrcOper = utils::ReadIntFromByteContainer<int16_t> (itbeg); //iter incremented
                curzone.ModTransOper  = utils::ReadIntFromByteContainer<int16_t> (itbeg); //iter incremented
                m_mods.push_back(std::move(curzone));
            }
            return itbeg;
        }*/

};