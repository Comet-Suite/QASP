//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
// QASP addition: reviewed beginner help, separate from the engineering model.
// This module has no FLTK or geometry dependencies so its matching, content,
// search and rebinding behaviour can be tested without launching the full app.
//
#ifndef QASP_HELP_H
#define QASP_HELP_H

#include <cstddef>
#include <string>
#include <vector>

namespace QaspHelp
{

enum class Diagram
{
    Forces, Thrust, Angle, Drag, Wing, Taper, Sweep, Dihedral, Airfoil,
    Cg, Units, Axes, Body, Tail, Measure, Results, Altitude, Mesh, Workflow
};

struct ParmHelp
{
    const char* group;
    const char* name;
    const char* descript;  // Optional exact-description guard for ambiguous keys.
    const char* title;
    const char* meaning;
    const char* effect;
    const char* example;
    const char* units;
};

struct Lesson
{
    const char* id;
    const char* category;
    const char* title;
    const char* summary;
    Diagram diagram;
    const char* meaning;
    const char* effect;
    const char* example;
    const char* try_it;
    const char* caution;
    const char* keywords;
};

std::size_t ParmHelpCount();
const ParmHelp* ParmHelpAt( std::size_t index );
const ParmHelp* FindParmHelp( const std::string& group, const std::string& name,
                             const std::string& descript = std::string() );
std::string FormatTooltip( const ParmHelp& help );
std::string TooltipForParm( const std::string& group, const std::string& name,
                            const std::string& descript );

std::size_t LessonCount();
const Lesson* LessonAt( std::size_t index );
int FindLesson( const std::string& id );
std::vector<int> SearchLessons( const std::string& query );
std::string LessonHtml( std::size_t index );

} // namespace QaspHelp

// Stores one device's help. Explicit screen-authored text wins. Automatic help
// follows the current group/name/description rather than the first Parm ID.
// A reused control must not retain an explanation from its previous role.
class QaspTooltipState
{
public:
    QaspTooltipState() : m_Explicit( false ) {}

    void SetExplicit( const std::string& text );
    bool Bind( const std::string& group, const std::string& name,
               const std::string& descript );
    bool ClearBinding();
    const std::string& Text() const { return m_Text; }

private:
    bool m_Explicit;
    std::string m_Key;
    std::string m_Text;
};

#endif
