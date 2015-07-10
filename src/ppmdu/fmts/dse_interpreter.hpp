#ifndef DSE_INTERPRETER_HPP
#define DSE_INTERPRETER_HPP
/*
dse_interpreter.hpp
2015/07/01
psycommando@gmail.com
Description: This class is meant to interpret a sequence of DSE audio events into standard MIDI events or text.

License: Creative Common 0 ( Public Domain ) https://creativecommons.org/publicdomain/zero/1.0/
All wrongs reversed, no crappyrights :P
*/
#include <ppmdu/pmd2/pmd2_audio_data.hpp>
#include <ppmdu/fmts/dse_common.hpp>
#include <ppmdu/fmts/dse_sequence.hpp>
#include <vector>
#include <map>
#include <cstdint>

namespace DSE
{

    /*
        eMIDIFormat
            The standard MIDI file format to use to export the MIDI data.
            - SingleTrack : Is format 0, a single track for all events.
            - MultiTrack  : Is format 1, one dedicated tempo track, and all the other tracks for events.
    */
    enum struct eMIDIFormat
    {
        SingleTrack,
        MultiTrack,
    };


    /*
        eMIDIMode
            The MIDI file's "sub-standard".
            - GS inserts a GS Mode reset SysEx event, and then turns the drum channel off.
            - XG insets a XG reset Sysex event.
            - GM doesn't insert any special SysEx events.
    */
    enum struct eMIDIMode
    {
        GM,
        GS,
        XG,
    };

    /*
        -presetbanks : The list for each presets of the bank to use
    */
    void SequenceToMidi( const std::string                 & outmidi, 
                         const pmd2::audio::MusicSequence  & seq, 
                         const std::map<uint16_t,uint16_t> & presetbanks,
                         eMIDIFormat                         midfmt      = eMIDIFormat::MultiTrack,
                         eMIDIMode                           midmode     = eMIDIMode::XG );

    /****************************************************************************************
        IRenderEngine
            Base class for rendering engines available to render a sequence.

            _OutputTy  : The output object to use for outputing the result into 
                          the output stream.
                
            _HandlerTy : The object handling the data format in the sequence to render, 
                          and feeding that to the _OutputTy object.
    ****************************************************************************************/
    //template<class _OutputTy, class _HandlerTy>
    //    class IRenderEngine
    //{
    //public:
    //    virtual ~IRenderEngine() = 0;

    //    //If is true, the engine renders the sequence over time. If false, it renders it instantaneously.
    //    virtual bool isRealTime()const = 0;

    //    //
    //    virtual bool isRendering()const = 0;

    //    //
    //    virtual void Render( typename _OutputTy::outstrm_t & dest ) = 0;

    //    //
    //    virtual void              setOutput( _OutputTy & out ) = 0;
    //    virtual _OutputTy       & getOutput()                  = 0;
    //    virtual const _OutputTy & getOutput()const             = 0;

    //    //
    //    virtual void               setHandler( _HandlerTy & hnlder ) = 0;
    //    virtual _HandlerTy       & getHandler()                      = 0;
    //    virtual const _HandlerTy & getHandler()const                 = 0;
    //};

    ///****************************************************************************************
    //    BaseRenderOutput
    //        Base class for objects that provide a mean of outputing data in a certain 
    //         specific format, using a specific format of data.
    //****************************************************************************************/
    //template<class _MessageTy, class _TimeTy = tick_t>
    //    class BaseRenderOutput
    //{
    //public:
    //    virtual ~BaseRenderOutput()=0;

    //    virtual void Present( _TimeTy ticks, _MessageTy & mess ) = 0;
    //};

    /*
        DSE_Renderer
            Wraper class around the rendering of DSE sequences to other forms.
    */
    //class DSE_Renderer 
    //{
    //public:
    //    DSE_Renderer();

    //    void Render( const pmd2::audio::MusicSequence & seq, std::ostream & out );

    //private:

    //};

};

#endif