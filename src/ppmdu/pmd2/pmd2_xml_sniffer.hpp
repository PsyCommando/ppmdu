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
#include <ppmdu/pmd2/pmd2_levels.hpp>
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


//
//
//
    /*
        SetPPMDU_RootNodeXMLAttributes
            Add to the specified XML node the attributes used on all XML root nodes used with PPMDU
            Those are the game version, game region, and toolset version!
    */
    inline void SetPPMDU_RootNodeXMLAttributes( pugi::xml_node & destnode, eGameVersion ver, eGameRegion reg )
    {
        pugixmlutils::AppendAttribute( destnode, CommonXMLGameVersionAttrStr, GetGameVersionName(ver) );
        pugixmlutils::AppendAttribute( destnode, CommonXMLGameRegionAttrStr,  GetGameRegionNames(reg) );
        pugixmlutils::AppendAttribute( destnode, CommonXMLToolVersionAttrStr, static_cast<std::string>(PMD2ToolsetVersionStruct) );
    }

    /*
        GetPPMDU_RootNodeXMLAttributes
            Get from the specified XML node the attributes used on all XML root nodes used with PPMDU
            Those are the game version, game region, and toolset version!
    */
    inline void GetPPMDU_RootNodeXMLAttributes( const pugi::xml_node & srcnode, eGameVersion & out_ver, eGameRegion & out_reg, std::string & out_toolsetver )
    {
        using namespace pugi;
        xml_attribute   xversion    = srcnode.attribute(CommonXMLGameVersionAttrStr.c_str());
        xml_attribute   xregion     = srcnode.attribute(CommonXMLGameRegionAttrStr.c_str());
        xml_attribute   xppmduv     = srcnode.attribute(CommonXMLToolVersionAttrStr.c_str());
        out_ver = StrToGameVersion(xversion.value());
        out_reg = StrToGameRegion (xregion.value());
        out_toolsetver = xppmduv.value();
    }

//
//
//

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
                                        eGameRegion        & out_reg,
                                        bool                 bprintdetails = true );

    inline bool IsXMLSingleScript(const std::string & rootname)
    {
        return (rootname == ScriptXMLRoot_SingleScript);
    }

    inline bool IsXMLSingleScriptData(const std::string & rootname)
    {
        return (rootname == ScriptDataXMLRoot_SingleDat);
    }

    inline bool IsXMLLevel(const std::string & rootname)
    {
        return (rootname == LevelDataXMLRoot);
    }

    //! #TODO: Add more boolean functions!!
};

#endif
