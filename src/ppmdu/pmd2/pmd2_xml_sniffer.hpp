#ifndef PMD2_XML_SNIFFER_HPP
#define PMD2_XML_SNIFFER_HPP
/*
pmd2_xml_sniffer.hpp
2016/08/22
psycommando@gmail.com
Description: A helper class for parsing the root node of a XML file exported by this library.
*/
#include <ppmdu/pmd2/pmd2.hpp>
#include <ppmdu/fmts/ssb.hpp>
#include <ppmdu/fmts/ssa.hpp>
#include <ppmdu/pmd2/pmd2_scripts.hpp>
#include <string>
#include <unordered_map>
#include <iostream>
#include <pugixml.hpp>
#include <utils/pugixml_utils.hpp>

namespace pmd2
{
    struct RootNodeInfo
    {
        std::string                                  name;
        std::unordered_map<std::string, std::string> attributes;
    };

    inline RootNodeInfo GetRootNodeFromXML( const std::string & file )
    {
        using namespace pugi;
        xml_document doc;
        pugixmlutils::HandleParsingError( doc.load_file(file.c_str()), file );
        RootNodeInfo rinf;
        rinf.name = doc.document_element().name();
        for( const auto & attr : doc.document_element().attributes() )
            rinf.attributes.emplace( attr.name(), attr.value() );
        return std::move(rinf);
    }

    void CheckGameVersionAndGameRegion( const RootNodeInfo & rootnode, 
                                        eGameVersion       & out_ver, 
                                        eGameRegion        & out_reg )
    {
        if( rootnode.attributes.size() >= 2 )
        {
            cout <<"<!>-XML Script file detected!\n";
            try
            {
                out_ver = pmd2::StrToGameVersion(rootnode.attributes.at(CommonXMLGameVersionAttrStr));
                out_reg  = pmd2::StrToGameRegion (rootnode.attributes.at(CommonXMLGameRegionAttrStr));
            }
            catch(const std::exception&)
            {
                throw_with_nested(std::logic_error("CheckGameVersionAndGameRegion(): Couldn't get the game version and or game region values attributes from document root!!"));
            }
            cout <<"<*>-Detected Game Version : " <<pmd2::GetGameVersionName(out_ver) <<"\n"
                 <<"<*>-Detected Game Region  : " <<pmd2::GetGameRegionNames(out_reg)  <<"\n";

            if( out_ver == eGameVersion::Invalid )
                throw std::invalid_argument("CheckGameVersionAndGameRegion(): Game version attribute in root node is invalid!");
            if( out_reg == eGameRegion::Invalid )
                throw std::invalid_argument("CheckGameVersionAndGameRegion(): Game region attribute in root node is invalid!");
        }
        else
            throw std::invalid_argument("CheckGameVersionAndGameRegion(): Game region and version attribute in root node are missing!");
    }

    inline bool IsXMLSingleScript(const std::string & rootname)
    {
        return (rootname == ScriptXMLRoot_SingleScript);
    }

    inline bool IsXMLSingleScriptData(const std::string & rootname)
    {
        return (rootname == ScriptDataXMLRoot_SingleDat);
    }

    //! #TODO: Add more boolean functions!!
};

#endif
