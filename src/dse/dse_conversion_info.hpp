#ifndef DSE_CONVERSION_INFO
#define DSE_CONVERSION_INFO
/*
dse_conversion_info.hpp
2015/10/11
psycommando@gmail.com
Description: Conversion info data used while converting SMDL files into MIDIs.
             
*/
#include <dse/dse_common.hpp>
#include <dse/dse_containers.hpp>
#include <string>
#include <vector>
#include <map>
#include <cstdint>

namespace DSE
{

//================================================================================
//  SMDLPresetConversionInfo
//================================================================================
    /*
        SMDLPresetConversionInfo
            This is used to convert dse tracks into midi.
            It contains details on how to remap notes, what midi preset and bank to use for certain DSE Presets.
    */
    class SMDLPresetConversionInfo
    {
    public:

        /*
            eEffectTy 
                The effects that can be simulated via midi
        */
        enum struct eEffectTy
        {
            Phaser,
            Vibrato,
        };

        /*
            ExtraEffects
                Data for effects that needs to be simulated in the midi!
        */
        struct ExtraEffects
        {
            eEffectTy effty;    //The type of the effect to simulate
            int       rate;     //The LFO rate
            int       delay;    //The delay before the effect kicks in on a key press.
            int       depth;    //The LFO depth
            int       fadeout;  //The delay before the effect fades out.
        };

        /*
            NoteRemapData
                Data for remapping notes to other notes/presets.
        */
        struct NoteRemapData
        {
            midinote_t destnote   = 0;                //The note to use instead
            presetid_t destpreset = InvalidPresetID;  //The preset to use for playing only this note!
            bankid_t   destbank   = InvalidBankID;    //The bank to use for playing only this note!
            uint8_t    idealchan  = UCHAR_MAX;        //The channel the note should be played on!
            uint16_t   origsmplid = 0;                //The original sample ID associated to this note. Only used when writing a cvinfo file
        };

        /*
            PresetConvData
                Information on how to handle certain presets. 
                What preset number to convert it to, in what bank, what keys to remap certain keys to.
                Also contains DSE-specific details resulting from parsing a SMDL + SWDL pair.
        */
        struct PresetConvData
        {
            static const uint8_t Invalid_Chan = UCHAR_MAX;

            PresetConvData( presetid_t presid         = 0, 
                            bankid_t   bank           = 0, 
                            uint8_t    maxpolyphony   = 255,        //255 means default polyphony!
                            uint8_t    prioritygrp    = 0,          //0 means global group!
                            uint32_t   maxkeyduration = 0,          //0 means ignored!
                            int8_t     transposenote  = 0,          //0 means don't transpose the notes
                            uint8_t    preferedchan   = Invalid_Chan)  //255 means no prefered channel!
                :midipres(presid), 
                 midibank(bank), 
                 maxpoly(maxpolyphony), 
                 priority(prioritygrp),
                 maxkeydowndur(maxkeyduration),
                 transpose(0),
                 idealchan(preferedchan)
            {}

            //** Conversion data ***
            presetid_t                          midipres;      //The midi preset to use for this preset
            bankid_t                            midibank;      //The midi bank to use for this preset
            std::map<midinote_t, NoteRemapData> remapnotes;    //Data on how to remap notes for instruments with complex splits, like drum presets
            //*** DSE Specific stuff ***
            std::vector<ExtraEffects>           extrafx;       //Data on any special effecs to be applied midi-side
            uint8_t                             maxpoly;       //The maximum ammount of simultaneous notes allowed for the preset. Previous notes will be turned off.
            uint8_t                             priority;      //The priority value from the DSE keygroup
            //*** Extra conversion stuff ***
            uint32_t                            maxkeydowndur; //The longest note duration allowed in MIDI ticks! Used to get rid of issues caused by notes being held for overly long durations in some SMDL.
            int8_t                              transpose;     //The amount of octaves to transpose the notes played by the instrument. Signed!
            uint8_t                             idealchan;     //The channel to force notes linked to this preset to be played on.
            //*** Optional Extra Data ***
            std::vector<uint16_t>               origsmplids;    //The original sample ids for this preset. Only used when writing a CVinfo file!
        };

        typedef std::map<dsepresetid_t, PresetConvData>::iterator       iterator;
        typedef std::map<dsepresetid_t, PresetConvData>::const_iterator const_iterator;

        inline bool           empty()const { return convtbl.empty(); }

        inline iterator       begin()      { return move(convtbl.begin()); }
        inline const_iterator begin()const { return move(convtbl.begin()); }

        inline iterator       end()        { return move(convtbl.end()); }
        inline const_iterator end()const   { return move(convtbl.end()); }


        /*
            FindConversionInfo
                Returns the iterator to the DSE preset matching the one specified. 
                Returns end() if it was not found, or an iterator to the matching PresetConvData!
        */
        inline iterator       FindConversionInfo( dsepresetid_t presid )      { return convtbl.find(presid); }
        inline const_iterator FindConversionInfo( dsepresetid_t presid )const { return convtbl.find(presid); }
        
        /*
            RemapNote
                Query the table with the specified presetid and specified note, and returns either the note it
                should be converted to, or if there were no key remaps, the same midi note passed as parameter!
        */
        NoteRemapData RemapNote( dsepresetid_t dsep, midinote_t note )const;

        /*
            AddPresetConvInfo
                Adds an entry for the specified DSE preset!
        */
        inline void AddPresetConvInfo( dsepresetid_t dseid, PresetConvData && remapdat )
        {
           convtbl.emplace( std::make_pair( dseid, std::move(remapdat) ) );
        }

        void SetOutputName( const std::string & name ) { outputname = name; }

        const std::string & GetOutputName()const       { return outputname; }
        std::string       & GetOutputName()            { return outputname; }

    private:
        //A list of presets conversion info for each DSE presets specified!
        std::map<dsepresetid_t, PresetConvData> convtbl;
        std::string                             outputname;
    };


//================================================================================
//  SMDLConvInfoDB
//================================================================================
    /*
        SMDLConvInfoDB
            Basically, a collection of several track conversion info, with facilities for making obtaining conversion
            info for a specific file easier.

            Handles parsing the XML file that specifies what to convert DSE programs to, and what notes to convert 
            specific notes to, and etc..
    */
    class SMDLConvInfoDB
    {
    public:
        typedef std::map<std::string, SMDLPresetConversionInfo>::iterator       iterator; 
        typedef std::map<std::string, SMDLPresetConversionInfo>::const_iterator const_iterator; 

        /*
            SMDLConvInfoDB
                -cvinfxml: path to the xml file containing conversion data.
                The file will be parsed on construction.
        */
        SMDLConvInfoDB( const std::string & cvinfxml );
        SMDLConvInfoDB();

        /*
            Parse
                Triggers parsing of the specified xml file!
        */
        void Parse( const std::string & cvinfxml );

        /*
            Write
                Writes a "blank" Conversion Info file, with all the default values for each programs and splits
        */
        void Write( const std::string & cvinfxml );

        /*
            FindConversionInfo
                Find the string under which the particular conversion data is stored under.
        */
        inline iterator       FindConversionInfo( const std::string & name )       { return m_convdata.find(name); }
        inline const_iterator FindConversionInfo( const std::string & name )const  { return m_convdata.find(name); }

        /*
            AddConversionInfo
                Add a conversion info entry
        */
        void AddConversionInfo( const std::string & name, SMDLPresetConversionInfo && info );

        /*
            Required STD methods for using with a C++11 foreach loop.
        */
        inline bool           empty()const { return m_convdata.empty(); }

        inline iterator       begin()      { return move(m_convdata.begin()); }
        inline const_iterator begin()const { return move(m_convdata.begin()); }

        inline iterator       end()        { return move(m_convdata.end()); }
        inline const_iterator end()const   { return move(m_convdata.end()); }

    private:
        std::map<std::string, SMDLPresetConversionInfo> m_convdata;
    };
};

#endif