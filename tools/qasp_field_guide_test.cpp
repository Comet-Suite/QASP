// NASA NOSA 1.3; see LICENSE.
// Runs the ACTUAL QaspFieldGuide component, not a web mock-up.
// --self-test exercises search, navigation, redraw, resize, hide and reopen.
// --lesson ID opens one lesson in the actual native component.
// This is not a full QASP geometry-engine or Windows launch test.
#include "QaspFieldGuide.h"
#include "QaspHelp.h"
#include "QaspTheme.h"
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Help_View.H>
#include <FL/Fl_Button.H>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace
{
int checks = 0;
void Check( bool pass, const char* message )
{
    ++checks;
    if ( !pass )
    {
        std::cerr << "FAIL: " << message << '\n';
        std::exit( 1 );
    }
}
void Collect( Fl_Group* group, std::vector<Fl_Widget*>& result )
{
    for ( int i = 0; i < group->children(); ++i )
    {
        Fl_Widget* widget = group->child( i );
        result.push_back( widget );
        Fl_Group* nested = dynamic_cast<Fl_Group*>( widget );
        if ( nested ) Collect( nested, result );
    }
}
void Paint()
{
    Fl::wait( 0.03 );
    Fl::flush();
    Fl::check();
}

}

int main( int argc, char** argv )
{
    bool selfTest = false;
    const char* lessonId = nullptr;
    for ( int i = 1; i < argc; ++i )
    {
        if ( !std::strcmp( argv[i], "--self-test" ) ) selfTest = true;
        else if ( !std::strcmp( argv[i], "--lesson" ) && i + 1 < argc ) lessonId = argv[++i];
    }
    QaspTheme::Apply();
    QaspFieldGuide::Show( lessonId );
    Paint();
    Fl_Window* window = Fl::first_window();
    Check( window && window->shown(), "Help component is visible" );
    Check( window->label() && std::string( window->label() ) == "QASP Field Guide", "Actual guide window" );
    if ( !selfTest ) return Fl::run();

    std::vector<Fl_Widget*> widgets;
    Collect( window, widgets );
    Fl_Input* search = nullptr;
    Fl_Hold_Browser* topics = nullptr;
    Fl_Help_View* body = nullptr;
    Fl_Button* clear = nullptr;
    Fl_Button* previous = nullptr;
    Fl_Button* next = nullptr;
    Fl_Button* close = nullptr;
    Fl_Widget* brand = nullptr;
    Fl_Widget* heading = nullptr;
    for ( Fl_Widget* widget : widgets )
    {
        const std::string widgetLabel = widget->label() ? widget->label() : "";
        if ( widgetLabel == "QASP" ) brand = widget;
        if ( widgetLabel == "Field Guide" ) heading = widget;
        if ( auto value = dynamic_cast<Fl_Input*>( widget ) ) search = value;
        if ( auto value = dynamic_cast<Fl_Hold_Browser*>( widget ) ) topics = value;
        if ( auto value = dynamic_cast<Fl_Help_View*>( widget ) ) body = value;
        if ( auto value = dynamic_cast<Fl_Button*>( widget ) )
        {
            const std::string label = value->label() ? value->label() : "";
            if ( label == "Clear" ) clear = value;
            if ( label == "< Previous" ) previous = value;
            if ( label == "Next >" ) next = value;
            if ( label == "Close" ) close = value;
        }
    }
    Check( search && topics && body && clear && previous && next && close, "Find actual interactive controls" );
    Check( brand && heading, "Find fixed header anchors" );
    const int brandX = brand->x(), brandWidth = brand->w(), headingX = heading->x();
    Check( topics->size() == static_cast<int>( QaspHelp::LessonCount() ), "All lessons are listed" );
    Check( !previous->active() && next->active(), "First lesson navigation state" );
    const std::string first = body->value();
    next->do_callback(); Paint();
    Check( std::string( body->value() ) != first, "Next loads a different lesson" );
    previous->do_callback(); Paint();
    Check( std::string( body->value() ) == first, "Previous returns to the first lesson" );
    search->value( "zzzz-no-lesson-zzzz" ); search->do_callback(); Paint();
    Check( topics->size() == 0, "No matches empties the topic list" );
    Check( !previous->active() && !next->active(), "No-match navigation disabled" );
    Check( std::string( body->value() ).find( "Try" ) != std::string::npos, "No-match body replaces old article" );
    clear->do_callback(); Paint();
    Check( topics->size() == static_cast<int>( QaspHelp::LessonCount() ), "Clear restores all lessons" );
    search->value( "WiNg ArEa" ); search->do_callback(); Paint();
    Check( topics->size() > 0 && topics->size() < static_cast<int>( QaspHelp::LessonCount() ), "Live multi-word case-insensitive filtering" );
    clear->do_callback(); Paint();
    for ( std::size_t i = 0; i < QaspHelp::LessonCount(); ++i )
    {
        const QaspHelp::Lesson* lesson = QaspHelp::LessonAt( i );
        QaspFieldGuide::Show( lesson->id ); Paint();
        Check( body->value() && std::string( body->value() ) == QaspHelp::LessonHtml( i ), "Every actual lesson loads and renders" );
        Check( previous->active() == ( i > 0 ), "Previous bound for each lesson" );
        Check( next->active() == ( i + 1 < QaspHelp::LessonCount() ), "Next bound for each lesson" );
    }
    window->size( 820, 600 ); Paint();
    Check( brand->x() == brandX && brand->w() == brandWidth && heading->x() == headingX,
           "Narrowing the window does not compress or overlap the header branding" );
    Check( body->w() >= 500 && body->h() >= 150, "Minimum-size article remains usable" );
    Check( next->x() + next->w() <= window->w() && next->y() + next->h() <= window->h(), "Minimum-size navigation remains inside window" );
    window->size( 1240, 850 ); Paint();
    Check( body->w() >= 850 && body->h() >= 400, "Article expands with window" );
    close->do_callback(); Paint();
    Check( !window->shown(), "Close hides the guide without closing the application" );
    QaspFieldGuide::Show(); Paint();
    Check( window->shown(), "Guide reopens" );
    std::cout << "PASS: " << checks << " actual FLTK guide checks, all "
              << QaspHelp::LessonCount() << " lesson drawings rendered\n";
    return 0;
}
