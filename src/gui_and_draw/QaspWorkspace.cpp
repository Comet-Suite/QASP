// This file is released under the NASA Open Source Agreement (NOSA) 1.3.
// See LICENSE. QASP addition; icons are original vector drawings, not icon fonts.
#include "QaspWorkspace.h"
#include <FL/Fl.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/Fl_Tooltip.H>
#include <FL/fl_draw.H>
#include <algorithm>
#include <utility>

namespace
{
Fl_Color Ink() { return fl_rgb_color( 33, 44, 61 ); }
Fl_Color Muted() { return fl_rgb_color( 104, 117, 135 ); }
Fl_Color Paper() { return fl_rgb_color( 246, 248, 251 ); }
Fl_Color Line() { return fl_rgb_color( 218, 224, 232 ); }
Fl_Color Accent() { return fl_rgb_color( 47, 111, 223 ); }

enum Icon { Tree, Sliders, Chart, Book, PlusFile, Folder, Disk, Fit, Top, Front, Iso, Cross };
void DrawIcon( int icon, int x, int y, Fl_Color color )
{
    fl_color( color );
    fl_line_style( FL_SOLID, 2 );
    switch ( icon )
    {
    case Tree:
        fl_rect( x + 2, y + 2, 7, 6 );
        fl_line( x + 5, y + 8, x + 5, y + 20 );
        fl_line( x + 5, y + 12, x + 12, y + 12 );
        fl_line( x + 5, y + 20, x + 12, y + 20 );
        fl_rect( x + 13, y + 9, 8, 6 ); fl_rect( x + 13, y + 17, 8, 6 );
        break;
    case Sliders:
        for ( int i = 0; i < 3; ++i )
        {
            int yy = y + 5 + i * 7, xx = x + ( i == 1 ? 15 : 8 );
            fl_line( x + 2, yy, x + 22, yy );
            fl_rectf( xx - 3, yy - 3, 6, 6 );
        }
        break;
    case Chart:
        fl_line( x + 3, y + 2, x + 3, y + 22, x + 23, y + 22 );
        fl_line( x + 6, y + 17, x + 11, y + 11, x + 16, y + 14 );
        fl_line( x + 16, y + 14, x + 22, y + 5 );
        break;
    case Book:
        fl_line( x + 12, y + 5, x + 12, y + 23 );
        fl_loop( x + 2, y + 3, x + 12, y + 5, x + 12, y + 23, x + 2, y + 21 );
        fl_loop( x + 12, y + 5, x + 22, y + 3, x + 22, y + 21, x + 12, y + 23 );
        break;
    case PlusFile:
        fl_rect( x + 4, y + 2, 14, 20 );
        fl_line( x + 8, y + 12, x + 16, y + 12 ); fl_line( x + 12, y + 8, x + 12, y + 16 );
        break;
    case Folder:
        fl_line( x + 2, y + 19, x + 2, y + 5, x + 9, y + 5 );
        fl_line( x + 9, y + 5, x + 12, y + 8, x + 22, y + 8 );
        fl_loop( x + 2, y + 20, x + 6, y + 11, x + 23, y + 11, x + 20, y + 20 );
        break;
    case Disk:
        fl_rect( x + 3, y + 3, 19, 19 ); fl_rect( x + 7, y + 3, 10, 6 );
        fl_rect( x + 7, y + 14, 11, 8 );
        break;
    case Fit:
        fl_line( x + 3, y + 9, x + 3, y + 3, x + 9, y + 3 );
        fl_line( x + 16, y + 3, x + 22, y + 3, x + 22, y + 9 );
        fl_line( x + 3, y + 16, x + 3, y + 22, x + 9, y + 22 );
        fl_line( x + 16, y + 22, x + 22, y + 22, x + 22, y + 16 );
        fl_line( x + 9, y + 12, x + 16, y + 12 ); fl_line( x + 12, y + 9, x + 12, y + 16 );
        break;
    case Top:
        fl_line( x + 12, y + 2, x + 12, y + 23 );
        fl_line( x + 3, y + 14, x + 12, y + 8, x + 22, y + 14 );
        fl_line( x + 7, y + 21, x + 12, y + 18, x + 18, y + 21 );
        break;
    case Front:
        fl_line( x + 2, y + 12, x + 23, y + 12 );
        fl_circle( x + 12, y + 12, 4 ); fl_line( x + 12, y + 3, x + 12, y + 8 );
        break;
    case Iso:
        fl_loop( x + 12, y + 2, x + 22, y + 7, x + 12, y + 13, x + 2, y + 7 );
        fl_line( x + 2, y + 7, x + 2, y + 18, x + 12, y + 23 );
        fl_line( x + 12, y + 13, x + 12, y + 23, x + 22, y + 18 );
        fl_line( x + 22, y + 18, x + 22, y + 7 );
        break;
    case Cross:
        fl_line( x + 7, y + 7, x + 17, y + 17 ); fl_line( x + 17, y + 7, x + 7, y + 17 );
        break;
    }
    fl_line_style( 0 );
}

Fl_Box *Text( int x, int y, int w, int h, const char *text, bool heading = false )
{
    Fl_Box *box = new Fl_Box( x, y, w, h, text );
    box->labelsize( heading ? 13 : 12 );
    box->labelfont( heading ? FL_HELVETICA_BOLD : FL_HELVETICA );
    box->labelcolor( heading ? Ink() : Muted() );
    box->align( FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_WRAP | FL_ALIGN_CLIP );
    return box;
}
}

class QaspIconButton : public Fl_Button
{
public:
    QaspIconButton( int x, int y, int w, int h, int icon, bool rail = false ) :
        Fl_Button( x, y, w, h ), m_Icon( icon ), m_Rail( rail ) {}
    void Selected( bool selected ) { if ( selected != m_Selected ) { m_Selected = selected; redraw(); } }
    void draw() override
    {
        Fl_Color background = m_Rail ? Ink() : Paper();
        if ( m_Hover || value() ) { background = m_Rail ? fl_rgb_color( 50, 64, 83 ) : fl_rgb_color( 227, 234, 244 ); }
        fl_color( background ); fl_rectf( x(), y(), w(), h() );
        if ( m_Selected && m_Rail ) { fl_color( fl_rgb_color( 120, 173, 255 ) ); fl_rectf( x(), y() + 8, 3, h() - 16 ); }
        DrawIcon( m_Icon, x() + ( w() - 24 ) / 2, y() + ( h() - 24 ) / 2,
                  m_Rail ? ( m_Selected ? FL_WHITE : fl_rgb_color( 176, 190, 209 ) ) : Ink() );
        if ( Fl::focus() == this ) { fl_color( Accent() ); fl_rect( x() + 4, y() + 4, w() - 8, h() - 8 ); }
    }
    int handle( int event ) override
    {
        if ( event == FL_ENTER || event == FL_LEAVE ) { m_Hover = event == FL_ENTER; redraw(); }
        return Fl_Button::handle( event );
    }
private:
    int m_Icon;
    bool m_Rail, m_Selected = false, m_Hover = false;
};

class QaspTabStrip : public Fl_Widget
{
public:
    using Callback = std::function<void( std::size_t )>;
    Callback select, close;
    std::function<void()> create;
    QaspTabStrip( int x, int y, int w, int h ) : Fl_Widget( x, y, w, h )
    {
        tooltip( "Project tabs. Ctrl+Tab / Ctrl+Shift+Tab switch projects; Ctrl+W closes. Scroll for more tabs." );
    }
    void Set( const std::vector<QaspTabLabel> &tabs, std::size_t active )
    {
        bool changed = m_Active != active || tabs.size() != m_Tabs.size();
        if ( !changed )
        {
            for ( std::size_t i = 0; i < tabs.size(); ++i )
            {
                if ( tabs[i].title != m_Tabs[i].title || tabs[i].dirty != m_Tabs[i].dirty ||
                     tabs[i].tooltip != m_Tabs[i].tooltip ) { changed = true; break; }
            }
        }
        if ( !changed ) { return; }
        bool navigate = m_Active != active || tabs.size() != m_Tabs.size();
        m_Tabs = tabs;
        m_Active = active;
        if ( navigate ) { Reveal(); }
        redraw();
    }
    void resize( int x, int y, int w, int h ) override { Fl_Widget::resize( x, y, w, h ); Reveal(); }
    void draw() override
    {
        fl_color( fl_rgb_color( 234, 238, 244 ) ); fl_rectf( x(), y(), w(), h() );
        fl_font( FL_HELVETICA, 12 );
        int xx = x() - m_Offset;
        fl_push_clip( x(), y(), Usable(), h() );
        for ( std::size_t i = 0; i < m_Tabs.size(); ++i )
        {
            int tw = Width( i );
            fl_color( i == m_Active ? FL_WHITE : fl_rgb_color( 237, 241, 246 ) );
            fl_rectf( xx, y(), tw, h() );
            fl_color( Line() ); fl_line( xx + tw - 1, y() + 7, xx + tw - 1, y() + h() - 7 );
            fl_color( i == m_Active ? Accent() : Muted() );
            fl_rect( xx + 12, y() + 10, 10, 13 );
            fl_line( xx + 15, y() + 14, xx + 20, y() + 14 );
            fl_color( i == m_Active ? Ink() : Muted() );
            fl_push_clip( xx + 31, y(), tw - 75, h() );
            fl_draw( m_Tabs[i].title.c_str(), xx + 31, y(), tw - 75, h(), FL_ALIGN_LEFT | FL_ALIGN_INSIDE );
            fl_pop_clip();
            if ( m_Tabs[i].dirty ) { fl_color( Accent() ); fl_pie( xx + tw - 38, y() + 14, 6, 6, 0, 360 ); }
            fl_color( Muted() );
            fl_line( xx + tw - 22, y() + 14, xx + tw - 15, y() + 21 );
            fl_line( xx + tw - 15, y() + 14, xx + tw - 22, y() + 21 );
            if ( i == m_Active ) { fl_color( Accent() ); fl_rectf( xx, y() + h() - 2, tw, 2 ); }
            xx += tw;
        }
        fl_pop_clip();
        fl_color( fl_rgb_color( 234, 238, 244 ) ); fl_rectf( x() + Usable(), y(), w() - Usable(), h() );
        fl_color( m_Tabs.size() > 1 ? Ink() : Line() );
        int nav = x() + w() - 82;
        fl_line( nav + 3, y() + 13, nav - 2, y() + 18, nav + 3, y() + 23 );
        nav += 28;
        fl_line( nav - 2, y() + 13, nav + 3, y() + 18, nav - 2, y() + 23 );
        fl_color( Ink() ); int plus = x() + w() - 20;
        fl_line( plus - 5, y() + 18, plus + 5, y() + 18 ); fl_line( plus, y() + 13, plus, y() + 23 );
        fl_color( Line() ); fl_line( x(), y() + h() - 1, x() + w(), y() + h() - 1 );
    }
    int handle( int event ) override
    {
        if ( event == FL_FOCUS || event == FL_UNFOCUS ) { redraw(); return 1; }
        if ( event == FL_MOUSEWHEEL )
        {
            m_Offset = std::max( 0, std::min( Total() - Usable(), m_Offset + ( Fl::event_dy() + Fl::event_dx() ) * 80 ) );
            redraw(); return 1;
        }
        if ( event == FL_PUSH && Fl::event_button() == FL_LEFT_MOUSE )
        {
            take_focus();
            int ex = Fl::event_x() - x();
            if ( ex >= w() - 36 ) { if ( create ) { create(); } return 1; }
            if ( ex >= Usable() )
            {
                if ( m_Tabs.size() > 1 && select )
                {
                    std::size_t next = ex < w() - 68 ? ( m_Active + m_Tabs.size() - 1 ) % m_Tabs.size() :
                                                                      ( m_Active + 1 ) % m_Tabs.size();
                    select( next );
                }
                return 1;
            }
            int xx = -m_Offset;
            for ( std::size_t i = 0; i < m_Tabs.size(); ++i )
            {
                int tw = Width( i );
                if ( ex >= xx && ex < xx + tw )
                {
                    if ( ex > xx + tw - 29 ) { if ( close ) { close( i ); } }
                    else if ( select ) { select( i ); }
                    return 1;
                }
                xx += tw;
            }
            return 1;
        }
        if ( event == FL_MOVE )
        {
            int xx = x() - m_Offset;
            for ( std::size_t i = 0; i < m_Tabs.size(); ++i )
            {
                int tw = Width( i );
                if ( Fl::event_x() >= xx && Fl::event_x() < xx + tw && Fl::event_x() < x() + Usable() )
                {
                    copy_tooltip( m_Tabs[i].tooltip.c_str() ); break;
                }
                xx += tw;
            }
            return 1;
        }
        return Fl_Widget::handle( event );
    }
private:
    std::vector<QaspTabLabel> m_Tabs;
    std::size_t m_Active = 0;
    int m_Offset = 0;
    int Usable() const { return std::max( 1, w() - 100 ); }
    int Width( std::size_t i ) const
    {
        fl_font( FL_HELVETICA, 12 );
        return std::max( 150, std::min( 230, static_cast<int>( fl_width( m_Tabs[i].title.c_str() ) ) + 82 ) );
    }
    int Total() const { int sum = 0; for ( std::size_t i = 0; i < m_Tabs.size(); ++i ) { sum += Width( i ); } return sum; }
    void Reveal()
    {
        int left = 0;
        for ( std::size_t i = 0; i < m_Active && i < m_Tabs.size(); ++i ) { left += Width( i ); }
        if ( left < m_Offset ) { m_Offset = left; }
        if ( m_Active < m_Tabs.size() && left + Width( m_Active ) > m_Offset + Usable() )
        { m_Offset = left + Width( m_Active ) - Usable(); }
        m_Offset = std::max( 0, std::min( std::max( 0, Total() - Usable() ), m_Offset ) );
    }
};

struct QaspDockArea::Hosted
{
    std::unique_ptr<Fl_Widget_Tracker> tracker;
    int width, height, x, y;
};

QaspDockArea::QaspDockArea( int x, int y, int w, int h, const char *message ) : Fl_Group( x, y, w, h )
{
    box( FL_FLAT_BOX ); color( Paper() );
    begin();
    m_Clip = new Fl_Double_Window( x, y, std::max( 1, w ), std::max( 1, h ) );
    m_Clip->color( Paper() ); m_Clip->border( 0 );
    m_Clip->begin();
    m_Empty = Text( 16, 20, std::max( 1, w - 32 ), std::max( 1, h - 40 ), message );
    m_Empty->align( FL_ALIGN_TOP_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_WRAP | FL_ALIGN_CLIP );
    m_Clip->end();
    m_Horizontal = new Fl_Scrollbar( x, y + h - 16, w, 16 ); m_Horizontal->type( FL_HORIZONTAL );
    m_Vertical = new Fl_Scrollbar( x + w - 16, y, 16, h );
    m_Horizontal->callback( ScrollCB, this ); m_Vertical->callback( ScrollCB, this );
    end();
    Layout();
}
QaspDockArea::~QaspDockArea() { DetachAll(); }
void QaspDockArea::Host( Fl_Double_Window *window )
{
    if ( !window || Contains( window ) ) { return; }
    std::unique_ptr<Hosted> host( new Hosted );
    host->tracker.reset( new Fl_Widget_Tracker( window ) );
    host->width = window->w(); host->height = window->h(); host->x = window->x(); host->y = window->y();
    window->hide();
    m_Clip->add( window );
    window->position( 0, 0 );
    m_Hosted.push_back( std::move( host ) );
    Layout();
}
bool QaspDockArea::Contains( const Fl_Double_Window *window ) const
{
    for ( const auto &p : m_Hosted ) { if ( p->tracker->widget() == window ) { return true; } }
    return false;
}
Fl_Double_Window *QaspDockArea::ActiveWindow() const
{
    for ( const auto &p : m_Hosted )
    {
        if ( !p->tracker->deleted() && p->tracker->widget()->visible() )
        { return static_cast<Fl_Double_Window *>( p->tracker->widget() ); }
    }
    return nullptr;
}
void QaspDockArea::Synchronize()
{
    if ( ActiveWindow() != m_Previous )
    {
        m_Horizontal->value( 0 ); m_Vertical->value( 0 );
        m_Previous = ActiveWindow();
    }
    Layout();
}
void QaspDockArea::DetachAll()
{
    for ( const auto &p : m_Hosted )
    {
        if ( !p->tracker->deleted() )
        {
            auto *window = static_cast<Fl_Double_Window *>( p->tracker->widget() );
            window->hide(); m_Clip->remove( window );
            window->resize( p->x, p->y, p->width, p->height );
        }
    }
    m_Hosted.clear(); m_Previous = nullptr;
}
void QaspDockArea::resize( int x, int y, int w, int h ) { Fl_Widget::resize( x, y, w, h ); Layout(); }
int QaspDockArea::HorizontalOffset() const { return static_cast<int>( m_Horizontal->value() ); }
int QaspDockArea::VerticalOffset() const { return static_cast<int>( m_Vertical->value() ); }
void QaspDockArea::ScrollCB( Fl_Widget *, void *data ) { static_cast<QaspDockArea *>( data )->Layout(); }
int QaspDockArea::handle( int event )
{
    if ( Fl_Group::handle( event ) ) { return 1; }
    if ( event == FL_MOUSEWHEEL && m_Vertical->visible() )
    {
        double value = m_Vertical->clamp( m_Vertical->value() + Fl::event_dy() * 28 );
        m_Vertical->value( value ); Layout(); return 1;
    }
    return 0;
}
void QaspDockArea::Layout()
{
    if ( m_LayingOut ) { return; }
    m_LayingOut = true;
    Fl_Double_Window *active = ActiveWindow();
    int naturalW = 0, naturalH = 0;
    for ( const auto &p : m_Hosted )
    {
        if ( p->tracker->widget() == active ) { naturalW = p->width; naturalH = p->height; break; }
    }
    bool horizontal = naturalW > w(), vertical = naturalH > h();
    horizontal = naturalW > w() - ( vertical ? 16 : 0 );
    vertical = naturalH > h() - ( horizontal ? 16 : 0 );
    horizontal = naturalW > w() - ( vertical ? 16 : 0 );
    int vw = std::max( 1, w() - ( vertical ? 16 : 0 ) ), vh = std::max( 1, h() - ( horizontal ? 16 : 0 ) );
    int contentW = std::max( vw, naturalW ), contentH = std::max( vh, naturalH );
    int sx = std::max( 0, std::min( contentW - vw, HorizontalOffset() ) );
    int sy = std::max( 0, std::min( contentH - vh, VerticalOffset() ) );
    m_Clip->resize( x(), y(), vw, vh );
    m_Empty->resize( 16, 20, std::max( 1, vw - 32 ), std::max( 1, vh - 40 ) );
    m_Horizontal->resize( x(), y() + vh, vw, 16 ); m_Horizontal->value( sx, vw, 0, contentW );
    m_Vertical->resize( x() + vw, y(), 16, vh ); m_Vertical->value( sy, vh, 0, contentH );
    if ( horizontal ) { m_Horizontal->show(); } else { m_Horizontal->hide(); }
    if ( vertical ) { m_Vertical->show(); } else { m_Vertical->hide(); }
    if ( active )
    {
        m_Empty->hide();
        if ( active->x() != -sx || active->y() != -sy || active->w() != contentW || active->h() != contentH )
        { active->resize( -sx, -sy, contentW, contentH ); }
    }
    else { m_Empty->show(); }
    m_Clip->redraw();
    m_LayingOut = false;
}

class QaspSidebarDivider : public Fl_Widget
{
public:
    QaspSidebarDivider( QaspWorkspace *owner ) : Fl_Widget( 0, 0, 5, 5 ), m_Owner( owner )
    { tooltip( "Drag to resize this panel. Click its activity icon again to hide it." ); }
    void draw() override
    { fl_color( m_Hover || m_Dragging ? Accent() : Line() ); fl_rectf( x(), y(), w(), h() ); }
    int handle( int event ) override
    {
        if ( event == FL_ENTER || event == FL_LEAVE )
        { m_Hover = event == FL_ENTER; if ( window() ) { window()->cursor( m_Hover ? FL_CURSOR_WE : FL_CURSOR_DEFAULT ); } redraw(); return 1; }
        if ( event == FL_PUSH && Fl::event_button() == FL_LEFT_MOUSE )
        { m_Dragging = true; m_Start = Fl::event_x(); m_Width = m_Owner->SidebarWidth(); redraw(); return 1; }
        if ( event == FL_DRAG && m_Dragging ) { m_Owner->SetSidebarWidth( m_Width + Fl::event_x() - m_Start ); return 1; }
        if ( event == FL_RELEASE && m_Dragging )
        { m_Dragging = false; if ( window() ) { window()->cursor( FL_CURSOR_DEFAULT ); } redraw(); return 1; }
        return 0;
    }
private:
    QaspWorkspace *m_Owner;
    bool m_Hover = false, m_Dragging = false;
    int m_Start = 0, m_Width = 300;
};

QaspWorkspace::QaspWorkspace( int x, int y, int w, int h ) : Fl_Group( x, y, w, h )
{
    box( FL_FLAT_BOX ); color( Paper() ); begin();
    m_Rail = new Fl_Group( x, y, 48, h ); m_Rail->box( FL_FLAT_BOX ); m_Rail->color( Ink() );
    m_Rail->begin();
    Fl_Box *brand = new Fl_Box( x, y + 4, 48, 38, "Q" );
    brand->labelfont( FL_HELVETICA_BOLD ); brand->labelsize( 23 ); brand->labelcolor( FL_WHITE );
    const char *tips[] = { "Components: add, select and arrange aircraft parts. Click again to hide.",
                          "Properties: edit the selected component. Click again to hide.",
                          "Analysis: choose a calculation tool. Click again to hide.",
                          "Learn: short illustrated lessons and their limits. Click again to hide." };
    for ( int i = 0; i < PanelCount; ++i )
    {
        m_Activity[i] = new QaspIconButton( x, y + 48 + i * 50, 48, 48, i, true );
        m_Activity[i]->tooltip( tips[i] ); Bind( m_Activity[i], i );
    }
    m_Rail->end();
    m_Tabs = new QaspTabStrip( x + 48, y, w - 48, 34 );
    m_Tabs->create = [this]() { if ( m_OnAction ) { m_OnAction( NewProject ); } };
    m_Toolbar = new Fl_Group( x + 48, y + 34, w - 48, 30 );
    m_Toolbar->box( FL_FLAT_BOX ); m_Toolbar->color( Paper() ); m_Toolbar->begin();
    const int icons[] = { PlusFile, Folder, Disk, Fit, Top, Front, Iso };
    const int commands[] = { NewProject, OpenProject, SaveProject, FitModel, TopView, FrontView, IsoView };
    const char *tooltips[] = { "New project (Ctrl+N). Keeps the current project open in its own tab.",
        "Open project in a tab (Ctrl+O). Existing unsaved models are kept.", "Save active project (Ctrl+S).",
        "Fit the visible model into all viewports (F).", "Top view", "Front view", "Isometric view" };
    for ( int i = 0; i < 7; ++i )
    {
        auto *button = new QaspIconButton( x + 56 + i * 36 + ( i > 2 ? 12 : 0 ), y + 35, 28, 28, icons[i] );
        button->tooltip( tooltips[i] ); Bind( button, commands[i] ); m_ToolButtons.push_back( button );
    }
    m_Caption = Text( x + 340, y + 34, w - 356, 30, "DESIGN WORKSPACE" );
    m_Caption->labelsize( 11 ); m_Caption->align( FL_ALIGN_RIGHT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP );
    m_Toolbar->end();
    m_PanelTitle = Text( x + 62, y + 72, 240, 32, "COMPONENTS", true ); m_PanelTitle->labelsize( 11 );
    m_ClosePanel = new QaspIconButton( x + 310, y + 74, 28, 28, Cross );
    m_ClosePanel->tooltip( "Hide this panel. Reopen it with its activity icon." ); Bind( m_ClosePanel, -1 );
    m_Divider = new QaspSidebarDivider( this );
    m_Components = new QaspDockArea( x + 48, y + 104, 300, h - 104,
        "Add a component above. Select it in the tree, then open Properties with the sliders icon." );
    m_Properties = new QaspDockArea( x + 48, y + 104, 460, h - 104,
        "Select a component in Components to edit it here.\n\nUse the tree icon to return to the model's parts." );
    m_Analysis = new Fl_Group( x + 48, y + 104, 300, h - 104 );
    m_Analysis->box( FL_FLAT_BOX ); m_Analysis->color( Paper() ); m_Analysis->begin();
    Text( x + 64, y + 120, 268, 28, "Understand your design", true );
    Text( x + 64, y + 150, 268, 72, "Choose a tool. Results are estimates, not a flight-safety approval. Check inputs, units and assumptions." );
    const char *names[] = { "Mass and centre of gravity", "Parasite drag", "VSPAERO aerodynamics", "CFD mesh preparation", "FEA structure" };
    for ( int i = 0; i < 5; ++i )
    {
        Fl_Button *button = new Fl_Button( x + 64, y + 238 + i * 42, 268, 34, names[i] );
        button->labelsize( 12 ); Bind( button, MassProperties + i );
    }
    m_Analysis->end();
    m_Learn = new Fl_Group( x + 48, y + 104, 320, h - 104 );
    m_Learn->box( FL_FLAT_BOX ); m_Learn->color( Paper() ); m_Learn->begin();
    Text( x + 64, y + 120, 280, 30, "One idea at a time", true );
    Text( x + 64, y + 158, 280, 112,
        "Short illustrated lessons explain what a term means, what changes on an aircraft, and one thing to try.\n\nNo engineering background required." );
    Fl_Button *guide = new Fl_Button( x + 64, y + 286, 280, 36, "Open the illustrated Field Guide" );
    guide->labelsize( 12 ); Bind( guide, FieldGuide );
    Text( x + 64, y + 340, 280, 110,
        "Start with the first-model lesson, then explore wing size, balance, lift and drag.\n\nIllustrations explain concepts; they are not simulation results." );
    m_Learn->end();
    m_Canvas = new Fl_Group( x + 353, y + 72, w - 353, h - 72 ); m_Canvas->end();
    end(); Layout();
}
QaspWorkspace::~QaspWorkspace()
{
    m_Components->DetachAll(); m_Properties->DetachAll();
}
void QaspWorkspace::Bind( Fl_Widget *widget, int command ) { m_Commands[widget] = command; widget->callback( ButtonCB, this ); }
void QaspWorkspace::ButtonCB( Fl_Widget *widget, void *data )
{
    auto *self = static_cast<QaspWorkspace *>( data );
    int command = self->m_Commands.at( widget );
    if ( command >= 0 && command < PanelCount ) { self->TogglePanel( static_cast<Panel>( command ) ); }
    else if ( command == -1 ) { self->HidePanel(); }
    if ( self->m_OnAction ) { self->m_OnAction( command ); }
}
void QaspWorkspace::ShowPanel( Panel panel )
{
    if ( panel < Components || panel >= PanelCount ) { return; }
    m_CurrentPanel = panel; m_LastPanel = panel; Layout(); Synchronize(); redraw();
}
void QaspWorkspace::TogglePanel( Panel panel ) { if ( m_CurrentPanel == panel ) { HidePanel(); } else { ShowPanel( panel ); } }
void QaspWorkspace::HidePanel()
{
    if ( m_CurrentPanel >= 0 ) { m_LastPanel = m_CurrentPanel; }
    m_CurrentPanel = -1; Layout(); redraw();
}
void QaspWorkspace::ToggleSidebar()
{
    if ( m_CurrentPanel >= 0 ) { HidePanel(); }
    else { ShowPanel( static_cast<Panel>( m_LastPanel ) ); }
}
void QaspWorkspace::SetSidebarWidth( int width )
{
    if ( m_CurrentPanel < 0 ) { return; }
    int maximum = std::max( 120, w() - 48 - 5 - 320 );
    m_Widths[m_CurrentPanel] = std::max( std::min( 260, maximum ), std::min( width, maximum ) );
    Layout(); redraw();
}
int QaspWorkspace::SidebarWidth() const
{
    if ( m_CurrentPanel < 0 ) { return 0; }
    return std::min( m_Widths[m_CurrentPanel], std::max( 120, w() - 48 - 5 - 320 ) );
}
void QaspWorkspace::SetViewport( Fl_Widget *viewport ) { m_Viewport = viewport; m_Canvas->add( viewport ); Layout(); }
void QaspWorkspace::resize( int x, int y, int w, int h ) { Fl_Widget::resize( x, y, w, h ); Layout(); }
void QaspWorkspace::SetTabs( const std::vector<QaspTabLabel> &tabs, std::size_t active ) { m_Tabs->Set( tabs, active ); }
void QaspWorkspace::OnSelectTab( std::function<void( std::size_t )> callback ) { m_Tabs->select = std::move( callback ); }
void QaspWorkspace::OnCloseTab( std::function<void( std::size_t )> callback ) { m_Tabs->close = std::move( callback ); }
void QaspWorkspace::SetProjectCaption( const std::string &caption ) { m_Caption->copy_label( caption.c_str() ); }
void QaspWorkspace::Synchronize() { m_Components->Synchronize(); m_Properties->Synchronize(); }
void QaspWorkspace::Layout()
{
    int rail = 48, tabs = 34, tools = 30, head = 32;
    int bx = x() + rail, by = y() + tabs + tools, bh = std::max( 1, h() - tabs - tools );
    int sidebar = SidebarWidth();
    m_Rail->resize( x(), y(), rail, h() );
    m_Rail->child( 0 )->resize( x(), y() + 4, rail, 38 );
    for ( int i = 0; i < PanelCount; ++i )
    {
        m_Activity[i]->resize( x(), y() + 48 + i * 50, 48, 48 );
        m_Activity[i]->Selected( m_CurrentPanel == i );
    }
    m_Tabs->resize( bx, y(), std::max( 1, w() - rail ), tabs );
    m_Toolbar->resize( bx, y() + tabs, std::max( 1, w() - rail ), tools );
    for ( std::size_t i = 0; i < m_ToolButtons.size(); ++i )
    { m_ToolButtons[i]->resize( bx + 8 + static_cast<int>( i ) * 36 + ( i > 2 ? 12 : 0 ), y() + tabs + 1, 28, 28 ); }
    m_Caption->resize( bx + 282, y() + tabs, std::max( 1, w() - rail - 298 ), tools );
    const char *titles[] = { "COMPONENTS", "COMPONENT PROPERTIES", "ANALYSIS", "LEARN" };
    if ( m_CurrentPanel >= 0 )
    {
        m_PanelTitle->copy_label( titles[m_CurrentPanel] );
        m_PanelTitle->resize( bx + 14, by, std::max( 1, sidebar - 50 ), head );
        m_ClosePanel->resize( bx + sidebar - 30, by + 2, 28, 28 );
        m_Divider->resize( bx + sidebar, by, 5, bh );
        m_PanelTitle->show(); m_ClosePanel->show(); m_Divider->show();
    }
    else { m_PanelTitle->hide(); m_ClosePanel->hide(); m_Divider->hide(); }
    Fl_Widget *panels[] = { m_Components, m_Properties, m_Analysis, m_Learn };
    for ( int i = 0; i < PanelCount; ++i )
    {
        panels[i]->resize( bx, by + head, std::max( 1, sidebar ), std::max( 1, bh - head ) );
        if ( m_CurrentPanel == i ) { panels[i]->show(); } else { panels[i]->hide(); }
    }
    // Keep launcher text/buttons usable when their sidebar is widened.
    for ( Fl_Group *panel : { m_Analysis, m_Learn } )
    {
        for ( int i = 0; i < panel->children(); ++i )
        {
            Fl_Widget *child = panel->child( i );
            child->resize( bx + 16, child->y(), std::max( 1, sidebar - 32 ), child->h() );
        }
    }
    int cx = bx + ( sidebar ? sidebar + 5 : 0 );
    m_Canvas->resize( cx, by, std::max( 1, x() + w() - cx ), bh );
    if ( m_Viewport ) { m_Viewport->resize( cx, by, m_Canvas->w(), bh ); }
}
