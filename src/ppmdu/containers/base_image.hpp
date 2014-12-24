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
#include <vector>
#include <algorithm>

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
        virtual unsigned int getNbRows()const        = 0;
        virtual unsigned int getNbCol()const         = 0;
        virtual unsigned int getNbPixelWidth()const  = 0;
        virtual unsigned int getNbPixelHeight()const = 0;
        virtual unsigned int getTotalNbPixels()const = 0;
        virtual unsigned int size()const             = 0; //Size, in PIXELS. For iterator and etc..

        //This returns the exact amount of bits that each pixels in the image uses
        virtual unsigned int getSizeInBits()const    = 0;

        //Access to the iterators
        //virtual iterator       begin() throw()       = 0;
        //virtual const_iterator begin() const throw() = 0;
        //virtual iterator       end()   throw()       = 0;
        //virtual const_iterator end()   const throw() = 0;
    };

};

#endif