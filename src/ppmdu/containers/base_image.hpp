#ifndef BASE_IMAGE_HPP
#define BASE_IMAGE_HPP
/*
base_image.hpp
2014/12/08
psycommando@gmail.com
Description: Base class for tiled_image and linear_image.

*/
#include "img_pixel.hpp"
#include "index_iterator.hpp"
#include <utils/utility.hpp>
#include <vector>
#include <array>
#include <algorithm>
#include <iostream>

namespace gimg
{
//=============================================================================
// Base Image
//=============================================================================

    /*************************************************************************************************
        base_image

    *************************************************************************************************/
    template<class _Child, class _PIXEL_T>
        class base_image
    {
    public:
        typedef _Child           child_t;
        typedef _PIXEL_T         pixel_t;

        //------ Methods ------
        //Access the image data like a linear 1D array
        virtual pixel_t & operator[]( unsigned int pos ) = 0;

        //Access the image data like a linear 1D array
        virtual const pixel_t & operator[]( unsigned int pos )const = 0;

        //Access the image data like a 2D bitmap
        virtual pixel_t & getPixel( unsigned int x, unsigned int y ) = 0;

        //Access the image data like a 2D bitmap
        virtual const pixel_t & getPixel( unsigned int x, unsigned int y )const = 0;

        //Set the image resolution in pixels. Must be divisible by 8!
        virtual void setPixelResolution( unsigned int pixelsWidth, unsigned int pixelsHeigth ) = 0;

        //Set the image resolution in pixels. Must be divisible by 8!
        virtual void setPixelResolution( const utils::Resolution & res )
        {
            setPixelResolution( res.width, res.height );
        }

        //Set the image pixel resolution //#FIXME : Compiler refuses to acknowledge this method in direct children of this class for some reasons..
        //virtual void resize( const utils::Resolution & res )
        //{
        //    resize( res.width, res.height );
        //}

        //Set the image pixel resolution
        virtual void resize( unsigned int width, unsigned int height ) = 0;

        //Implementation for Non-indexed images
        inline virtual colorRGB24 getPixelRGBColor( unsigned int x, unsigned int y )const
        {
            return getPixel(x,y).ConvertToRGBColor();
        }

        //Implementation for Non-indexed images
        inline virtual colorRGB24 getPixelRGBColor( unsigned int linearpixelindex )const
        {
            return (*this)[linearpixelindex].ConvertToRGBColor();
        }

        //Get sizes and stuff
        virtual unsigned int getNbRows()const        = 0;   //#FIXME: Its very unclear that those are meant for Tiles!!!
        virtual unsigned int getNbCol()const         = 0;   //#FIXME: Its very unclear that those are meant for Tiles!!!

        virtual unsigned int getNbPixelWidth()const  = 0;   //#FIXME: We should stick with width and height..
        virtual unsigned int getNbPixelHeight()const = 0;   //#FIXME: We should stick with width and height..

        virtual unsigned int width()const            = 0;
        virtual unsigned int height()const           = 0;

        virtual unsigned int getTotalNbPixels()const = 0;
        virtual bool         empty()const            = 0; //Whether the pixel size is non-zero!
        virtual unsigned int size()const             = 0; //Size, in PIXELS. For iterator and etc..

        //This returns the exact amount of bits that each pixels in the image uses
        virtual unsigned int getSizeInBits()const    = 0;

        //Access to the iterators
        //virtual iterator       begin() throw()       = 0;
        //virtual const_iterator begin() const throw() = 0;
        //virtual iterator       end()   throw()       = 0;
        //virtual const_iterator end()   const throw() = 0;
    };

//=============================================================================
// Base Indexed Image
//=============================================================================

    /*************************************************************************************************
        base_indexed_image
            Generic parent for images using indexed pixels.
    *************************************************************************************************/
    template<class _Color_T, class _PIXEL_T>
        class base_indexed_image 
    {
    public:
        //typedef _PIXEL_T pixel_t; //#FIXED: This would conflict in multi-inheritance situations
        typedef _Color_T pal_color_t;

        //------ Methods ------
        //Accessors
        virtual std::vector<pal_color_t>       & getPalette()                                         = 0;
        virtual const std::vector<pal_color_t> & getPalette()const                                    = 0;
        virtual const pal_color_t              & getColor( typename _PIXEL_T::pixeldata_t index )const = 0;
        virtual pal_color_t                    & getColor( typename _PIXEL_T::pixeldata_t index )      = 0;
        virtual unsigned int                     getNbColors()const                                   = 0;
        virtual void                             setNbColors(unsigned int nbcol)                      = 0;
        virtual void setColor( typename _PIXEL_T::pixeldata_t index, pal_color_t && color )            = 0;
        virtual void setColor( typename _PIXEL_T::pixeldata_t index, pal_color_t color )               = 0;

        //Get the color of a pixel at (X, Y) directly
        virtual pal_color_t       & getPixelColorFromPalette( unsigned int x, unsigned int y )        = 0;
        virtual const pal_color_t & getPixelColorFromPalette( unsigned int x, unsigned int y )const   = 0;
    };


//================================================================================================
//  ImgPixReader
//================================================================================================
    /*
        ImgPixReader
            Give this an iterator to a base_image, feed it bytes and it will write the 
            the proper ammount of pixels into the target image !

            NOTE: This provide a way to invert pixel endianess at bit level.
    */
    template<class _Img_T, class _outit>
        class ImgPixReader
    {
    //---------------------------------
    //       Constants + Typedefs
    //---------------------------------
    public:
        static const unsigned int BytesPerPixel = _Img_T::pixel_t::mypixeltrait_t::BYTES_PER_PIXEL;
        static const unsigned int BitsPerPixel  = _Img_T::pixel_t::mypixeltrait_t::BITS_PER_PIXEL; //If constexpr would work in vs, this would be less ugly !

        typedef std::array<uint8_t, BytesPerPixel>                     buffer_t;
        typedef typename _Img_T::pixel_t::mypixeltrait_t::pixeldata_t  pixeldata_t;
        typedef _Img_T                                                 img_t;
        typedef _outit                                                 outit_t;

    private:
    //---------------------------------
    //        Optimized Handlers
    //---------------------------------
        /*
            Those structs contain specialised handling code for specific types of pixels.
            They're instantiated depeneding on the pixel format of image we're handling,
            thanks to the magic of templates!
        */
        friend struct Handle4bpp;
        friend struct Handle8bppMultiBytes;
        friend struct Handle8bpp;
        friend struct GenericBitHandler;

        typedef ImgPixReader<_Img_T,_outit> parent_t;

        /***************************************************************************************
            Handle 4bpp pixels only
        ***************************************************************************************/
        struct Handle4bpp
        {
            Handle4bpp( parent_t * parentpixreader )
                :m_pPixEater(parentpixreader)
            {}

            static const bool ShouldUse = BitsPerPixel == 4 && BytesPerPixel == 1;

            inline void Parse( uint8_t abyte )
            {
                auto & itrout = (m_pPixEater->m_itOut); 

                if( m_pPixEater->m_bLittleEndian )
                {
                    (*itrout) = abyte & 0xf;
                    ++itrout;
                    (*itrout) = abyte >> 4;
                    ++itrout;
                }
                else
                {
                    (*itrout) = abyte >> 4;
                    ++itrout;
                    (*itrout) = abyte & 0xf;
                    ++itrout;
                }
            }
            parent_t * m_pPixEater;
        };

        /***************************************************************************************
            Handle multi-bytes pixels where each components are 8 bits only
        ***************************************************************************************/
        struct Handle8bppMultiBytes
        {
            typedef std::array<uint8_t, BytesPerPixel> buffer_t;

            Handle8bppMultiBytes( parent_t * pixreader )
                :m_pPixEater(pixreader)
            {
                m_itBuff = m_buffer.begin();
            }

            static const bool ShouldUse = BitsPerPixel == 8 && BytesPerPixel > 1; //#FIXME: Um, this makes no sense, no ? BitsPerPixel should be > 8, right ?

            inline void Parse( uint8_t abyte )
            {
                (*m_itBuff) = abyte;
                ++m_itBuff;

                //Check if after inserting we've filled the buffer
                if( m_itBuff == m_buffer.end() )
                {
                    (*(m_pPixEater->m_itOut)) = utils::ReadIntFromBytes<pixeldata_t>( m_buffer.begin(), (m_pPixEater->m_bLittleEndian) );
                    ++(m_pPixEater->m_itOut);
                    m_itBuff = m_buffer.begin(); //reset buffer write pos
                }
            }

            buffer_t                      m_buffer;
            typename buffer_t::iterator   m_itBuff;
            parent_t                    * m_pPixEater;
        };

        /***************************************************************************************
            Handle 8bpp pixels only
        ***************************************************************************************/
        struct Handle8bpp
        {
            Handle8bpp( parent_t * pixreader )
                :m_pPixEater(pixreader)
            {}

            static const bool ShouldUse = BitsPerPixel == 8 && BytesPerPixel == 1;

            inline void Parse( uint8_t abyte )
            {
                (*(m_pPixEater->m_itOut)) = abyte;
                ++(m_pPixEater->m_itOut);
            }
            parent_t * m_pPixEater;
        };

        /***************************************************************************************
            Handle any pixel formats bit per bit. 
            This is the slowest method, but the surefire one!
        ***************************************************************************************/
        struct GenericBitHandler
        {
            static const bool ShouldUse = BitsPerPixel != 8 && BitsPerPixel != 4;

            GenericBitHandler( parent_t * pixreader )
                :m_bitsbuff(pixreader->m_itOut),m_pPixEater(pixreader)
            {}

            ~GenericBitHandler()
            {
                //Warn when stopping parsing before all bits were removed
                if( m_bitsbuff.curbit != 0 )
                {
                    std::cerr << "<!>- Warning: ImgPixReader::GenericBitHandler : Stopping with a pixel still being parsed !\n";
                }
            }

            inline void Parse( uint8_t abyte )
            {
                if( m_pPixEater->m_bLittleEndian )
                {
                    assert( false ); //Can't make this guarranty yet ! #TODO: Gotta make sure if its safe to ignore pixel endian on single bits !
                }

                //Just feed the bits to the pixel eater and empty it only when its full !
                for( int8_t bit = 7; bit >= 0; --bit, m_bitsbuff( ( (bit >> abyte) & 0x1) ) );
            }

            /***************************************************************************************
                A bitbuffer to accumulate bits for a pixel
            ***************************************************************************************/
            struct BitEater
            {
                BitEater( outit_t & theitout )
                    :itout(theitout)
                { 
                    reset();
                }

                inline void reset()
                {
                    curbit=0;
                    buffer=0;
                }

                //Return true, when full.
                // Fills the pixel bits from left to right. highest to lowest!
                // The value of the bit passed must be in the lowest bit of the byte !
                inline bool operator()( uint8_t abit )
                {
                    buffer |= ( ((BitsPerPixel-1) - curbit) << (abit & 0x1) );
                    ++curbit;

                    if( curbit == BitsPerPixel )
                    {
                        (*itout) = buffer;
                        reset();
                        return true;
                    }
                    return false;
                }

                outit_t                              & itout;
                unsigned int                           curbit;
                typename img_t::pixel_t::pixeldata_t  buffer;
            };

            BitEater   m_bitsbuff;
            parent_t * m_pPixEater;
        };

    
    //---------------------------------
    //   Optimized handler selector
    //---------------------------------
    public:
        typedef typename std::conditional<Handle4bpp::ShouldUse, Handle4bpp,
                         typename std::conditional<Handle8bpp::ShouldUse, Handle8bpp, 
                                  typename std::conditional<Handle8bppMultiBytes::ShouldUse, Handle8bppMultiBytes, 
                                           GenericBitHandler >::type >::type >::type
                handlerstruct_t;    //The struct containing the optimized handling code for the pixel we're dealing with !

    //---------------------------------
    //     Constructor + Operator
    //---------------------------------
        explicit ImgPixReader( outit_t itoutbeg, bool bLittleEndian = true )
            :m_itOut(itoutbeg), m_bLittleEndian(bLittleEndian), m_pixelHandler(this)
        {}

        explicit ImgPixReader( img_t & destimg, bool bLittleEndian = true )
            :m_bLittleEndian(bLittleEndian), m_pixelHandler(this), m_itOut(&destimg)
        {
            //m_itOut = destimg.begin();
        }

        ImgPixReader & operator=( uint8_t abyte )
        {
            m_pixelHandler.Parse( abyte );
            return (*this);
        }

    private:

        handlerstruct_t m_pixelHandler;
        outit_t         m_itOut;
        bool            m_bLittleEndian;
    };

//================================================================================================
//  ImgPixWriter
//================================================================================================

    template<class _Pixel_T, class _ByContainer_T>
        class ImgPixWriter
    {
    public:
    //---------------------------------
    //       Constants + Typedefs
    //---------------------------------
    
        static const unsigned int BytesPerPixel = _Pixel_T::mypixeltrait_t::BYTES_PER_PIXEL;
        static const unsigned int BitsPerPixel  = _Pixel_T::mypixeltrait_t::BITS_PER_PIXEL; //If constexpr would work in vs, this would be less ugly !

        
        typedef _Pixel_T                                      pixel_t;
        typedef typename pixel_t::mypixeltrait_t::pixeldata_t pixeldata_t;
        typedef _ByContainer_T                                container_t;
        typedef ImgPixWriter<_Pixel_T,_ByContainer_T>         my_t;
        

    public:
        ImgPixWriter( container_t & container, bool blittleendian = true )
            :m_container(container),m_bLittleEndian(blittleendian), m_pixelHandler( (*const_cast<my_t*>(this)))
        {
        }

        ~ImgPixWriter()
        {
            m_pixelHandler.Flush();
        }


        //This output iterator on a byte container takes pixels, and push back the bytes resulting from
        // converting to raw data the pixels.

        //It has to handle pixel endianess, and as many pixel formats as possible.

        ImgPixWriter & operator=( const pixel_t & pix )
        {
            m_pixelHandler.Convert( pix );
            return (*this);
        }

    private:


    private:

        friend struct HndlrMBy;
        friend struct Hndlr8bpp;
        friend struct Hndlr4bpp;
        friend struct HndlrBit;

        /*
            Optimized handler for pixels with a number of bits per pixels higher than 8, and divisible by 8.

            #TODO: Gotta investigate endianess issues a little more for this one..
        */
        struct HndlrMBy
        {
            //typedef std::array<uint8_t, BytesPerPixel> buffer_t;

            static const bool IsAdequate = (BitsPerPixel > 8) && (BitsPerPixel % 8 == 0) && (BytesPerPixel > 1);

            HndlrMBy( my_t & parent )
                :m_pixwriter(parent)
            {}

            void Convert( const pixel_t & pix )
            {
                for( unsigned int i = 0; i < BytesPerPixel; ++i )
                    parent.m_container.push_back( static_cast<uint8_t>( pix.getWholePixelData() >> (i * 8) ) );
            }

            /*
                Call this to push back the current buffer as-is, only if its not empty.
            */
            void Flush()
            {
            }

            my_t & m_pixwriter;
        };

        /*
            Optimized handler for 8bpp pixels.
        */
        struct Hndlr8bpp
        {
            static const bool IsAdequate = (BitsPerPixel == 8) && (BytesPerPixel == 1);

            Hndlr8bpp( my_t & parent )
                :m_pixwriter(parent)
            {}

            void Convert( const pixel_t & pix )
            {
                m_pixwriter.m_container.push_back(pix.getWholePixelData());
            }

            /*
                Call this to push back the current buffer as-is, only if its not empty.
            */
            void Flush()
            {
            }

            my_t & m_pixwriter;
        };

        /*
            Optimized handler for 4bpp pixels.
        */
        struct Hndlr4bpp
        {
            static const bool IsAdequate = (BitsPerPixel == 4) && (BytesPerPixel == 1);

            Hndlr4bpp( my_t & parent )
                :m_pixwriter(parent), m_writeSlot2(false), m_buf(0)
            {}

            void Convert( const pixel_t & pix )
            {
                if( !m_writeSlot2 )
                {
                    if( m_pixwriter.m_bLittleEndian )
                        m_buf |= (pix.getWholePixelData() & 0xf);
                    else
                        m_buf |= ((pix.getWholePixelData() & 0xf) << 4);
                    m_writeSlot2 = true;
                }
                else
                {
                    if( m_pixwriter.m_bLittleEndian )
                        m_buf |= ((pix.getWholePixelData() & 0xf) << 4 );
                    else
                        m_buf |= (pix.getWholePixelData() & 0xf);

                    m_pixwriter.m_container.push_back(m_buf);
                    m_writeSlot2 = false;
                    m_buf = 0;
                }
            }

            /*
                Call this to push back the current buffer as-is, only if its not empty.
            */
            void Flush()
            {
                if( m_writeSlot2 )
                {
                    m_pixwriter.m_container.push_back(m_buf);
                    m_writeSlot2 = false;
                    m_buf = 0;
                }
            }

            bool      m_writeSlot2; //If False, fill first slot, if true, fill second slot!
            uint8_t   m_buf;
            my_t    & m_pixwriter;
        };

        /*
            Generic bit per bit handler for pixel data.
        */
        struct HndlrBit
        {
            //Use this in last resort!
            static const bool IsAdequate = !Hndlr4bpp::IsAdequate && !Hndlr8bpp::IsAdequate && !HndlrMBy::IsAdequate;

            HndlrBit( my_t & parent )
                :m_pixwriter(parent), m_curbit(0), m_buf(0)
            {}

            void Convert( const pixel_t & pix )
            {
                if( m_pixwriter.m_bLittleEndian )
                    ConvertLittleEndian(pix);
                else
                    ConvertBigEndian(pix);
            }

            void ConvertBigEndian( const pixel_t & pix )
            {
                unsigned int curbufbit = 7;

                for( unsigned int curbit = 0; curbit < BitsPerPixel; ++curbit )
                {
                    m_buf |=  (pix.getASingleBit(curbit) << m_curbit);

                    //Set the next bit to fill
                    if( m_curbit > 0 )
                        --m_curbit;
                    else
                    {
                        m_pixwriter.m_container.push_back(m_buf);
                        m_curbit = 7;
                        m_buf    = 0;
                    }
                }
            }

            void ConvertLittleEndian( const pixel_t & pix )
            {
                for( unsigned int curbit = 0; curbit < BitsPerPixel; ++curbit )
                {
                    m_buf |=  (pix.getASingleBit(curbit) << m_curbit);

                    //Set the next bit to fill
                    if( m_curbit < 7 )
                        ++m_curbit;
                    else
                    {
                        m_pixwriter.m_container.push_back(m_buf);
                        m_curbit = 0;
                        m_buf    = 0;
                    }
                }
            }

            /*
                Call this to push back the current buffer as-is, only if its not empty.
            */
            void Flush()
            {
                if( m_pixwriter.m_bLittleEndian && m_curbit > 0 )
                {
                    m_pixwriter.m_container.push_back(m_buf);
                    m_curbit = 0;
                    m_buf    = 0;
                }
                else if( !m_pixwriter.m_bLittleEndian && m_curbit < 7 )
                {
                    m_pixwriter.m_container.push_back(m_buf);
                    m_curbit = 7;
                    m_buf    = 0;
                }
            }

            uint8_t   m_curbit;
            uint8_t   m_buf;
            my_t    & m_pixwriter;
        };

        /*
            Handler selector
        */
        typedef typename std::conditional< HndlrMBy::IsAdequate, HndlrMBy,
                                  typename std::conditional< Hndlr8bpp::IsAdequate, Hndlr8bpp,
                                                    typename std::conditional< Hndlr4bpp::IsAdequate, Hndlr4bpp, HndlrBit>::type 
                                                  >::type 
                                >::type
                pixhndlr_t;

    private:
        container_t & m_container;
        bool          m_bLittleEndian;
        pixhndlr_t    m_pixelHandler;
    };

//================================================================================================
//  PxlReadIter
//================================================================================================
    /*
        PxlReadIter
            Pass a base_image as parameter at construction, and feed the PxlReadIter bytes. 
            It will assemble pixels from those automatically and push them back into the 
            image passed as parameter!

            NOTE: This does not provide any way of inverting pixel "endianness" or actual endianness !
    */
    template<class _ContainerType>
        class PxlReadIter : public std::_Outit
    {
    public:
        
        typedef  PxlReadIter<_ContainerType>                                        mytype_t;
        typedef _ContainerType                                                      container_type;
        typedef _ContainerType                                                      container_t;
        typedef typename  container_t *                                             container_ptr_t;
        typedef ImgPixReader<typename container_t, typename container_t::iterator>  mypixreader_t;
        typedef typename _ContainerType::value_type                                 valty_t;

        //Default constructor
        explicit PxlReadIter( container_t & img )
            :m_pContainer( std::addressof(img) ), m_pixreader( img )
        {}

        //Copy constructor
        PxlReadIter( const mytype_t & other )
            :m_pContainer( other.m_pContainer ), m_pixreader( other.m_pixreader )
        {}

        //Copy operator
        mytype_t & operator=( const mytype_t & other )
        {
            this->m_pContainer = other.m_pContainer;
            this->m_pixreader  = other.m_pixreader;
            return *this;
        }

        /*
            Operator =
        */
        mytype_t & operator=( uint8_t val )
        {
            m_pixreader = val;
            return (*this);
        }

        mytype_t & operator=( uint8_t && val )
        {
            m_pixreader = val;
            return (*this);
        }

        /*
            Operator ++(prefix)
        */
        mytype_t& operator++()  
        { 
            //nothing
            return (*this);
        }

        /*
            Operator ++(postfix)
        */
        mytype_t operator++(int)
        {
            //nothing;
            return (*this);
        }

        /*
            Operator *
        */
        mytype_t&       operator*()        { return (*this); }
        const mytype_t& operator*() const  { return (*this); }

    protected:
        container_ptr_t                m_pContainer;
        mypixreader_t                  m_pixreader;
    };


//================================================================================================
//  PxlWriteIter
//================================================================================================
    /*
        PxlWriteIter
            This output iterator takes an image pixel and convert the data to bytes, places it inside
            the container the iterator was passed as parameter at construction.

            This iterator calls only the method push_back on the container.
    */

    template< class _PIXEL_T, class _ByteContainerType >
        class PxlWriteIter : public std::_Outit
    {
    public:
        
        typedef _PIXEL_T                                                            pixel_t;
        typedef  PxlWriteIter<_PIXEL_T, _ByteContainerType>                         mytype_t;
        typedef _ByteContainerType                                                  container_type;
        typedef _ByteContainerType                                                  container_t;
        typedef typename  container_t *                                             container_ptr_t;
        typedef ImgPixWriter<_PIXEL_T, _ByteContainerType>   mypixwriter_t;
        typedef typename _ByteContainerType::value_type                             valty_t;

        //Default constructor
        explicit PxlWriteIter( container_t & bycnt )
            :m_pContainer( std::addressof(bycnt) ), m_pixwriter( bycnt )
        {}

        //Copy constructor
        PxlWriteIter( const mytype_t & other )
            :m_pContainer( other.m_pContainer ), m_pixwriter( other.m_pixwriter )
        {}

        //Copy operator
        mytype_t & operator=( const mytype_t & other )
        {
            this->m_pContainer = other.m_pContainer;
            this->m_pixwriter  = other.m_pixwriter;
            return *this;
        }

        /*
            Operator =
        */
        mytype_t & operator=( pixel_t val )
        {
            m_pixwriter = val;
            return (*this);
        }

        mytype_t & operator=( pixel_t && val )
        {
            m_pixwriter = val;
            return (*this);
        }

        /*
            Operator ++(prefix)
        */
        mytype_t& operator++()  
        { 
            //nothing
            return (*this);
        }

        /*
            Operator ++(postfix)
        */
        mytype_t operator++(int)
        {
            //nothing;
            return (*this);
        }

        /*
            Operator *
        */
        mytype_t&       operator*()        { return (*this); }
        const mytype_t& operator*() const  { return (*this); }

    protected:
        container_ptr_t                m_pContainer;
        mypixwriter_t                  m_pixwriter;
    };
};

#endif