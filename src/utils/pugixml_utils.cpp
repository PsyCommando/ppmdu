#include "pugixml_utils.hpp"

namespace pugixmlutils
{
    /*
        HandleParsingError
            If there were no errors while parsing does nothing. Otherwise throws an appropriate exception!
    */
    void HandleParsingError( const pugi::xml_parse_result & result, const std::string & xmlpath )
    {
        using namespace std;
        using namespace pugi;
        if( !result )
        {
            stringstream sstr;
            sstr << "Error, pugixml couldn't parse the XML file \"" <<xmlpath <<"\" !";

            switch( result.status )
            {
                case xml_parse_status::status_file_not_found:
                {
                    sstr<< "The file was not found!";
                    break;
                }
                case xml_parse_status::status_io_error:
                {
                    sstr<< "An error occured while opening the file for reading!";
                    break;
                }
                case xml_parse_status::status_no_document_element:
                {
                    sstr<< "The XML file lacks a document element!";
                    break;
                }
                case xml_parse_status::status_unrecognized_tag:
                {
                    sstr<< "The tag at offset " <<result.offset <<" was unrecognized !";
                    break;
                }
                case xml_parse_status::status_end_element_mismatch:
                {
                    sstr<< "Encountered unexpected end tag at offset " <<result.offset <<" !";
                    break;
                }
                case xml_parse_status::status_bad_start_element:
                {
                    sstr<< "Encountered bad start tag at offset " <<result.offset <<" !";
                    break;
                }
                case xml_parse_status::status_bad_end_element:
                {
                    sstr<< "Encountered bad end tag at offset " <<result.offset <<" !";
                    break;
                }
                case xml_parse_status::status_bad_attribute:
                {
                    sstr<< "Encountered bad attribute at offset " <<result.offset <<" !";
                    break;
                }
                case xml_parse_status::status_bad_cdata:
                {
                    sstr<< "Encountered bad cdata at offset " <<result.offset <<" !";
                    break;
                }
                case xml_parse_status::status_bad_comment:
                {
                    sstr<< "Encountered bad comment at offset " <<result.offset <<" !";
                    break;
                }
                case xml_parse_status::status_bad_doctype:
                {
                    sstr<< "Encountered bad document type at offset " <<result.offset <<" !";
                    break;
                }
                case xml_parse_status::status_bad_pcdata:
                {
                    sstr<< "Encountered bad pcdata at offset " <<result.offset <<" !";
                    break;
                }
                case xml_parse_status::status_bad_pi:
                {
                    sstr<< "Encountered bad document declaration/parsing instructions at offset " <<result.offset <<" !";
                    break;
                }
                case xml_parse_status::status_append_invalid_root:
                {
                    sstr<< "Invalid type of root node at offset " <<result.offset <<" !";
                    break;
                }
            };

            throw runtime_error( sstr.str() );
        }
    }
};