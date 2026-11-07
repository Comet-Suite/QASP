//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
// QaspTheme.cpp
//
// QASP addition (not present in upstream OpenVSP).  See QaspTheme.h.
//

#include "QaspTheme.h"

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/fl_draw.H>

bool QaspTheme::s_Applied = false;

//----------------------------------------------------------------------------
// Palette
//----------------------------------------------------------------------------
// Kept low-contrast and slightly cool.  Pure white is reserved for inputs so
// they read as "type here", and the window background is a soft neutral so
// panels separate themselves without heavy borders.

static Fl_Color QaspBorder()      { return fl_rgb_color( 205, 212, 222 ); }  // hairline edge
static Fl_Color QaspBorderDark()  { return fl_rgb_color( 160, 170, 185 ); }  // pressed edge

//----------------------------------------------------------------------------
// Box drawing
//----------------------------------------------------------------------------
// FLTK calls these with (x, y, w, h, colour) and expects the function to paint
// the whole box.  Everything here is a flat fill plus an optional border, which
// is what removes the 1990s bevel.

// Raised / normal control: flat fill, hairline border.
static void qasp_up_box( int x, int y, int w, int h, Fl_Color c )
{
    fl_color( c );
    fl_rectf( x, y, w, h );
    fl_color( QaspBorder() );
    fl_rect( x, y, w, h );
}

// Pressed / selected control: slightly darker fill so the click still reads.
static void qasp_down_box( int x, int y, int w, int h, Fl_Color c )
{
    fl_color( fl_color_average( c, FL_BLACK, 0.88f ) );
    fl_rectf( x, y, w, h );
    fl_color( QaspBorderDark() );
    fl_rect( x, y, w, h );
}

// Border only - used for engraved/embossed frames around groups and boxes.
static void qasp_frame( int x, int y, int w, int h, Fl_Color c )
{
    fl_color( c );
    fl_rect( x, y, w, h );
}

// Thin variant used by some containers.
static void qasp_thin_up_box( int x, int y, int w, int h, Fl_Color c )
{
    qasp_up_box( x, y, w, h, c );
}

//----------------------------------------------------------------------------
// Apply
//----------------------------------------------------------------------------

void QaspTheme::Apply()
{
    if ( s_Applied )
    {
        return;
    }

    //---- 1. Base scheme ---------------------------------------------------
    // Must be set before the box overrides below, because the scheme installs
    // its own box types as a side effect.
    Fl::scheme( "gtk+" );

    //---- 2. Palette -------------------------------------------------------
    Fl::background(  244, 246, 249 );   // window / panel background
    Fl::background2( 255, 255, 255 );   // input fields and lists
    Fl::foreground(   30,  38,  48 );   // body text

    Fl::set_color( FL_SELECTION_COLOR, 0, 122, 204 );  // accent blue selection
    Fl::set_color( FL_INACTIVE_COLOR, 132, 142, 155 ); // disabled text

    //---- 3. Box types -----------------------------------------------------
    // Replacing the bevel drawer is what actually modernises the look.  The
    // final four arguments are the box's border metrics (dx, dy, dw, dh),
    // which must stay small so widgets keep their existing geometry.
    Fl::set_boxtype( FL_UP_BOX,          qasp_up_box,   1, 1, 2, 2 );
    Fl::set_boxtype( FL_DOWN_BOX,        qasp_down_box, 1, 1, 2, 2 );
    Fl::set_boxtype( FL_THIN_UP_BOX,     qasp_thin_up_box, 1, 1, 2, 2 );
    Fl::set_boxtype( FL_THIN_DOWN_BOX,   qasp_down_box, 1, 1, 2, 2 );

    Fl::set_boxtype( FL_UP_FRAME,        qasp_frame,    1, 1, 2, 2 );
    Fl::set_boxtype( FL_DOWN_FRAME,      qasp_frame,    1, 1, 2, 2 );
    Fl::set_boxtype( FL_THIN_UP_FRAME,   qasp_frame,    1, 1, 2, 2 );
    Fl::set_boxtype( FL_THIN_DOWN_FRAME, qasp_frame,    1, 1, 2, 2 );

    Fl::set_boxtype( FL_ENGRAVED_BOX,    qasp_frame,    1, 1, 2, 2 );
    Fl::set_boxtype( FL_EMBOSSED_BOX,    qasp_up_box,   1, 1, 2, 2 );
    Fl::set_boxtype( FL_ENGRAVED_FRAME,  qasp_frame,    1, 1, 2, 2 );
    Fl::set_boxtype( FL_EMBOSSED_FRAME,  qasp_frame,    1, 1, 2, 2 );

    // Round and shadowed boxes appear on dialogs and menus.
    Fl::set_boxtype( FL_ROUND_UP_BOX,    qasp_up_box,   1, 1, 2, 2 );
    Fl::set_boxtype( FL_ROUND_DOWN_BOX,  qasp_down_box, 1, 1, 2, 2 );
    Fl::set_boxtype( FL_SHADOW_BOX,      qasp_up_box,   1, 1, 2, 2 );

    s_Applied = true;
}
