#ifndef LINEAR_IMAGE_HPP
#define LINEAR_IMAGE_HPP
/*
linear_image.hpp
2014/12/07
psycommando@gmail.com
Description: A linear image where pixels are loaded linearily and then accessed via 2d 
             coordinates! Like the tiled_image, but without tiles!
*/
#include <ppmdu/containers/base_image.hpp>
#include <ppmdu/utils/utility.hpp>

namespace gimg
{

//
//
//
    /*
        linear_image
            A class for a regular 2d non tiled image.
    */
    template<class _PIXEL_T>
        class linear_image : public base_image< linear_image<_PIXEL_T>, _PIXEL_T>
    {
    public:
        typedef _PIXEL_T                                        pixel_t;
        typedef linear_image<_PIXEL_T>                          _myty;
        typedef pixel_t                                         value_type; //For the iterator
        typedef utils::index_iterator<linear_image>             iterator;
        typedef utils::const_index_iterator<const linear_image> const_iterator;

        linear_image()throw()
            :m_img( 0, std::vector<pixel_t>(0) )
        {
        }

        linear_image( unsigned int xres, unsigned int yres )
            //:m_img( yres, std::vector<pixel_t>(xres) )
        {
            setPixelResolution(xres, yres);
        }

        //Copy
        inline linear_image( const _myty & other )              { copyFrom(&other); }
        //Move
        inline linear_image( _myty && other )                   { moveFrom(&other); }
        //Copy
        virtual inline const _myty & operator=( const _myty & other )   
        {
            copyFrom(&other);
            return *this;
        }
        //Move
        inline const _myty & operator=( _myty && other )
        {
            moveFrom(&other);
            return *this;
        }

        //Copy
        inline void copyFrom( const _myty * const other )
        {
            //Move assignement operator called
            m_img = std::vector< std::vector<pixel_t> >( other->m_img.begin(), other->m_img.end() );
        }

        //Move
        inline void moveFrom( _myty * other )
        {
            //Move
            m_img = std::move(other->m_img);
        }

        //------ Methods ------
        //Access the image data like a linear 1D array
        inline pixel_t & operator[]( unsigned int pos )
        {
            size_t xpos = pos % m_img.front().size();
            size_t ypos = pos / m_img.front().size();
            return m_img[ypos][xpos];
        }

        //Access the image data like a linear 1D array
        inline const pixel_t & operator[]( unsigned int pos )const
        {
            return (*const_cast<linear_image<_PIXEL_T> *>(this))[pos];
        }

        //Access the image data like a 2D bitmap
        inline pixel_t & getPixel( unsigned int x, unsigned int y ) 
        { 
            return m_img[y][x]; 
        }

        //Access the image data like a 2D bitmap
        inline const pixel_t & getPixel( unsigned int x, unsigned int y )const
        {
            return (const_cast<linear_image<_PIXEL_T> *>(this))->getPixel(x,y);
        }

        //Set the image resolution in pixels.
        inline void setPixelResolution( unsigned int pixelsWidth, unsigned int pixelsHeigth )
        {
            resize( pixelsWidth, pixelsHeigth );
        }

        virtual void resize( unsigned int width, unsigned int height )
        {
            if( height == 0 && width == 0 )
            {
                m_img.resize(height);
            }
            else if( height == 0 || width == 0 )
            {
                throw std::runtime_error( "linear_image::resize(): Attempted to resize image with a null dimension and a non-null dimension. If a dimension is set to 0, the other must be 0 as well!" );
            }
            else
                m_img.resize( height, std::vector<pixel_t>(width) );
        }

        //Get sizes and stuff
        inline unsigned int getNbRows()const
        {
            return getNbPixelHeight();
        }

        inline unsigned int getNbCol()const
        {
            return getNbPixelWidth();
        }
        inline unsigned int getNbPixelWidth()const
        {
            if( m_img.size() == 0 )
                return 0;
            else
                return m_img.front().size();
        }

        inline unsigned int getNbPixelHeight()const
        {
            return m_img.size();
        }

        inline unsigned int width()const
        { 
            if( m_img.size() == 0 )
                return 0;
            else
                return m_img.front().size();
        }
        
        inline unsigned int height()const           
        { 
            return m_img.size();
        }

        inline unsigned int getTotalNbPixels()const
        {
            return getNbPixelWidth() * getNbPixelHeight(); 
        }

        inline bool empty()const
        {
            return m_img.empty(); // This works, because we forbid images with a width set to 0!
        }

        //Size, in PIXELS. For iterator and etc..
        inline unsigned int size()const
        {
            return getTotalNbPixels();
        }

        //This returns the exact amount of bits that each pixels in the image uses
        inline unsigned int getSizeInBits()const
        {
            return getTotalNbPixels() * pixel_t::GetBitsPerPixel();
        }

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


        //Access to the iterators
        inline iterator       begin() throw()       { return iterator(this,0); }
        inline const_iterator begin() const throw() { return const_iterator(this,0); }
        inline iterator       end()   throw()       { return iterator( const_cast<linear_image<_PIXEL_T>* >(this), size() ); }
        inline const_iterator end()   const throw() { return const_iterator( this, size() ); }

    protected:
        std::vector< std::vector<pixel_t> > m_img;
    };


//
//
//
    /*
        linear_indexed_image
            A class for a 2d non tiled image with a color palette.
    */
    template<class _PIXEL_T, class _Color_T>
        class linear_indexed_image : public linear_image<_PIXEL_T>, public base_indexed_image<_Color_T,_PIXEL_T>
    {
    public:

        typedef linear_image<_PIXEL_T>                                   _parentty;
        typedef _PIXEL_T                                                pixel_t;
        typedef linear_indexed_image<_PIXEL_T,_Color_T>                 _myty;
        typedef pixel_t                                                 value_type; //For the iterator
        typedef utils::index_iterator<linear_indexed_image>             iterator;
        typedef utils::const_index_iterator<const linear_indexed_image> const_iterator;

        //------ Constructors ------
        linear_indexed_image()
            :_parentty(), m_palette(pixel_t::mypixeltrait_t::MAX_VALUE_PER_COMPONEMENT)
        {}

        linear_indexed_image( unsigned int xres, unsigned int yres )
            :_parentty(xres,yres), m_palette(pixel_t::mypixeltrait_t::MAX_VALUE_PER_COMPONEMENT)
        {}

        linear_indexed_image( unsigned int xres, unsigned int yres, unsigned int nbcolors )
            :_parentty(xres,yres), m_palette(nbcolors)
        {}


        //Copy
        inline linear_indexed_image( const _myty & other )              { copyFrom(&other); }
        //Move
        inline linear_indexed_image( _myty && other )                   { moveFrom(&other); }
        //Copy
        inline const _myty & operator=( const _myty & other )   
        {
            copyFrom(&other);
            return *this;
        }
        //Move
        inline const _myty & operator=( _myty && other )
        {
            moveFrom(&other);
            return *this;
        }

        inline void copyFrom(const _myty * const other) 
        {
            m_palette = std::vector<pal_color_t>(other->m_palette.begin(), other->m_palette.end());
            //Call copyFrom from parent
            _parentty::copyFrom( dynamic_cast<const _myty * const>(other) );
        }

        inline void moveFrom(_myty * other)
        {
            m_palette = std::move_if_noexcept(other->m_palette);
            //Call moveFrom from parent
            _parentty::moveFrom( dynamic_cast<_myty*>(other) );
        }

       //------ Methods ------
        //Accessors
        inline std::vector<pal_color_t>       & getPalette()                                         { return m_palette; }
        inline const std::vector<pal_color_t> & getPalette()const                                    { return m_palette; }
        inline const pal_color_t              & getColor( typename pixel_t::pixeldata_t index )const { return m_palette[index]; }
        inline pal_color_t                    & getColor( typename pixel_t::pixeldata_t index )      { return m_palette[index]; }
        inline unsigned int                     getNbColors()const                                   { return m_palette.size(); }
        inline void                             setNbColors(unsigned int nbcol)                      { return m_palette.resize(nbcol); }
        inline void setColor( typename pixel_t::pixeldata_t index, pal_color_t && color )            { m_palette[index] = color; }
        inline void setColor( typename pixel_t::pixeldata_t index, pal_color_t color )               { m_palette[index] = std::move(color); }



        //Get the color of a pixel at (X, Y) directly
        inline pal_color_t & getPixelColorFromPalette( unsigned int x, unsigned int y )
        {
            return getColor( getPixel(x,y) );
        }

        inline const pal_color_t & getPixelColorFromPalette( unsigned int x, unsigned int y )const
        {
            return const_cast<_myty *>(this)->getPixelColorFromPalette(x,y);
        }

        inline virtual colorRGB24 getPixelRGBColor( unsigned int x, unsigned int y )const
        {
            return getPixel(x,y).ConvertToRGBColor();
        }

        //Implementation for Non-indexed images
        inline virtual colorRGB24 getPixelRGBColor( unsigned int linearpixelindex )const
        {
            return (*this)[linearpixelindex].ConvertToRGBColor();
        }

    protected:
        std::vector<pal_color_t> m_palette;
    };


//=============================================================================
//Common linear_image Typedefs
//=============================================================================
    typedef linear_image        <trait_indexed_1bpp>             image_1bpp;  //1bpp image.
    typedef linear_indexed_image<pixel_indexed_4bpp, colorRGB24> image_i4bpp; //Indexed 4bpp image with rgb24 palette.
    typedef linear_indexed_image<pixel_indexed_8bpp, colorRGB24> image_i8bpp; //Indexed 8bpp image with rgb24 palette.


//==============================================================================
//  Functions
//==============================================================================

    /*************************************************************************************************
        ParseTiledImg
            This function parses linear images. It only handles the pixels, not the color palettes.
            Pass it iterators to a container containing raw bytes.

            - invertpixelorder : If true, reverse the pixel order on pixels smaller than 
                                 a single byte!

                                 If false, read higher pixels bits first, then the lower ones. If true,
                                 reads lower pixels bits first, then higher ones.
                                 Basically, false is big endian, and true is little endian!

                                 For example, a 4bpp image would have its low nybble pixel read 
                                 first, if invertpixelorder was set to true !

    *************************************************************************************************/
    //template<class _LINEAR_IMG_T, class _init>
    //    void ParseLinearImg( _init                itBegByte, 
    //                         _init                itEndByte,
    //                         utils::Resolution    imgrespixels,
    //                         _LINEAR_IMG_T       &out_img,
    //                         bool                 invertpixelorder = false )
    //{

    //    typedef _LINEAR_IMG_T                 image_t;
    //    typedef typename image_t::pixel_t     pixel_t;
    //    typedef typename pixel_t::pixeldata_t pixeldata_t;

    //    const unsigned int NbBitsPerPixel      = pixel_t::GetBitsPerPixel();
    //    const unsigned int NbOverlappingBits   = (pixel_t::GetBytesPerPixel() * 8) % NbBitsPerPixel; 
    //    const unsigned int DataSz              = std::distance( itBegByte, itEndByte ); //How much bytes we got to handle
    //    const unsigned int ExpectedTotalNbBits = (imgrespixels.width * imgrespixels.height) * NbBitsPerPixel;

    //    if( NbOverlappingBits != 0 )
    //        throw std::runtime_error( "ParseLinearImg(): Pixel data overlapps " );


    //    //Compare size

    //}


};

#endif