#ifndef MAPPA_HPP
#define MAPPA_HPP
/*
mappa.hpp
2016/05/18
psycommando@gmail.com
Description: Utilities for parsing the several mappa files from PMD2.
*/
#include <ppmdu/fmts/sir0.hpp>
#include <ppmdu/containers/dungeon_rng_data.hpp>

namespace filetypes
{
    const std::string DEF_MAPPA_FILENAME_PREFIX = "mappa_";
    const std::string DEF_MAPPA_FILENAME_SUFFIX = ".bin";

//========================================================================================
//  Mappa header
//========================================================================================
    /***********************************************************************
        mappa_subhdr
            This is what the first pointer in the SIR0 header points to.
            5, 32 bits pointer to various places in the data.
    ***********************************************************************/
    struct mappa_subhdr
    {
        static const size_t SIZE = 20;  //bytes
        uint32_t ptrDungeonLUT       = 0;
        uint32_t ptrFloorDataBlock   = 0;
        uint32_t ptrItemSpawnListLUT = 0;
        uint32_t ptrPkmnSpwnLUT      = 0;
        uint32_t ptrTrapsLUT         = 0;


        //Write the structure using an iterator to a byte container
        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( ptrDungeonLUT,          itwriteto );
            itwriteto = utils::WriteIntToBytes( ptrFloorDataBlock,      itwriteto );
            itwriteto = utils::WriteIntToBytes( ptrItemSpawnListLUT,    itwriteto );
            itwriteto = utils::WriteIntToBytes( ptrPkmnSpwnLUT,         itwriteto );
            itwriteto = utils::WriteIntToBytes( ptrTrapsLUT,            itwriteto );
            return itwriteto;
        }

        //Read the structure from an iterator on a byte container
        template<class _init>
            _init ReadFromContainer( _init itReadfrom, _init itend )
        {
            itReadfrom = utils::ReadIntFromBytes( ptrDungeonLUT,          itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( ptrFloorDataBlock,      itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( ptrItemSpawnListLUT,    itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( ptrPkmnSpwnLUT,         itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( ptrTrapsLUT,            itReadfrom, itend );
            return itReadfrom;
        }
    };

//========================================================================================
//  Mappa_g header
//========================================================================================

    /*
        mappa_g[s,t,y].bin sub-header.
    */
    struct mappa_g_subhdr
    {
        static const size_t SIZE = 8;  //bytes
        uint32_t ptrdungeonslut = 0;
        uint32_t ptrfloordata = 0;


        //Write the structure using an iterator to a byte container
        template<class _outit>
            _outit WriteToContainer(_outit itwriteto)const
        {
            itwriteto = utils::WriteIntToBytes(ptrdungeonslut, itwriteto);
            itwriteto = utils::WriteIntToBytes(ptrfloordata, itwriteto);
            return itwriteto;
        }

        //Read the structure from an iterator on a byte container
        template<class _init>
            _init ReadFromContainer(_init itReadfrom, _init itend)
        {
            itReadfrom = utils::ReadIntFromBytes(ptrdungeonslut,itReadfrom, itend);
            itReadfrom = utils::ReadIntFromBytes(ptrfloordata,itReadfrom, itend);
            return itReadfrom;
        }
    };

//========================================================================================
//  Functions
//========================================================================================
    
    /*
		Loads a mappa file set. Basically each unique mappa file has its own mappa_g file containing extra info.
    */
    pmd2::stats::DungeonRNGDataSet LoadMappaSetEoS ( const std::string & fnamemappa, 
										      const std::string & fnamemappag );

    /*
        Loads a mappa file from the EoTD games
    */
    pmd2::stats::DungeonRNGDataSet LoadMappaSetEoTD ( const std::string & fnamemappa);

    /*
		Writes a mappa data set to a mappa and mappa_g file.
    */
    void                    WriteMappaSetEoS( const std::string                     & fnamemappa, 
										      const std::string			            & fnamemappag, 
                                              const pmd2::stats::DungeonRNGDataSet  & mappaset );
    /*
        Writes a mappa file for the EoTD games
    */
    void                    WriteMappaSetEoTD( const std::string                    & fnamemappa, 
                                               const pmd2::stats::DungeonRNGDataSet & mappaset );

};
#endif
