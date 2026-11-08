//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
// QASP addition: conceptual diagrams drawn by the actual FLTK help component.
// They deliberately show no calculated forces, aircraft performance or safety
// claims. Drawings are illustrative and not to scale.
//
#include "QaspLessonDiagram.h"

#include <FL/fl_draw.H>
#include <cmath>
#include <initializer_list>
#include <utility>

namespace
{
const Fl_Color Ink = fl_rgb_color( 35, 53, 76 );
const Fl_Color Blue = fl_rgb_color( 0, 112, 183 );
const Fl_Color Pale = fl_rgb_color( 210, 231, 247 );
const Fl_Color Muted = fl_rgb_color( 93, 112, 134 );
const Fl_Color Orange = fl_rgb_color( 177, 91, 25 );

// A 720 x 190 design space, converted to real widget coordinates. Fonts retain
// a legible screen size instead of becoming tiny when the window is narrowed.
struct Drawing
{
    Fl_Widget& widget;
    int X( double v ) const { return widget.x() + static_cast<int>( v * widget.w() / 720.0 ); }
    int Y( double v ) const { return widget.y() + static_cast<int>( v * widget.h() / 190.0 ); }
    void Line( double x1, double y1, double x2, double y2, Fl_Color c = Blue ) const
    {
        fl_color( c );
        fl_line_style( FL_SOLID, 2 );
        fl_line( X( x1 ), Y( y1 ), X( x2 ), Y( y2 ) );
        fl_line_style( 0 );
    }
    void Arrow( double x1, double y1, double x2, double y2, Fl_Color c = Blue, bool both = false ) const
    {
        Line( x1, y1, x2, y2, c );
        const double a = std::atan2( static_cast<double>( Y( y2 ) - Y( y1 ) ),
                                    static_cast<double>( X( x2 ) - X( x1 ) ) );
        fl_color( c );
        const auto head = [a]( int x, int y, double sign )
        {
            fl_polygon( x, y,
                x - static_cast<int>( sign * 9 * std::cos( a ) - 4 * std::sin( a ) ),
                y - static_cast<int>( sign * 9 * std::sin( a ) + 4 * std::cos( a ) ),
                x - static_cast<int>( sign * 9 * std::cos( a ) + 4 * std::sin( a ) ),
                y - static_cast<int>( sign * 9 * std::sin( a ) - 4 * std::cos( a ) ) );
        };
        head( X( x2 ), Y( y2 ), 1.0 );
        if ( both ) head( X( x1 ), Y( y1 ), -1.0 );
    }
    void Text( const char* text, double x, double y, double width = 200,
               Fl_Color c = Ink, int size = 12, bool bold = false ) const
    {
        fl_color( c );
        fl_font( bold ? FL_HELVETICA_BOLD : FL_HELVETICA, size );
        fl_draw( text, X( x ), Y( y ), X( x + width ) - X( x ), 22,
                 FL_ALIGN_LEFT | FL_ALIGN_INSIDE );
    }
    void Polygon( std::initializer_list<std::pair<double, double>> points, Fl_Color fill = Pale ) const
    {
        fl_color( fill );
        fl_begin_polygon();
        for ( const auto& p : points ) fl_vertex( X( p.first ), Y( p.second ) );
        fl_end_polygon();
        fl_color( Blue );
        fl_begin_loop();
        for ( const auto& p : points ) fl_vertex( X( p.first ), Y( p.second ) );
        fl_end_loop();
    }
    void Box( double x, double y, double w, double h, const char* text ) const
    {
        fl_color( FL_WHITE );
        fl_rectf( X( x ), Y( y ), X( x + w ) - X( x ), Y( y + h ) - Y( y ) );
        fl_color( fl_rgb_color( 190, 213, 231 ) );
        fl_rect( X( x ), Y( y ), X( x + w ) - X( x ), Y( y + h ) - Y( y ) );
        Text( text, x + 12, y + h / 2 - 10, w - 20, Ink, 12, true );
    }
    void SidePlane() const
    {
        Polygon( { { 235, 102 }, { 290, 102 }, { 325, 87 }, { 408, 87 },
                   { 454, 101 }, { 466, 112 }, { 246, 112 } } );
        Polygon( { { 267, 101 }, { 255, 69 }, { 274, 69 }, { 300, 102 } } );
        Line( 333, 104, 392, 104 );
    }
    void Note( const char* text ) const
    {
        fl_color( Muted );
        fl_font( FL_HELVETICA, 11 );
        fl_draw( text, X( 20 ), Y( 160 ), X( 700 ) - X( 20 ), Y( 188 ) - Y( 160 ),
                 FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_WRAP );
    }
};
}

QaspLessonDiagram::QaspLessonDiagram( int x, int y, int w, int h )
    : Fl_Widget( x, y, w, h ), m_Diagram( QaspHelp::Diagram::Forces ), m_Empty( false )
{
    tooltip( "Explanatory drawing only. Not to scale; not a result from your aircraft model." );
}

void QaspLessonDiagram::SetDiagram( QaspHelp::Diagram diagram )
{
    m_Diagram = diagram;
    m_Empty = false;
    redraw();
}

void QaspLessonDiagram::Clear()
{
    m_Empty = true;
    redraw();
}

void QaspLessonDiagram::draw()
{
    fl_push_clip( x(), y(), w(), h() );
    fl_color( fl_rgb_color( 242, 248, 253 ) );
    fl_rectf( x(), y(), w(), h() );
    Drawing d = { *this };
    if ( m_Empty )
    {
        d.Text( "Try a term such as thrust, wing area or centre of gravity.", 30, 76, 660, Muted );
        fl_pop_clip();
        return;
    }
    using QaspHelp::Diagram;
    switch ( m_Diagram )
    {
        case Diagram::Forces:
            d.SidePlane();
            d.Arrow( 350, 82, 350, 27 );
            d.Text( "Lift", 365, 20, 100, Blue, 13, true );
            d.Arrow( 350, 118, 350, 151, Orange );
            d.Text( "Weight", 366, 131, 110, Orange, 13, true );
            d.Arrow( 475, 103, 628, 103 );
            d.Text( "Thrust", 503, 69, 130, Blue, 13, true );
            d.Arrow( 226, 103, 76, 103, Orange );
            d.Text( "Drag", 91, 69, 120, Orange, 13, true );
            d.Note( "Straight, level flight at steady speed: opposing forces balance." );
            break;
        case Diagram::Thrust:
            d.SidePlane();
            d.Line( 454, 73, 454, 136 );
            d.Arrow( 475, 105, 643, 105 );
            d.Text( "THRUST", 497, 70, 175, Blue, 16, true );
            d.Arrow( 208, 124, 98, 124, Muted );
            d.Arrow( 208, 91, 98, 91, Muted );
            d.Text( "Air pushed back", 53, 47, 200 );
            d.Text( "Propeller", 403, 27, 130 );
            d.Note( "The propeller pushes air backward; the air pushes the propeller forward." );
            break;
        case Diagram::Angle:
            d.Polygon( { { 211, 68 }, { 269, 47 }, { 358, 61 }, { 529, 126 }, { 310, 101 } } );
            fl_line_style( FL_DASH, 1 );
            fl_color( Muted );
            fl_line( d.X( 211 ), d.Y( 68 ), d.X( 548 ), d.Y( 68 ) );
            fl_line( d.X( 211 ), d.Y( 68 ), d.X( 529 ), d.Y( 126 ) );
            fl_line_style( 0 );
            d.Arrow( 41, 130, 186, 130 );
            d.Text( "Oncoming air", 39, 98, 173 );
            d.Text( "Angle to the airflow", 411, 29, 270, Blue, 12, true );
            d.Text( "Chord line", 440, 124, 180, Muted );
            d.Note( "Angle of attack describes orientation to the air, not simply nose angle to the horizon." );
            break;
        case Diagram::Drag:
            d.SidePlane();
            d.Arrow( 221, 106, 68, 106, Orange );
            d.Text( "DRAG", 77, 69, 145, Orange, 16, true );
            d.Arrow( 498, 48, 625, 48, Muted );
            d.Text( "Aircraft motion", 482, 18, 210 );
            d.Text( "Skin friction + shape + lift-related effects", 226, 129, 470 );
            d.Note( "Drag opposes motion through the air. Different tools estimate different contributions." );
            break;
        case Diagram::Wing:
            d.Polygon( { { 91, 69 }, { 630, 69 }, { 630, 121 }, { 91, 121 } } );
            d.Polygon( { { 350, 34 }, { 369, 34 }, { 379, 149 }, { 340, 149 } }, FL_WHITE );
            d.Arrow( 91, 40, 630, 40, Blue, true );
            d.Text( "Span: tip to tip", 260, 8, 260, Blue, 13, true );
            d.Arrow( 657, 69, 657, 121, Orange, true );
            d.Text( "Chord", 563, 126, 130, Orange );
            d.Text( "Area = wing outline size", 116, 80, 220 );
            d.Note( "Top view of a simple rectangular wing. Aspect ratio = span squared / area." );
            break;
        case Diagram::Taper:
            d.Polygon( { { 191, 48 }, { 572, 81 }, { 572, 120 }, { 191, 139 } } );
            d.Arrow( 157, 48, 157, 139, Orange, true );
            d.Arrow( 604, 81, 604, 120, Blue, true );
            d.Text( "Root chord", 73, 13, 200, Orange, 13, true );
            d.Text( "Tip chord", 504, 37, 190, Blue, 13, true );
            d.Text( "One wing section", 277, 81, 240 );
            d.Note( "Taper ratio = tip chord / root chord. Equal chords give a ratio of 1." );
            break;
        case Diagram::Sweep:
            d.Polygon( { { 69, 72 }, { 301, 72 }, { 301, 119 }, { 69, 119 } } );
            d.Polygon( { { 404, 56 }, { 638, 104 }, { 638, 143 }, { 404, 113 } } );
            d.Text( "Straight leading edge", 62, 23, 270 );
            d.Text( "Swept leading edge", 408, 23, 270 );
            d.Note( "Always state the measurement line: leading edge, quarter-chord or another chord location." );
            break;
        case Diagram::Dihedral:
            d.Line( 86, 61, 358, 129 );
            d.Line( 358, 129, 633, 61 );
            d.Polygon( { { 340, 111 }, { 375, 111 }, { 375, 144 }, { 340, 144 } } );
            fl_color( Muted );
            fl_line_style( FL_DASH, 1 );
            fl_line( d.X( 82 ), d.Y( 129 ), d.X( 640 ), d.Y( 129 ) );
            fl_line_style( 0 );
            d.Text( "Dihedral: upward tilt, seen from the front", 144, 16, 530, Blue, 13, true );
            d.Note( "Twist is different: it changes an airfoil's angle along the wing." );
            break;
        case Diagram::Airfoil:
        {
            fl_color( Pale );
            fl_begin_polygon();
            for ( int side = 1; side >= -1; side -= 2 )
            {
                for ( int j = 0; j <= 80; ++j )
                {
                    const double u = side == 1 ? j / 80.0 : 1.0 - j / 80.0;
                    const double thick = 90.0 * std::sqrt( u ) * ( 1.0 - u );
                    const double camber = -21.0 * std::sin( 3.141592653589793 * u );
                    fl_vertex( d.X( 106 + 498 * u ), d.Y( 108 + camber - side * thick ) );
                }
            }
            fl_end_polygon();
            d.Line( 106, 108, 604, 108, Muted );
            d.Arrow( 106, 143, 604, 143, Blue, true );
            d.Text( "Chord: front to back", 254, 116, 290, Blue, 12, true );
            d.Text( "Front / leading edge", 51, 24, 240 );
            d.Text( "Back / trailing edge", 458, 24, 244 );
            d.Note( "Airfoil cross-section. Camber is its middle-line bend; thickness is a separate dimension." );
            break;
        }
        case Diagram::Cg:
            d.Line( 91, 115, 634, 115, Muted );
            d.Box( 183, 62, 111, 51, "Mass" );
            d.Box( 443, 62, 111, 51, "Mass" );
            d.Polygon( { { 369, 118 }, { 343, 151 }, { 395, 151 } } );
            d.Arrow( 364, 32, 364, 100 );
            d.Text( "CG: mass balance point", 392, 14, 303, Blue, 13, true );
            d.Note( "Move a mass and the balance point moves. Stability still needs aerodynamic analysis." );
            break;
        case Diagram::Units:
            d.Box( 47, 47, 246, 74, "Geometry: length = 7" );
            d.Arrow( 313, 83, 397, 83 );
            d.Box( 416, 47, 258, 74, "Analysis: metres or feet?" );
            d.Note( "Tell each analysis what one model unit means. Unit selection is not always rescaling." );
            break;
        case Diagram::Axes:
            d.Arrow( 319, 132, 596, 132 );
            d.Arrow( 319, 132, 150, 61, Orange );
            d.Arrow( 319, 132, 319, 30 );
            d.Text( "X", 610, 115, 50, Blue, 14, true );
            d.Text( "Y", 111, 43, 50, Orange, 14, true );
            d.Text( "Z", 335, 12, 50, Blue, 14, true );
            d.Note( "Coordinate axes locate parts. Relative coordinates use the chosen parent / attachment frame." );
            break;
        case Diagram::Body:
        case Diagram::Measure:
            d.Polygon( { { 75, 113 }, { 146, 79 }, { 497, 79 }, { 648, 113 }, { 501, 134 }, { 146, 134 } } );
            for ( int i = 0; i < 5; ++i ) d.Line( 159 + i * 72, 80, 159 + i * 72, 133, Muted );
            d.Arrow( 76, 45, 647, 45, Blue, true );
            d.Text( m_Diagram == Diagram::Body ? "Body made from cross-sections" : "Measure dimensions, areas and volumes", 155, 7, 500, Blue, 13, true );
            d.Note( m_Diagram == Diagram::Body ? "Section sizes and positions describe the outside shape, not the real internal structure." : "Geometry measurements do not tell you maximum speed, altitude or structural strength." );
            break;
        case Diagram::Tail:
            d.Polygon( { { 346, 22 }, { 368, 22 }, { 379, 151 }, { 337, 151 } } );
            d.Polygon( { { 136, 65 }, { 576, 65 }, { 576, 103 }, { 136, 103 } } );
            d.Polygon( { { 262, 129 }, { 450, 129 }, { 450, 148 }, { 262, 148 } } );
            d.Text( "Main wing", 83, 20, 210 );
            d.Text( "Horizontal tail", 476, 124, 230 );
            d.Note( "Top view of a conventional layout. Tail size and position require trim and stability checks." );
            break;
        case Diagram::Results:
            d.Box( 40, 57, 176, 63, "CL = 0.6" );
            d.Box( 269, 57, 177, 63, "CD = 0.03" );
            d.Box( 498, 57, 183, 63, "L/D = 20" );
            d.Text( "/", 235, 76, 30, Muted, 17 );
            d.Text( "=", 463, 76, 30, Muted, 17 );
            d.Note( "Worked example only, not a result for your model. Coefficients need consistent references." );
            break;
        case Diagram::Altitude:
            d.Box( 27, 44, 199, 67, "Aircraft mass" );
            d.Box( 255, 44, 213, 67, "Lift + total drag" );
            d.Box( 497, 44, 198, 67, "Engine capability" );
            d.Text( "+", 233, 66, 25, Blue, 17 );
            d.Text( "+", 473, 66, 25, Blue, 17 );
            d.Text( "All are needed before estimating performance.", 111, 121, 600, Blue, 13, true );
            d.Note( "An altitude input is a requested condition, not a computed flight ceiling." );
            break;
        case Diagram::Mesh:
            for ( int r = 0; r < 3; ++r )
            {
                for ( int c = 0; c < 6; ++c )
                {
                    const double xx = 105 + c * 84;
                    const double yy = 36 + r * 36;
                    d.Line( xx, yy, xx + 84, yy, Muted );
                    d.Line( xx, yy, xx, yy + 36, Muted );
                    d.Line( xx, yy, xx + 84, yy + 36, Blue );
                }
            }
            d.Line( 105, 144, 609, 144, Muted );
            d.Line( 609, 36, 609, 144, Muted );
            d.Note( "Small elements approximate a surface. More elements do not fix incorrect physical assumptions." );
            break;
        case Diagram::Workflow:
            d.Box( 29, 49, 180, 65, "Your question" );
            d.Arrow( 222, 82, 253, 82 );
            d.Box( 267, 49, 179, 65, "Choose a tool" );
            d.Arrow( 459, 82, 490, 82 );
            d.Box( 503, 49, 187, 65, "Check the limits" );
            d.Note( "Use a tool for a specific question; not every tool belongs in every analysis." );
            break;
    }
    fl_line_style( 0 );
    fl_pop_clip();
}
