//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
// QASP addition: context-aware explanations, short lessons and tested rebinding.
//
#include "QaspHelp.h"

#include <algorithm>
#include <cctype>
#include <sstream>

#include "QaspHelpData.inc"

namespace
{
std::string Lower( std::string text )
{
    std::transform( text.begin(), text.end(), text.begin(), []( unsigned char c )
    {
        return static_cast<char>( std::tolower( c ) );
    } );
    return text;
}

std::string Wrap( const std::string& text )
{
    std::istringstream words( text );
    std::string word, out;
    std::size_t column = 0;
    while ( words >> word )
    {
        if ( column && column + 1 + word.size() > 68 )
        {
            out += '\n';
            column = 0;
        }
        else if ( column )
        {
            out += ' ';
            ++column;
        }
        out += word;
        column += word.size();
    }
    return out;
}

std::string EscapeHtml( const char* text )
{
    std::string out;
    for ( const char* p = text; p && *p; ++p )
    {
        switch ( *p )
        {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            default: out += *p; break;
        }
    }
    return out;
}
}

std::size_t QaspHelp::ParmHelpCount()
{
    return sizeof( g_ParmHelp ) / sizeof( g_ParmHelp[0] );
}

const QaspHelp::ParmHelp* QaspHelp::ParmHelpAt( std::size_t index )
{
    return index < ParmHelpCount() ? &g_ParmHelp[index] : nullptr;
}

const QaspHelp::ParmHelp* QaspHelp::FindParmHelp( const std::string& group,
                                               const std::string& name,
                                               const std::string& descript )
{
    const ParmHelp* generic = nullptr;
    for ( const ParmHelp& entry : g_ParmHelp )
    {
        if ( group == entry.group && name == entry.name )
        {
            if ( entry.descript[0] )
            {
                if ( descript == entry.descript )
                {
                    return &entry;
                }
            }
            else
            {
                generic = &entry;
            }
        }
    }
    return generic;
}

std::string QaspHelp::FormatTooltip( const ParmHelp& help )
{
    std::string out = help.title;
    out += "\n" + Wrap( help.meaning );
    out += "\n\n" + Wrap( std::string( "Effect: " ) + help.effect );
    out += "\n\n" + Wrap( std::string( "Example: " ) + help.example );
    if ( help.units[0] )
    {
        out += "\n\n" + Wrap( std::string( "Units: " ) + help.units );
    }
    return out;
}

std::string QaspHelp::TooltipForParm( const std::string& group, const std::string& name,
                                    const std::string& descript )
{
    const ParmHelp* help = FindParmHelp( group, name, descript );
    if ( help )
    {
        return FormatTooltip( *help );
    }

    // Preserve advanced reference text rather than invent a physical effect.
    // It is deliberately labelled, and is not counted as a beginner explanation.
    if ( !descript.empty() && descript != "Default Description" && descript != "Deprecated" )
    {
        return Wrap( "Reference: " + descript );
    }
    return std::string();
}

std::size_t QaspHelp::LessonCount()
{
    return sizeof( g_Lessons ) / sizeof( g_Lessons[0] );
}

const QaspHelp::Lesson* QaspHelp::LessonAt( std::size_t index )
{
    return index < LessonCount() ? &g_Lessons[index] : nullptr;
}

int QaspHelp::FindLesson( const std::string& id )
{
    for ( std::size_t i = 0; i < LessonCount(); ++i )
    {
        if ( id == g_Lessons[i].id )
        {
            return static_cast<int>( i );
        }
    }
    return -1;
}

std::vector<int> QaspHelp::SearchLessons( const std::string& query )
{
    std::istringstream input( Lower( query ) );
    std::vector<std::string> needles;
    std::string word;
    while ( input >> word )
    {
        needles.push_back( word );
    }
    std::vector<int> result;
    for ( std::size_t i = 0; i < LessonCount(); ++i )
    {
        const Lesson& l = g_Lessons[i];
        std::string haystack = Lower( std::string( l.title ) + ' ' + l.summary + ' ' + l.meaning +
            ' ' + l.effect + ' ' + l.example + ' ' + l.try_it + ' ' + l.caution + ' ' + l.keywords );
        bool match = true;
        for ( const std::string& needle : needles )
        {
            if ( haystack.find( needle ) == std::string::npos )
            {
                match = false;
                break;
            }
        }
        if ( match )
        {
            result.push_back( static_cast<int>( i ) );
        }
    }
    return result;
}

std::string QaspHelp::LessonHtml( std::size_t index )
{
    const Lesson* l = LessonAt( index );
    if ( !l )
    {
        return "<html><body><p>No lesson selected.</p></body></html>";
    }
    std::string html = "<html><body bgcolor=\"#ffffff\" text=\"#253248\">";
    const char* labels[] = { "WHAT IT MEANS", "HOW IT AFFECTS YOUR AIRCRAFT", "EXAMPLE", "TRY IN QASP", "KEEP IN MIND" };
    const char* values[] = { l->meaning, l->effect, l->example, l->try_it, l->caution };
    for ( int i = 0; i < 5; ++i )
    {
        html += "<p><font color=\"";
        html += ( i == 4 ? "#8a5210" : "#0069ac" );
        html += "\"><b>" + std::string( labels[i] ) + "</b></font><br>";
        html += EscapeHtml( values[i] );
        html += "</p>";
    }
    html += "</body></html>";
    return html;
}

void QaspTooltipState::SetExplicit( const std::string& text )
{
    m_Explicit = true;
    m_Text = text;
}

bool QaspTooltipState::Bind( const std::string& group, const std::string& name,
                             const std::string& descript )
{
    if ( m_Explicit )
    {
        return false;
    }
    const std::string key = group + '\n' + name + '\n' + descript;
    if ( key == m_Key )
    {
        return false;
    }
    m_Key = key;
    const std::string next = QaspHelp::TooltipForParm( group, name, descript );
    if ( next == m_Text )
    {
        return false;
    }
    m_Text = next;
    return true;
}

bool QaspTooltipState::ClearBinding()
{
    if ( m_Explicit )
    {
        return false;
    }
    m_Key.clear();
    const bool changed = !m_Text.empty();
    m_Text.clear();
    return changed;
}
