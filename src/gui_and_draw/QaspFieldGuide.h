//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
// QASP addition. Revised after user testing: short illustrated lessons instead
// of long, unformatted manual pages. All content is embedded and works offline.
//
#ifndef QASP_FIELD_GUIDE_H
#define QASP_FIELD_GUIDE_H

class Fl_Window;
class Fl_Hold_Browser;
class Fl_Help_View;
class Fl_Input;
class Fl_Widget;
class Fl_Box;
class Fl_Button;
class QaspLessonDiagram;

class QaspFieldGuide
{
public:
    // Existing Help menu callers use Show(); component tests can select a lesson.
    static void Show( const char* lesson_id = nullptr );

private:
    static void BuildWindow();
    static void LoadTopic( int index );
    static void FilterTopics();
    static void UpdateNavigation();
    static void TopicCB( Fl_Widget*, void* );
    static void CloseCB( Fl_Widget*, void* );
    static void SearchCB( Fl_Widget*, void* );
    static void ClearSearchCB( Fl_Widget*, void* );
    static void NavigateCB( Fl_Widget*, void* );

    static Fl_Window* s_Window;
    static Fl_Hold_Browser* s_Topics;
    static Fl_Help_View* s_Body;
    static Fl_Input* s_Search;
    static Fl_Box* s_Title;
    static Fl_Box* s_Category;
    static Fl_Box* s_Summary;
    static Fl_Box* s_Count;
    static Fl_Box* s_Progress;
    static Fl_Button* s_Previous;
    static Fl_Button* s_Next;
    static QaspLessonDiagram* s_Diagram;
    static int s_Current;
};

#endif
