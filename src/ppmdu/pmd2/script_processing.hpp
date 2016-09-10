#ifndef SCRIPT_PROCESSING_HPP
#define SCRIPT_PROCESSING_HPP
/*
script_processing.hpp
2016/08/03
psycommando@gmail.com
    Description: Contains utilities for working on script data. Or more precisely, for processing scripts.
*/
//! #NOTE: This is just a work in progress prototype to test out the best approach for the compiler/decompiler.
#include <ppmdu/pmd2/pmd2.hpp>
#include <ppmdu/pmd2/pmd2_scripts_opcodes.hpp>
#include <ppmdu/pmd2/pmd2_configloader.hpp>
#include <ppmdu/containers/script_content.hpp>
#include <memory>


//! #TODO: Complete this?

namespace pmd2
{
//========================================================================================
//  CompileOptions
//========================================================================================
    
    /*
        scriptprocoptions
            A set of generic options for processing script data.
    */
    struct scriptprocoptions
    {
        bool binvertdebug    = false;       //Whether the branching commands relying on the debug state of the engine are to be inverted
        bool bcommentoffsets = false;       //Whether the original instruction offsets will appear as comment in the decompiled output.
        
    };

////========================================================================================
////  IScriptSource
////========================================================================================
//    /*
//        IScriptSource
//            Interface for an object that can parse script data from a 
//            decompiled format into a processable format.
//    */
//    class IScriptSource
//    {
//    public:
//        virtual ~IScriptSource()=0;
//        virtual ScriptSet Parse(const scriptprocoptions&) = 0;
//    };
//
//    /*
//        IScriptLevelSource
//            Interface for an object that can parse script data from a 
//            decompiled format into a processable format.
//    */
//    class IScriptLevelSource
//    {
//    public:
//        virtual ~IScriptLevelSource()=0;
//        virtual LevelScript Parse(const scriptprocoptions&) = 0;
//    };
//
////========================================================================================
////  IScriptDestination
////========================================================================================
//    /*
//        IScriptDestination
//            Interface for an object that can write a processed
//            script set to a decompiled format.
//    */
//    class IScriptDestination
//    {
//    public:
//        virtual ~IScriptDestination()=0;
//        virtual void Write(const ScriptSet &, const scriptprocoptions&) = 0;
//    };
//
//    /*
//        IScriptLevelDestination
//            Interface for an object that can write a processed
//            LevelScript to a decompiled format.
//    */
//    class IScriptLevelDestination
//    {
//    public:
//        virtual ~IScriptLevelDestination()=0;
//        virtual void Write(const LevelScript &, const scriptprocoptions&) = 0;
//    };


//========================================================================================
//  ScriptCompiler
//========================================================================================
    /*
        ScriptCompiler
            Interface for compiling script data into raw byte data to be written to a binary file.
    */
    class ScriptCompiler
    {
        friend class ScriptCompilerImpl;
    public:
        /*
            ScriptCompiler
                - config : The game configuration object containing the relevant data for target
                           game version.
                - common : The script set for the common script routines.
                - options: Options for generic processing.
        */
        ScriptCompiler( const ConfigLoader & config, const ScriptSet & common, const scriptprocoptions & options );
        ~ScriptCompiler();

        /*
            SetCommon
                - common : The script set for the common script routines.
        */
        void SetCommon( const ScriptSet & common );

        /*
            SetConfig
                - config : The game configuration object containing the relevant data for target
                           game version.
        */
        void SetConfig( const ConfigLoader & config );

        /*
            SetOptions
                - options : Sets the generic processing options.
        */
        void SetOptions( const scriptprocoptions & options );

        /*
            CompileSet
                Compile a given ScriptSet. Write the script and script data
                files to the specified directory.
        */
        void CompileSet( const ScriptSet & set, const std::string & outdir );

        /*
            CompileLevel
                Compile a given LevelScript. Write the script, script data, and lsd
                files to the specified directory.
        */
        void CompileLevel( const LevelScript & lvl, const std::string &  outdir );

        /*
            CompileSet
                Compile a given ScriptSet. Write the output files to the 
                specified directory.
                    - setpath : The path to the script data file of the set. SSA, SSE, SSA.
                    - outdir     : Path to directory where to place the decompiled output.
        */
        void CompileSet( const std::string &  setpath, const std::string &  outdir );

        /*
            DecompileLevel
                Compile a given LevelScript. Write the output files to the 
                specified directory.
                    - lvldirpath : Path to directory of the level.
                    - outdir     : Path to directory where to place the decompiled output.
        */
        void CompileLevel( const std::string &  lvldirpath, const std::string &  outdir );

    private:
        std::unique_ptr<ScriptCompilerImpl> m_pimpl;
    };

//========================================================================================
//  ScriptDecompiler
//========================================================================================
    /*
        ScriptDecompiler
            Interface for decompiling script data from raw bytes data from files,
            into an editable format.
    */
    class ScriptDecompiler
    {
        friend class ScriptDecompilerImpl;
    public:
        /*
            ScriptDecompiler
                - config : The game configuration object containing the relevant data for target
                           game version.
                - common : The script set for the common script routines. May be null.
                - options: Options for generic processing.
        */
        ScriptDecompiler( const ConfigLoader & config, const ScriptSet * common, const scriptprocoptions & options );
        ~ScriptDecompiler();

        /*
            SetCommon
                - common : The script set for the common script routines.
        */
        void SetCommon( const ScriptSet & common );

        /*
            SetConfig
                - config : The game configuration object containing the relevant data for target
                           game version.
        */
        void SetConfig( const ConfigLoader & config );

        /*
            SetOptions
                - options : Sets the generic processing options.
        */
        void SetOptions( const scriptprocoptions & options );

        /*
            DecompileSet
                Decompile a given ScriptSet. Write the output files to the 
                specified directory.
        */
        void DecompileSet( const ScriptSet & set, const std::string & outdir );

        /*
            DecompileLevel
                Decompile a given LevelScript. Write the output files to the 
                specified directory.
        */
        void DecompileLevel( const LevelScript & lvl, const std::string & outdir );

        /*
            DecompileSet
                Decompile a given ScriptSet. Write the output files to the 
                specified directory.
                    - setpath : The path to the script data file of the set. SSA, SSE, SSA.
                    - outdir     : Path to directory where to place the decompiled output.
        */
        void DecompileSet( const std::string & setpath, const std::string & outdir );

        /*
            DecompileLevel
                Decompile a given LevelScript. Write the output files to the 
                specified directory.
                    - lvldirpath : Path to directory of the level.
                    - outdir     : Path to directory where to place the decompiled output.
        */
        void DecompileLevel( const std::string & lvldirpath, const std::string & outdir );

    private:
        std::unique_ptr<ScriptDecompilerImpl> m_pimpl;
    };
};

#endif // !SCRIPT_PROCESSING.HPP

