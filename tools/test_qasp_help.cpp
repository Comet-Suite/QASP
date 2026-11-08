// NASA NOSA 1.3; see LICENSE. QASP regression tests, independent of FLTK.
#include "QaspHelp.h"
#include <cstdlib>
#include <iostream>
#include <set>
#include <sstream>
#include <string>

namespace
{
int checks = 0;
void Check( bool pass, const std::string& message )
{
    ++checks;
    if ( !pass )
    {
        std::cerr << "FAIL: " << message << '\n';
        std::exit( 1 );
    }
}
bool Contains( const std::string& text, const std::string& needle )
{
    return text.find( needle ) != std::string::npos;
}
}

int main()
{
    using namespace QaspHelp;
    Check( ParmHelpCount() >= 100, "Substantive authored coverage" );
    Check( LessonCount() >= 20, "Short lessons cover basics and analysis" );
    Check( ParmHelpAt( ParmHelpCount() ) == nullptr, "Tooltip bounds" );
    Check( LessonAt( LessonCount() ) == nullptr, "Lesson bounds" );
    for ( std::size_t i = 0; i < ParmHelpCount(); ++i )
    {
        const ParmHelp& entry = *ParmHelpAt( i );
        Check( FindParmHelp( entry.group, entry.name, entry.descript ) == &entry,
               std::string( "Each authored entry resolves: " ) + entry.group + "/" + entry.name );
        const std::string text = FormatTooltip( entry );
        Check( Contains( text, "Effect:" ) && Contains( text, "Example:" ), "Meaning/effect/example format" );
        Check( text.size() < 1100, "Hover card remains short" );
        std::istringstream lines( text );
        std::string line;
        while ( std::getline( lines, line ) ) Check( line.size() <= 68, "No unbounded hover lines" );
    }
    const ParmHelp* thrust = FindParmHelp( "Rotor", "RotorThrust" );
    Check( thrust && std::string( thrust->title ) == "Thrust", "Actual rotor thrust parameter, not a guessed name" );
    Check( Contains( FormatTooltip( *thrust ), "pushing or pulling" ), "Thrust definition" );
    const std::string air = TooltipForParm( "ParasiteDrag", "Density", "Density" );
    const std::string material = TooltipForParm( "Mass_Props", "Density", "Volumetric density (mass/len^3)" );
    Check( air != material && Contains( air, "air mass" ) && Contains( material, "component" ), "Two densities must differ" );
    const std::string wing = TooltipForParm( "XSec", "Area", "Area of Wing Section" );
    const std::string section = TooltipForParm( "XSecCurve", "Area", "Area of cross section" );
    Check( wing != section && Contains( section, "not wing" ), "Section area is not wing area" );
    Check( FindParmHelp( "WrongGroup", "Area" ) == nullptr, "No unsafe name-only Area matching" );
    Check( FindParmHelp( "Design", "Length" ) == nullptr, "Ambiguous Length needs context" );
    const ParmHelp* fuselage = FindParmHelp( "Design", "Length", "Length of fuselage" );
    const ParmHelp* pod = FindParmHelp( "Design", "Length", "Length of pod" );
    Check( fuselage && pod && fuselage != pod, "Fuselage versus pod length" );
    Check( FindParmHelp( "Design", "Length", "A different kind of length" ) == nullptr, "Description guard" );
    Check( TooltipForParm( "Unknown", "Unknown", "Default Description" ).empty(), "No default placeholder" );
    Check( TooltipForParm( "Unknown", "Unknown", "Deprecated" ).empty(), "No deprecated placeholder" );
    Check( TooltipForParm( "Unknown", "Unknown", "Technical label" ) == "Reference: Technical label", "Honest advanced reference fallback" );
    const ParmHelp* meanChord = FindParmHelp( "Design", "Chord" );
    Check( meanChord && Contains( meanChord->meaning, "radius" ) && Contains( meanChord->units, "Ratio" ), "Propeller chord metric uses c/R, not length" );
    const ParmHelp* fineness = FindParmHelp( "Design", "FineRatio" );
    Check( fineness && Contains( fineness->meaning, "radius, not its diameter" ), "Pod fineness uses actual implementation" );
    const std::string alt = FindParmHelp( "ParasiteDrag", "Alt" )->effect;
    Check( ( Contains( alt, "not a calculated" ) && Contains( alt, "maximum" ) ), "Altitude input is not ceiling" );

    QaspTooltipState state;
    Check( state.Bind( "Rotor", "RotorThrust", "Rotor thrust" ), "First automatic binding" );
    Check( Contains( state.Text(), "pushing or pulling" ), "Correct first binding text" );
    Check( !state.Bind( "Rotor", "RotorThrust", "Rotor thrust" ), "No unnecessary refresh" );
    Check( state.Bind( "Mass_Props", "Density", "Volumetric density" ), "Reused control refreshes" );
    Check( !Contains( state.Text(), "pushing or pulling" ), "No stale thrust text" );
    Check( state.Bind( "ParasiteDrag", "Density", "Density" ), "Same name, different context refreshes" );
    Check( Contains( state.Text(), "air mass" ), "Context follows the new group" );
    Check( state.ClearBinding() && state.Text().empty(), "Unbound control clears stale help" );
    Check( !state.ClearBinding(), "Repeated clear is harmless" );
    Check( state.Bind( "Rotor", "RotorThrust", "Rotor thrust" ), "Rebind after clearing" );
    state.SetExplicit( "Screen-specific action help" );
    Check( !state.Bind( "Rotor", "RotorPower", "Rotor power" ), "Explicit override wins" );
    Check( !state.ClearBinding() && state.Text() == "Screen-specific action help", "Explicit action survives unbound refresh" );
    state.SetExplicit( "" );
    Check( !state.Bind( "Rotor", "RotorPower", "Rotor power" ) && state.Text().empty(), "Explicitly disabling help stays disabled" );
    QaspTooltipState changedDescription;
    changedDescription.Bind( "Design", "Length", "Length of fuselage" );
    Check( changedDescription.Bind( "Design", "Length", "Length of pod" ), "Changed description on same key refreshes" );
    Check( Contains( changedDescription.Text(), "pod" ), "New guarded description used" );

    Check( SearchLessons( "" ).size() == LessonCount(), "Empty search shows all" );
    Check( SearchLessons( " \t\n " ).size() == LessonCount(), "Whitespace search shows all" );
    Check( SearchLessons( "zzzz-no-lesson-zzzz" ).empty(), "No-match search" );
    Check( SearchLessons( "THRUST" ) == SearchLessons( "thrust" ), "Case-insensitive search" );
    Check( !SearchLessons( "wing area" ).empty(), "Multiple-word search" );
    Check( FindLesson( "thrust" ) >= 0 && FindLesson( "nonexistent" ) == -1, "Stable lesson IDs" );
    const int altitude = FindLesson( "altitude" );
    Check( altitude >= 0 && Contains( LessonAt( altitude )->try_it, "no validated automatic ceiling" ), "No fabricated flight verdict" );
    std::set<std::string> ids;
    for ( std::size_t i = 0; i < LessonCount(); ++i )
    {
        const Lesson& lesson = *LessonAt( i );
        Check( ids.insert( lesson.id ).second, "Unique lesson ID" );
        const std::string html = LessonHtml( i );
        Check( Contains( html, "WHAT IT MEANS" ) && Contains( html, "TRY IN QASP" ) && Contains( html, "KEEP IN MIND" ), "Structured lesson sections" );
        Check( !Contains( html, "http:" ) && !Contains( html, "https:" ) && !Contains( html, "<img" ), "Guide is completely offline" );
    }
    std::cout << "PASS: " << checks << " help checks; " << ParmHelpCount()
              << " authored parameter explanations; " << LessonCount() << " short lessons\n";
    return 0;
}
