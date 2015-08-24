#include "txt_palette_io.hpp"
#include <utils/gfileio.hpp>
#include <sstream>
#include <iomanip>
using namespace utils;
using namespace std;

namespace utils{ namespace io
{
    vector<gimg::colorRGB24> ImportFrom_TXT_Palette( const string & inputpath )
    {
        vector<gimg::colorRGB24> colors;
        vector<string>           stringvector = ReadTextFileLineByLine( inputpath );
        colors.reserve( stringvector.size() );
        
        for( const auto & entry : stringvector )
        {
            stringstream sstr;
            if( !entry.empty() && entry.front() == '#' )
            {
                uint32_t entireColor = 0;
                sstr <<hex << entry.substr(1);
                sstr >> entireColor;

                gimg::colorRGB24::colordata_t r = ( (entireColor & 0xFF0000) >> 16 ), 
                                              g = ( (entireColor & 0xFF00  ) >> 8  ), 
                                              b = ( entireColor & 0xFF );

                colors.push_back( gimg::colorRGB24( r, g, b ) );
            }
        }

        colors.shrink_to_fit();
        return std::move( colors );
    }
    
    void ExportTo_TXT_Palette( const vector<gimg::colorRGB24> & in_palette, const string & outputpath )
    {
        vector<string> stringvector;
        stringvector.reserve( in_palette.size() );

        for( const auto & acolor : in_palette )
        {
            stringstream sstrcol;
            sstrcol <<"#" <<setfill('0')
                    <<setw(2) <<hex <<static_cast<unsigned int>(acolor.red) 
                    <<setw(2) <<hex <<static_cast<unsigned int>(acolor.green) 
                    <<setw(2) <<hex <<static_cast<unsigned int>(acolor.blue);
            stringvector.push_back( sstrcol.str() );
        }
        WriteTextFileLineByLine( stringvector, outputpath );
    }


};};