#ifndef SF2_HPP
#define SF2_HPP
/*
sf2.hpp
2015/05/20
psycommando@gmail.com
Description: Utilities for reading and writing SF2 soundfonts files. 
*/
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <cstdint>

namespace io
{



    class SoundFont
    {
    public:
    //--------------------------------
    //  Types Stuff
    //--------------------------------
        typedef size_t sampleid_t;

    public:
    //--------------------------------
    //  Nested Classes Stuff
    //--------------------------------
        //Contains details about a sample to be added to the soundfont
        struct smplinfo
        {
        };

        //Represent a file, and a location within that file.
        struct fileloc
        {
            fileloc( const std::string & path = std::string(), size_t offset = 0 )
                :foffset_(offset), fpath_(path)
            {}

            std::string fpath_;
            size_t      foffset_;
        };

    public:
    //--------------------------------
    //  Constructors
    //--------------------------------
        /*
            path: path to sf2 file to load, or write.
        */
        SoundFont(const std::string & path)
        {
        }

    //--------------------------------
    //  IO Stuff
    //--------------------------------

        /*
            Write
                Write data structures and samples to disk
        */
        void Write();

        /*
            Read
                Parse data structures in memory, samples stays in the file and are loaded on demand.
        */
        void Read();


    //--------------------------------
    //  Samples Stuff
    //--------------------------------
        /*
            GetNbSamples
        */
        size_t GetNbSamples()const;

        /*
            AreSamplesLoaded
                Returns whether the samples are loaded in memory or not.
        */
        bool AreSamplesLoaded()const;

        /*
            AddSample
                Adds sample to the SF2.

                First version adds a block of bytes loaded in memory. It must be a WAV sample.
                The second adds a sample location, whithin a file at a specific offset. It must be a WAV sample.
        */
        sampleid_t AddSample( std::vector<uint8_t> &&smpl,     smplinfo info );
        sampleid_t AddSample( const std::string     &fpath,    size_t foffset,  smplinfo info );

        /*
            RemSample
        */
        void RemSample( sampleid_t index );

        /*
            GetSampleData
                Returns a copy of a loaded sample.
                Otherwise if the samples are unloaded, the data is loaded from file and returned.
        */
        std::vector<uint8_t> GetSampleData( sampleid_t index );

    //--------------------------------
    //  Instruments Stuff
    //--------------------------------

    //--------------------------------
    //  Presets Stuff
    //--------------------------------


    private:
        std::string m_path;

        //This is filled with sample data, if we want to have them loaded in memory
        std::vector<std::vector<uint8_t>> m_smplsdat;
        // Conatins details on the location of each samples. What file, and what offset.
        std::vector<fileloc>              m_smplsfstrs;
    };
};

#endif