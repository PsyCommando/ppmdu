#ifndef IMG_PIXEL_HPP
#define IMG_PIXEL_HPP
/*
img_pixel.hpp
2014/12/07
psycommando@gmail.com
Description: flexible pixel type used in tiled_image and linear_image!
*/
#include <cstdint>
#include <cassert>
#include <type_traits>
#include <sstream>
//#include <utils/gbyteutils.hpp>
#include "color.hpp"
//#include <ppmdu/pmd2/pmd2_image_formats.hpp>

namespace gimg
{
//=============================================================================
// ConvertToRGB Function
//=============================================================================
    /*
        ConvertToRGB

    */
    template< class _PIXEL_T >
        inline colorRGB24 ConvertToRGB( _PIXEL_T pixeldata )
    {
        //#REMOVEME: is this even used anymore ?

        _PIXELDATA_T myred   = (pixeldata & _MASK_RED)   >> _BITSHIFT_RED,
                     mygreen = (pixeldata & _MASK_GREEN) >> _BITSHIFT_GREEN,
                     myblue  = (pixeldata & _MASK_BLUE)  >> _BITSHIFT_BLUE;
        return colorRGB24( static_cast<uint8_t>(myred), static_cast<uint8_t>(mygreen), static_cast<uint8_t>(myblue) );
    }

    /*************************************************************************************************
        pixel_RGB_component_indexes
            Use this to get the index of the RGB components for a given pixel type !
            Override by specializing for pixel_t types where the RGB values aren't in this order !
    *************************************************************************************************/
    template<class _PIXEL_T>
        struct pixel_RGB_component_indexes
    {
        static const unsigned int INDEX_RED    = 0;
        static const unsigned int INDEX_GREEN  = 1;
        static const unsigned int INDEX_BLUE   = 2;
    };

//=============================================================================
// Pixeltrait #MOVEME
//=============================================================================

    /*************************************************************************************************
        pixel_trait
            Data for the various kinds of pixel.
    *************************************************************************************************/
    template<unsigned int _BytesPerPixel, unsigned int _BitsPerPixel, bool _IsIndexed = true, unsigned int _nbcomponents = 1u>
        struct pixel_trait
    {
        //Some validation in case invalid parameters are used!
        static_assert( _BytesPerPixel > 0 && _BitsPerPixel > 0 && _nbcomponents > 0, 
                        "_BytesPerPixel, _BitsPerPixel, and/or _nbcomponents cannot be null !" );
        static_assert( _BytesPerPixel <= sizeof(long long), 
                        "Pixel is larger than a 64 bits integer ! Too large to use with this class!" );
        static_assert( _BitsPerPixel <= (_BytesPerPixel * 8), 
                        "Amount of bits per pixel is bigger than the amount of bytes per pixel allows for!" );
        static_assert( _nbcomponents <= _BitsPerPixel, 
                        "Too many components for the ammount of bits available !" );
        static_assert( _nbcomponents <= 4,
                        "Only 4 components and less are fully supported right now!" );

        //This sets the smallest type available as pixel container, to contain the nb of byte(s) per pixel specified, up to 8 bytes
        typedef typename std::conditional< 
                                            _BytesPerPixel == 1, 
                                            uint8_t,                            //If one byte, type to use is uint8
                                            typename std::conditional< 
                                                    _BytesPerPixel == 2,
                                                    uint16_t,                   //If two bytes, type to use is uint16
                                                    typename std::conditional< 
                                                        _BytesPerPixel <= 4,
                                                        uint32_t,               //If 3-4 bytes, type to use is uint32
                                                        uint64_t                //Anything higher uses a uint64
                                                    >::type
                                                >::type  
                                        >::type pixeldata_t; //Type pixel_t is the container type for the pixel data
        //Info on internal storage type
        static const unsigned int  BYTES_SIZE_DATA_CONTAINER = sizeof(pixeldata_t); //Size of the currently used pixel data container

        //Constants with info on the pixel type
        static const unsigned int  BYTES_PER_PIXEL           = _BytesPerPixel;
        static const unsigned int  BITS_PER_PIXEL            = _BitsPerPixel;
        static const unsigned int  NB_COMPONENTS             = _nbcomponents;
        static const unsigned int  BITS_PER_COMPONENTS       = BITS_PER_PIXEL / NB_COMPONENTS;
        static const pixeldata_t   MAX_VALUE_PER_COMPONEMENT = ( utils::do_exponent_of_2_<BITS_PER_COMPONENTS>::value - 1u); 
        static const bool          IS_INDEXED                = _IsIndexed;

        //DataMask
        static const pixeldata_t   MASK_PIXEL_DATA     = ( utils::do_exponent_of_2_<_BitsPerPixel>::value - 1u); //The mask for all components together
        static const pixeldata_t   MASK_COMPONEMENT    = MAX_VALUE_PER_COMPONEMENT; //The base mask for a single component


        static const pixeldata_t & GetAcomponentBitmask( unsigned int componentindex )
        {
            static std::vector<pixeldata_t> ALL_COMPONENT_BITMASKS;

            //If we didn't init this session
            if( ALL_COMPONENT_BITMASKS.size() == 0 )
            {
                //Build the components list
                ALL_COMPONENT_BITMASKS.resize(NB_COMPONENTS,0);

                //Make all the individual components masks
                unsigned int cptinsert = 0;  //Use this to re-order the masks the way the the components are stored
                for( int cptshift = (ALL_COMPONENT_BITMASKS.size() - 1); cptshift >= 0 ; --cptshift, ++cptinsert )
                    ALL_COMPONENT_BITMASKS[cptinsert] = MASK_COMPONEMENT << (cptshift * BITS_PER_COMPONENTS);
            }

            return ALL_COMPONENT_BITMASKS[componentindex];
        }



    };

//=============================================================================
// Pixel #MOVEME
//=============================================================================

    /*************************************************************************************************
        pixel
            A class to represent a pixel of variable size, with one or more components.
    *************************************************************************************************/
    template<class _pixeltrait>
        struct pixel
    {
        typedef pixel<_pixeltrait>                   _myty;
        typedef _pixeltrait                          mypixeltrait_t;
        typedef typename mypixeltrait_t::pixeldata_t pixeldata_t;

    //--------------------------------------------------------------------------------------------------
    // Trait data access
    //--------------------------------------------------------------------------------------------------
        //Whether the pixel has a single component containing a palette offset or not
        inline static bool         IsIndexedPixel()         { return mypixeltrait_t::IS_INDEXED;                }

        //Returnns the maks for the entire data of the pixel
        inline static pixeldata_t  GetPixelDataMask()       { return mypixeltrait_t::MASK_PIXEL_DATA;           }

        //The maximum value that fits within the bits for a component
        inline static pixeldata_t  GetNbComponents()        { return mypixeltrait_t::NB_COMPONENTS;             } 

        //The maximum value that fits within the bits for a component
        inline static pixeldata_t  GetMaxValuePerComponent(){ return mypixeltrait_t::MAX_VALUE_PER_COMPONEMENT; }

        //Get the amount of bits per pixel
        inline static unsigned int GetBitsPerPixel()        { return mypixeltrait_t::BITS_PER_PIXEL;            } 

        //Get the amount of bits per component
        inline static unsigned int GetBitsPerComponent()    { return mypixeltrait_t::BITS_PER_COMPONENTS;       } 

        //Get the amount of bits per pixel
        static unsigned int GetBytesPerPixel()              { return mypixeltrait_t::BYTES_PER_PIXEL;           } 

        //Return the bitmask needed to isolate the value of the desired component
        inline static pixeldata_t  GetAcomponentBitmask( unsigned int indexcomponent ) 
        { return mypixeltrait_t::GetAcomponentBitmask(indexcomponent); }

    //--------------------------------------------------------------------------------------------------
    // Constructor
    //--------------------------------------------------------------------------------------------------
        pixel( pixeldata_t value = 0 )throw()
            :pixeldata(value)
        {}

        pixel( const _myty & other )throw()
        {
            pixeldata = other.pixeldata;
        }

        pixel( _myty && other )throw()
        {
            pixeldata = other.pixeldata;
        }

        _myty & operator=( const _myty & other )throw()
        {
            pixeldata = other.pixeldata;
            return *this;
        }

        _myty & operator=( _myty && other )throw()
        {
            pixeldata = other.pixeldata;
            return *this;
        }

    //--------------------------------------------------------------------------------------------------
    // Accessors
    //--------------------------------------------------------------------------------------------------
        /*************************************************************************************************
            getComponent
                Use this to get the value of a single component.
        *************************************************************************************************/
        inline pixeldata_t getComponent( unsigned int component )const
        {
            if( component >= mypixeltrait_t::NB_COMPONENTS )
            {
                assert( false );//Make sure the component is not out of range !
                std::stringstream strs;
                strs << "pmd2::graphics::pixel::getComponent() : The component to get doesn't exist ! Expected smaller than "
                        << mypixeltrait_t::NB_COMPONENTS << " but got " <<component <<" !";
                throw std::out_of_range(strs.str());
            }

            pixeldata_t  isolateddata = mypixeltrait_t::GetAcomponentBitmask(component) & pixeldata;
            unsigned int shiftamt     = (((mypixeltrait_t::NB_COMPONENTS-1u) - component) * mypixeltrait_t::BITS_PER_COMPONENTS);

            return isolateddata >> shiftamt;
        }

        /*************************************************************************************************
            setComponent
                Use this to set the value of a single component.
        *************************************************************************************************/
        inline void setComponent( unsigned int component, pixeldata_t value )
        {
            const pixeldata_t mask         = mypixeltrait_t::GetAcomponentBitmask(component);
            const pixeldata_t invertedmask = !mask;

            pixeldata = invertedmask & pixeldata;
            pixeldata |= (value << ((mypixeltrait_t::NB_COMPONENTS - component) * mypixeltrait_t::BITS_PER_PIXEL) ) & mask;
        }

        /*************************************************************************************************
            getASingleBit
                Return a single bit from the pixel data's bits.

                Bits are numbered this way:
                0123 4567...
                Higher bits are a smaller value, and lower bits are bigger values.
        *************************************************************************************************/
        inline pixeldata_t getASingleBit( unsigned int index )const
        {
            unsigned int actualshift = (mypixeltrait_t::BITS_PER_PIXEL-1u) - index; //Because the index begins at 0
            return (pixeldata >> actualshift) & 1u;
        }

        /*************************************************************************************************
            getASingleBitInPlace
                Return a single bit from the pixel data's bits, without shifting it all the way 
                to the right, so its at the same position it would be in the pixel data !

                Bits are numbered this way:
                0123 4567...
                Higher bits are a smaller value, and lower bits are bigger values.
        *************************************************************************************************/
        inline pixeldata_t getASingleBitInPlace( unsigned int index )const
        {
            unsigned int actualshift = (mypixeltrait_t::BITS_PER_PIXEL-1u) - index; //Because the index begins at 0
            return pixeldata & (1u << actualshift);
        }

        /*************************************************************************************************
            getWholePixelData
                Returns all the bits of data used by the pixel!
        *************************************************************************************************/
        inline pixeldata_t getWholePixelData()const { return pixeldata; }


        /*************************************************************************************************
            ConvertToRGBColor
                Use this to get a rgb color from a non-indexed pixel! Using this on indexed pixel is
                undefined behavior..
        *************************************************************************************************/
        inline colorRGB24 ConvertToRGBColor()const
        {
            if( !mypixeltrait_t::IS_INDEXED )
            {
                //A couple vars to make things easier to read
                const unsigned int indexRed     = pixel_RGB_component_indexes<mypixeltrait_t>::INDEX_RED,
                                    indexGreen   = pixel_RGB_component_indexes<mypixeltrait_t>::INDEX_GREEN,
                                    indexBlue    = pixel_RGB_component_indexes<mypixeltrait_t>::INDEX_BLUE;

                const pixeldata_t  bitmaskRed   = GetAcomponentBitmask(indexRed),
                                    bitmaskGreen = GetAcomponentBitmask(indexGreen),
                                    bitmaskBlue  = GetAcomponentBitmask(indexBlue),
                                    myred        = (pixeldata & bitmaskRed  ) >> ( indexRed   * GetBitsPerComponent() ),
                                    mygreen      = (pixeldata & bitmaskGreen) >> ( indexGreen * GetBitsPerComponent() ),
                                    myblue       = (pixeldata & bitmaskBlue ) >> ( indexBlue  * GetBitsPerComponent() );

                return colorRGB24( static_cast<uint8_t>(myred), static_cast<uint8_t>(mygreen), static_cast<uint8_t>(myblue) );

            }
            else
            {
                //Just put the index right into each channels, we can't really protect people against themselves!
                // Really, don't call this on an indexed pixel... Unless this is really what you want..
                assert(false); 
                return colorRGB24( mypixeltrait_t::GetAcomponentBitmask(0) & pixeldata,
                                    mypixeltrait_t::GetAcomponentBitmask(0) & pixeldata,
                                    mypixeltrait_t::GetAcomponentBitmask(0) & pixeldata);
            }
        }

    //--------------------------------------------------------------------------------------------------
    // Operators
    //--------------------------------------------------------------------------------------------------
        //Cast operators
        inline operator uint8_t()const  { return static_cast<uint8_t> (pixeldata); }
        inline operator uint16_t()const { return static_cast<uint16_t>(pixeldata); }
        inline operator uint32_t()const { return static_cast<uint32_t>(pixeldata); }
        inline operator uint64_t()const { return static_cast<uint64_t>(pixeldata); }

        /*************************************************************************************************
            operator=
                Use this to assign an integral type directly to the pixel data!
        *************************************************************************************************/
        template<class T> inline _myty & operator=( T val )
        {
            static_assert( std::is_convertible<T, pixeldata_t>::value, "Can't assign type T to pixel! Something is wrong about the type T!" );
            //static_assert( std::numeric_limits<T>::is_integer, "Trying to assign a non-integer to a pixel !" );
            pixeldata = static_cast<pixeldata_t>(val);
            return *this; 
        }

    //--------------------------------------------------------------------------------------------------
    // Arithmetic Operators
    //--------------------------------------------------------------------------------------------------
        /*************************************************************************************************
            operator|=
                Use this to apply a Bitwise OR with an integral type, and assign the result directly to 
                the pixel data!
        *************************************************************************************************/
        template<class T> inline _myty & operator|=( T val )
        {
            static_assert( std::is_convertible<T, pixeldata_t>::value, "Can't assign type T to pixel! Something is wrong about the type T!" );
            pixeldata |= static_cast<pixeldata_t>(val);
            return *this; 
        }

        /*************************************************************************************************
            operator&=
                Use this to apply a Bitwise AND with an integral type, and assign the result directly to 
                the pixel data!
        *************************************************************************************************/
        template<class T> inline _myty & operator&=( T val )
        {
            static_assert( std::is_convertible<T, pixeldata_t>::value, "Can't assign type T to pixel! Something is wrong about the type T!" );
            pixeldata &= static_cast<pixeldata_t>(val);
            return *this;
        }

        /*************************************************************************************************
            operator^=
                Use this to apply a Bitwise XOR with an integral type, and assign the result directly to 
                the pixel data!
        *************************************************************************************************/
        template<class T> inline _myty & operator^=( T val )
        {
            static_assert( std::is_convertible<T, pixeldata_t>::value, "Can't assign type T to pixel! Something is wrong about the type T!" );
            pixeldata ^= static_cast<pixeldata_t>(val);
            return *this;
        }

        /*************************************************************************************************
            operator+=
                Use this to apply addition with an integral type, and assign the result directly to the 
                pixel data!
        *************************************************************************************************/
        template<class T> inline _myty & operator+=( T val )
        {
            static_assert( std::is_convertible<T, pixeldata_t>::value, "Can't assign type T to pixel! Something is wrong about the type T!" );
            pixeldata += static_cast<pixeldata_t>(val);
            return *this;
        }

        /*************************************************************************************************
            operator-=
                Use this to apply subtraction with an integral type, and assign the result directly to 
                the pixel data!
        *************************************************************************************************/
        template<class T> inline _myty & operator-=( T val )
        {
            static_assert( std::is_convertible<T, pixeldata_t>::value, "Can't assign type T to pixel! Something is wrong about the type T!" );
            pixeldata -= static_cast<pixeldata_t>(val);
            return *this;
        }

        /*************************************************************************************************
            operator*=
                Use this to apply multiplication with an integral type, and assign the result directly 
                to the pixel data!
        *************************************************************************************************/
        template<class T> inline _myty & operator*=( T val )
        {
            static_assert( std::is_convertible<T, pixeldata_t>::value, "Can't assign type T to pixel! Something is wrong about the type T!" );
            pixeldata *= static_cast<pixeldata_t>(val);
            return *this;
        }

        /*************************************************************************************************
            operator/=
                Use this to apply division with an integral type, and assign the result directly to the 
                pixel data!
        *************************************************************************************************/
        template<class T> inline _myty & operator/=( T val )
        {
            static_assert( std::is_convertible<T, pixeldata_t>::value, "Can't assign type T to pixel! Something is wrong about the type T!" );
            pixeldata /= static_cast<pixeldata_t>(val);
            return *this;
        }

        /*************************************************************************************************
            operator%=
                Use this to apply division with an integral type, and assign the result directly to the 
                pixel data!
        *************************************************************************************************/
        template<class T> inline _myty & operator%=( T val )
        {
            static_assert( std::is_convertible<T, pixeldata_t>::value, "Can't assign type T to pixel! Something is wrong about the type T!" );
            pixeldata %= static_cast<pixeldata_t>(val);
            return *this;
        }

        template<class T> inline _myty operator&( T val )const
        { return _myty( pixeldata & static_cast<pixeldata_t>(val) ); }

        template<class T> inline _myty operator|( T val )const
        { return _myty( pixeldata | static_cast<pixeldata_t>(val) ); }

        template<class T> inline _myty operator^( T val )const
        { return _myty( pixeldata ^ static_cast<pixeldata_t>(val) ); }

        template<class T> inline _myty operator+( T val )const
        { return _myty( pixeldata + static_cast<pixeldata_t>(val) ); }

        inline _myty & operator++()
        { 
            pixeldata += static_cast<pixeldata_t>(1);
            return *this; 
        }

        template<class T> inline _myty operator-( T val )const
        { return _myty( pixeldata - static_cast<pixeldata_t>(val) ); }

        inline _myty & operator--()
        {
            pixeldata -= static_cast<pixeldata_t>(1);
            return *this; 
        }

        template<class T> inline _myty operator*( T val )const
        { return _myty( pixeldata * static_cast<pixeldata_t>(val) ); }

        template<class T> inline _myty operator/( T val )const
        { return _myty( pixeldata / static_cast<pixeldata_t>(val) ); }

        template<class T> inline _myty operator%( T val )const
        { return _myty( pixeldata % static_cast<pixeldata_t>(val) ); }

        template<class T> inline _myty operator!()const
        { return _myty( !pixeldata ); }

    //--------------------------------------------------------------------------------------------------
    // Data
    //--------------------------------------------------------------------------------------------------
        pixeldata_t pixeldata;
    };

//=============================================================================
//Common pixel traits types
//=============================================================================
    typedef pixel_trait<1,  1>           trait_indexed_1bpp;   //#TODO: 1bpp and anything below 8bpp is pretty space inefficient right now.
    typedef pixel_trait<1,  4>           trait_indexed_4bpp;
    typedef pixel_trait<1,  8>           trait_indexed_8bpp;
    typedef pixel_trait<2, 16>           trait_indexed_16bpp;  //Is it even used ?
    
    typedef pixel_trait<2, 15, false, 3> trait_rgb15;
    typedef pixel_trait<3, 24, false, 3> trait_rgb24;
    typedef pixel_trait<4, 32, false, 4> trait_rgbx32;

//=============================================================================
//Common pixel types
//=============================================================================
    typedef pixel<trait_indexed_1bpp>  pixel_indexed_1bpp;
    typedef pixel<trait_indexed_4bpp>  pixel_indexed_4bpp;
    typedef pixel<trait_indexed_8bpp>  pixel_indexed_8bpp;
    typedef pixel<trait_indexed_16bpp> pixel_indexed_16bpp; //Is it even used ?
    
    typedef pixel<trait_rgb15>         pixel_rgb15;
    typedef pixel<trait_rgb24>         pixel_rgb24;
    typedef pixel<trait_rgbx32>        pixel_rgbx32;
};

#endif