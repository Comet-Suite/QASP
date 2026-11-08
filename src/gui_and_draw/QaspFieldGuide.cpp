//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
// QASP addition, revised after user feedback. One focused concept per lesson:
// illustration, meaning, effect, example, something to try, and an honest limit.
// The actual component also compiles independently for visual and interaction QA.
// No browser, network request, runtime JSON or external image file is needed.
//
#include "QaspFieldGuide.h"
#include "QaspHelp.h"
#include "QaspLessonDiagram.h"

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Help_View.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Group.H>
#include <FL/fl_draw.H>

#include <algorithm>
#include <string>
#include <vector>

Fl_Window* QaspFieldGuide::s_Window = nullptr;
Fl_Hold_Browser* QaspFieldGuide::s_Topics = nullptr;
Fl_Help_View* QaspFieldGuide::s_Body = nullptr;
Fl_Input* QaspFieldGuide::s_Search = nullptr;
Fl_Box* QaspFieldGuide::s_Title = nullptr;
Fl_Box* QaspFieldGuide::s_Category = nullptr;
Fl_Box* QaspFieldGuide::s_Summary = nullptr;
Fl_Box* QaspFieldGuide::s_Count = nullptr;
Fl_Box* QaspFieldGuide::s_Progress = nullptr;
Fl_Button* QaspFieldGuide::s_Previous = nullptr;
Fl_Button* QaspFieldGuide::s_Next = nullptr;
QaspLessonDiagram* QaspFieldGuide::s_Diagram = nullptr;
int QaspFieldGuide::s_Current = -1;

namespace
{
std::vector<int> g_Visible;
Fl_Color Ink() { return fl_rgb_color( 30, 45, 65 ); }
Fl_Color Muted() { return fl_rgb_color( 92, 108, 129 ); }
Fl_Color Accent() { return fl_rgb_color( 0, 105, 172 ); }

Fl_Box* Label( int x, int y, int w, int h, const char* text, int size,
               Fl_Color color, bool bold = false )
{
    Fl_Box* box = new Fl_Box( x, y, w, h, text );
    box->labelsize( size );
    box->labelfont( bold ? FL_HELVETICA_BOLD : FL_HELVETICA );
    box->labelcolor( color );
    box->align( FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_WRAP );
    return box;
}
}

void QaspFieldGuide::BuildWindow()
{
    int sx, sy, sw, sh;
    Fl::screen_work_area( sx, sy, sw, sh );
    const int W = std::min( 1080, std::max( 820, sw - 60 ) );
    const int H = std::min( 760, std::max( 600, sh - 60 ) );
    s_Window = new Fl_Double_Window( sx + std::max( 0, ( sw - W ) / 2 ),
                                    sy + std::max( 0, ( sh - H ) / 2 ), W, H,
                                    "QASP Field Guide" );
    s_Window->color( FL_WHITE );
    s_Window->size_range( 820, 600 );
    s_Window->callback( CloseCB );
    s_Window->begin();

    Fl_Group* header = new Fl_Group( 0, 0, W, 64 );
    header->box( FL_FLAT_BOX );
    header->color( fl_rgb_color( 23, 40, 64 ) );
    Label( 22, 11, 78, 27, "QASP", 20, FL_WHITE, true );
    Label( 112, 10, W - 270, 25, "Field Guide", 19, FL_WHITE, true );
    Label( 112, 34, W - 270, 19, "One idea. One diagram. One thing to try.", 12,
           fl_rgb_color( 187, 211, 233 ) );
    Fl_Button* close = new Fl_Button( W - 106, 18, 84, 29, "Close" );
    close->color( fl_rgb_color( 48, 77, 108 ) );
    close->labelcolor( FL_WHITE );
    close->labelsize( 13 );
    close->callback( CloseCB );
    // Keep branding and left-aligned header text fixed when the window narrows;
    // only the middle spacer stretches, and Close stays aligned to the right.
    Fl_Box* headerSpacer = new Fl_Box( 420, 0, W - 548, 64 );
    header->resizable( headerSpacer );
    header->end();

    Fl_Group* sidebar = new Fl_Group( 0, 64, 250, H - 64 );
    sidebar->box( FL_FLAT_BOX );
    sidebar->color( fl_rgb_color( 244, 247, 251 ) );
    Label( 16, 79, 220, 22, "FIND A LESSON", 11, Muted(), true );
    s_Search = new Fl_Input( 16, 106, 155, 31 );
    s_Search->box( FL_THIN_UP_BOX );
    s_Search->color( FL_WHITE );
    s_Search->textsize( 13 );
    s_Search->textfont( FL_HELVETICA );
    s_Search->when( FL_WHEN_CHANGED );
    s_Search->copy_tooltip( "Search lesson titles and explanations. Try thrust, wing area, or CG." );
    s_Search->callback( SearchCB );
    Fl_Button* clear = new Fl_Button( 177, 106, 58, 31, "Clear" );
    clear->labelsize( 12 );
    clear->tooltip( "Clear the search and show every lesson." );
    clear->callback( ClearSearchCB );
    s_Count = Label( 16, 145, 219, 22, "", 12, Muted() );
    s_Topics = new Fl_Hold_Browser( 14, 175, 222, H - 261 );
    s_Topics->box( FL_FLAT_BOX );
    s_Topics->color( sidebar->color() );
    s_Topics->selection_color( Accent() );
    s_Topics->textsize( 13 );
    s_Topics->textfont( FL_HELVETICA );
    s_Topics->textcolor( Ink() );
    s_Topics->has_scrollbar( Fl_Browser::VERTICAL );
    s_Topics->tooltip( "Choose a short lesson. Use the arrow keys to move through this list." );
    s_Topics->callback( TopicCB );
    Label( 16, H - 73, 219, 58,
           "Hover help explains main controls.\nSome advanced controls still show\nlabelled reference text.", 11, Muted() );
    sidebar->resizable( s_Topics );
    sidebar->end();

    Fl_Group* article = new Fl_Group( 250, 64, W - 250, H - 64 );
    s_Category = Label( 278, 80, W - 310, 20, "", 11, Accent(), true );
    s_Title = Label( 278, 107, W - 310, 34, "", 23, Ink(), true );
    s_Summary = Label( 278, 143, W - 310, 35, "", 14, Muted() );
    s_Diagram = new QaspLessonDiagram( 278, 188, W - 310, 174 );
    s_Body = new Fl_Help_View( 269, 373, W - 287, H - 442 );
    s_Body->box( FL_FLAT_BOX );
    s_Body->color( FL_WHITE );
    s_Body->textfont( FL_HELVETICA );
    s_Body->textsize( 14 );
    s_Body->textcolor( Ink() );
    s_Previous = new Fl_Button( 278, H - 49, 108, 30, "< Previous" );
    s_Previous->labelsize( 12 );
    s_Previous->shortcut( FL_ALT + FL_Left );
    s_Previous->tooltip( "Previous matching lesson (Alt+Left)." );
    s_Previous->callback( NavigateCB );
    s_Progress = Label( 401, H - 49, W - 565, 30, "", 12, Muted() );
    s_Next = new Fl_Button( W - 142, H - 49, 110, 30, "Next >" );
    s_Next->labelsize( 12 );
    s_Next->color( Accent() );
    s_Next->labelcolor( FL_WHITE );
    s_Next->shortcut( FL_ALT + FL_Right );
    s_Next->tooltip( "Next matching lesson (Alt+Right)." );
    s_Next->callback( NavigateCB );
    article->resizable( s_Body );
    article->end();

    s_Window->resizable( article );
    s_Window->end();
    FilterTopics();
}

void QaspFieldGuide::Show( const char* lesson_id )
{
    if ( !s_Window ) BuildWindow();
    if ( lesson_id )
    {
        const int index = QaspHelp::FindLesson( lesson_id );
        if ( index >= 0 )
        {
            s_Search->value( "" );
            FilterTopics();
            LoadTopic( index );
        }
    }
    s_Window->show();
}

void QaspFieldGuide::LoadTopic( int index )
{
    if ( index < 0 ) return;
    const QaspHelp::Lesson* lesson = QaspHelp::LessonAt( static_cast<std::size_t>( index ) );
    if ( !lesson ) return;
    s_Current = index;
    s_Category->copy_label( lesson->category );
    s_Title->copy_label( lesson->title );
    s_Summary->copy_label( lesson->summary );
    s_Diagram->SetDiagram( lesson->diagram );
    const std::string html = QaspHelp::LessonHtml( static_cast<std::size_t>( index ) );
    s_Body->value( html.c_str() );
    s_Body->topline( 0 );
    s_Body->leftline( 0 );
    const auto pos = std::find( g_Visible.begin(), g_Visible.end(), index );
    if ( pos != g_Visible.end() )
    {
        const int row = static_cast<int>( pos - g_Visible.begin() ) + 1;
        s_Topics->value( row );
        s_Topics->middleline( row );
    }
    UpdateNavigation();
    s_Window->redraw();
}

void QaspFieldGuide::FilterTopics()
{
    const int old = s_Current;
    g_Visible = QaspHelp::SearchLessons( s_Search->value() ? s_Search->value() : "" );
    s_Topics->clear();
    for ( int index : g_Visible )
    {
        s_Topics->add( QaspHelp::LessonAt( static_cast<std::size_t>( index ) )->title );
    }
    std::string count = std::to_string( g_Visible.size() ) +
        ( g_Visible.size() == 1 ? " short lesson" : " short lessons" );
    s_Count->copy_label( count.c_str() );
    if ( !g_Visible.empty() )
    {
        LoadTopic( std::find( g_Visible.begin(), g_Visible.end(), old ) != g_Visible.end()
                   ? old : g_Visible.front() );
    }
    else
    {
        s_Current = -1;
        s_Category->copy_label( "Search" );
        s_Title->copy_label( "No matching lesson" );
        s_Summary->copy_label( "Try a shorter term, or Clear to show every lesson." );
        s_Diagram->Clear();
        s_Body->value( "<html><body><p>Search covers the explanations as well as titles.</p>"
                       "<p>Try <b>thrust</b>, <b>wing area</b>, <b>stall</b> or <b>CG</b>.</p></body></html>" );
        s_Body->topline( 0 );
        s_Body->leftline( 0 );
        UpdateNavigation();
        s_Window->redraw();
    }
}

void QaspFieldGuide::UpdateNavigation()
{
    const auto pos = std::find( g_Visible.begin(), g_Visible.end(), s_Current );
    s_Previous->deactivate();
    s_Next->deactivate();
    if ( pos == g_Visible.end() )
    {
        s_Progress->copy_label( "0 matching lessons" );
        return;
    }
    const std::size_t step = static_cast<std::size_t>( pos - g_Visible.begin() );
    const std::string progress = "Lesson " + std::to_string( step + 1 ) + " of " +
                                  std::to_string( g_Visible.size() );
    s_Progress->copy_label( progress.c_str() );
    if ( step > 0 ) s_Previous->activate();
    if ( step + 1 < g_Visible.size() ) s_Next->activate();
}

void QaspFieldGuide::TopicCB( Fl_Widget*, void* )
{
    const int row = s_Topics->value();
    if ( row > 0 && row <= static_cast<int>( g_Visible.size() ) ) LoadTopic( g_Visible[row - 1] );
}

void QaspFieldGuide::SearchCB( Fl_Widget*, void* )
{
    FilterTopics();
}

void QaspFieldGuide::ClearSearchCB( Fl_Widget*, void* )
{
    s_Search->value( "" );
    FilterTopics();
    s_Search->take_focus();
}

void QaspFieldGuide::NavigateCB( Fl_Widget* widget, void* )
{
    const auto pos = std::find( g_Visible.begin(), g_Visible.end(), s_Current );
    if ( pos == g_Visible.end() ) return;
    const int step = static_cast<int>( pos - g_Visible.begin() ) + ( widget == s_Previous ? -1 : 1 );
    if ( step >= 0 && step < static_cast<int>( g_Visible.size() ) ) LoadTopic( g_Visible[step] );
}

void QaspFieldGuide::CloseCB( Fl_Widget*, void* )
{
    if ( s_Window ) s_Window->hide();
}
