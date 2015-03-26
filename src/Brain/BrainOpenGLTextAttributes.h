#ifndef __BRAIN_OPEN_G_L_TEXT_ATTRIBUTES_H__
#define __BRAIN_OPEN_G_L_TEXT_ATTRIBUTES_H__

/*LICENSE_START*/
/*
 *  Copyright (C) 2015 Washington University School of Medicine
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*LICENSE_END*/

#include <stdint.h>
#include "CaretObject.h"



namespace caret {

    class BrainOpenGLTextAttributes : public CaretObject {
        
    public:
        /**
         * Name of font.
         */
        enum FontName {
            ARIAL
        };
        
        /**
         * Horizontal alignment of the text
         */
        enum HorizontalAlignment {
            /** Coordinate is at center of text string */
            X_CENTER,
            /** Coordinate is at left edge of first character */
            X_LEFT,
            /** Coordinate is at right edge of last character */
            X_RIGHT
        };
        
        /**
         * Vertical Alignment of the text
         */
        enum VerticalAlignment {
            /** Coordinate is at bottom edge of text */
            Y_BOTTOM,
            /** Coordinate is at center of text */
            Y_CENTER,
            /** Coordinate is a top of text */
            Y_TOP
        };
        
        /**
         * Orientation of text
         */
        enum Orientation {
            /** Orient left-to-right */
            LEFT_TO_RIGHT,
            /** Orient top-to-bottom */
            TOP_TO_BOTTOM
        };
        
        BrainOpenGLTextAttributes();
        
        virtual ~BrainOpenGLTextAttributes();
        
        BrainOpenGLTextAttributes(const BrainOpenGLTextAttributes& obj);
        
        BrainOpenGLTextAttributes& operator=(const BrainOpenGLTextAttributes& obj);
        
        AString getFontRenderingEncodedName() const;
        
        /**
         * @return The text font name.
         */
        inline FontName getFontName() const { return m_fontName; }
        
        /**
         * Set the text font name.
         *
         * @param fontName
         *     New font name.
         */
        inline void setFontName(const FontName fontName) { m_fontName = fontName; }
        
        /**
         * @return The horizontal alignment.
         */
        inline HorizontalAlignment getHorizontalAlignment() const { return m_horizontalAlignment; }
        
        /**
         * Set bold enabled.
         *
         * @param enabled
         *     New status for bold enabled.
         */
        inline void setBoldEnabled(const bool enabled) { m_boldEnabled = enabled; }
        
        /**
         * @return
         *    Is italic enabled ?
         */
        inline bool isItalicEnabled() const { return m_italicEnabled; }
        
        /**
         * Set italic enabled.
         *
         * @param enabled
         *     New status for italic enabled.
         */
        inline void setItalicEnabled(const bool enabled) { m_italicEnabled = enabled; }
        
        /**
         * @return
         *    Is underline enabled ?
         */
        inline bool isUnderlineEnabled() const { return m_underlineEnabled; }
        
        /**
         * Set underline enabled.
         *
         * @param enabled
         *     New status for underline enabled.
         */
        inline void setUnderlineEnabled(const bool enabled) { m_underlineEnabled = enabled; }
        
        /**
         * Set the horizontal alignment
         *
         * @param horizontalAlignment
         *     New horizontal alignment.
         */
        inline void setHorizontalAlignment(const HorizontalAlignment horizontalAlignment) { m_horizontalAlignment = horizontalAlignment; }
        
        /**
         * @return The vertical alignment
         */
        inline VerticalAlignment getVerticalAlignment() const { return m_verticalAlignment; }
        
        /**
         * Set the vertical alignment.
         *
         * @param verticalAlignment
         *     New vertical alignment.
         */
        inline void setVerticalAlignment(const VerticalAlignment verticalAlignment) { m_verticalAlignment = verticalAlignment; }
        
        /**
         * @return The orientation.
         */
        inline Orientation getOrientation() const { return m_orientation; }
        
        /**
         * Set the orientation.
         *
         * @param orientation
         *     New orientation.
         */
        inline void setOrientation(const Orientation orientation) { m_orientation = orientation; }
        
        /**
         * @return The font height.
         */
        inline int32_t getFontHeight() const { return m_fontHeight; }
        
        /**
         * Set the font height.
         *
         * @param fontHeight
         *     New font height.
         */
        inline void setFontHeight(const int32_t fontHeight) { m_fontHeight = fontHeight; }

        void getForegroundColor(float rgbaOut[4]) const;
        
        /**
         * @return
         *    Foreground color RGBA components (red, green, blue, alpha) each of which ranges [0.0, 1.0].
         */
        inline const float* getForegroundColor() const { return m_colorForeground; }
        
        void setForegroundColor(const float rgba[4]);
        
        void setForegroundColor(const uint8_t rgba[4]);
        
        void getBackgroundColor(float rgbaOut[4]) const;
        
        /**
         * @return
         *    Background color RGBA components (red, green, blue, alpha) each of which ranges [0.0, 1.0].
         *    The background color is applied only when its alpha component is greater than zero.
         */
        inline const float* getBackgroundColor() const { return m_colorBackground; }
        
        void setBackgroundColor(const float rgba[4]);
        
        void setBackgroundColor(const uint8_t rgba[4]);
        
         // ADD_NEW_METHODS_HERE

        virtual AString toString() const;
        
        /**
         * @return 
         *    Is bold enabled ?
         */
        inline bool isBoldEnabled() const { return m_boldEnabled; }
        
    private:
        void copyHelperBrainOpenGLTextAttributes(const BrainOpenGLTextAttributes& obj);
        
        FontName m_fontName;
        
        bool m_boldEnabled;
        
        bool m_italicEnabled;
        
        bool m_underlineEnabled;
        
        HorizontalAlignment m_horizontalAlignment;
        
        VerticalAlignment m_verticalAlignment;
        
        Orientation m_orientation;
        
        int32_t m_fontHeight;
        
        float m_colorForeground[4];
        
        float m_colorBackground[4];
        
        // ADD_NEW_MEMBERS_HERE

    };
    
#ifdef __BRAIN_OPEN_G_L_TEXT_ATTRIBUTES_DECLARE__
    // <PLACE DECLARATIONS OF STATIC MEMBERS HERE>
#endif // __BRAIN_OPEN_G_L_TEXT_ATTRIBUTES_DECLARE__

} // namespace
#endif  //__BRAIN_OPEN_G_L_TEXT_ATTRIBUTES_H__