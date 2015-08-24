#ifndef DSE_TO_XML_HPP
#define DSE_TO_XML_HPP
/*
dse_to_xml.hpp
2015/07/07
psycommando@gmail.com
Descritpton: Contains utilities for converting DSE audio parameters to XML data.
*/
#include <dse/dse_common.hpp>
#include <dse/dse_sequence.hpp>
#include <ppmdu/pmd2/pmd2_audio_data.hpp>
#include <string>

namespace dse
{

    /*
    */
    void ConvertSWDToXML( const pmd2::audio::PresetBank & bank, const std::string & outfile );

    /*
    */

};

#endif 