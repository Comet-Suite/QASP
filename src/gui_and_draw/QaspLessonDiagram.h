// NASA Open Source Agreement (NOSA) version 1.3; see LICENSE.
// QASP addition: small explanatory drawings, not screenshots or solver output.
#ifndef QASP_LESSON_DIAGRAM_H
#define QASP_LESSON_DIAGRAM_H

#include <FL/Fl_Widget.H>
#include "QaspHelp.h"

class QaspLessonDiagram : public Fl_Widget
{
public:
    QaspLessonDiagram( int x, int y, int w, int h );
    void SetDiagram( QaspHelp::Diagram diagram );
    void Clear();
    void draw() override;

private:
    QaspHelp::Diagram m_Diagram;
    bool m_Empty;
};

#endif
