// This file is released under the NASA Open Source Agreement (NOSA) 1.3.
// See LICENSE. Original QASP workspace widgets and vector icons.
#ifndef QASP_WORKSPACE_H
#define QASP_WORKSPACE_H

#include <FL/Fl_Group.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Fl_Button;
class Fl_Scrollbar;
class Fl_Widget_Tracker;
class QaspIconButton;
class QaspTabStrip;
class QaspSidebarDivider;

// A real native child-window clipping viewport. A plain Fl_Scroll cannot clip
// another native window at its edges on every backend. The scrollbars are OUTSIDE
// this viewport, so a wide/tall hosted editor cannot paint over them or the header.
class QaspDockArea : public Fl_Group
{
public:
    QaspDockArea( int x, int y, int w, int h, const char *emptyMessage );
    ~QaspDockArea() override;
    void Host( Fl_Double_Window *window );
    bool Contains( const Fl_Double_Window *window ) const;
    void Synchronize();
    void DetachAll(); // Screens retain ownership of their windows.
    void resize( int x, int y, int w, int h ) override;
    int handle( int event ) override;
    Fl_Double_Window *ActiveWindow() const;
    int HorizontalOffset() const;
    int VerticalOffset() const;
private:
    struct Hosted;
    std::vector<std::unique_ptr<Hosted>> m_Hosted;
    Fl_Double_Window *m_Clip;
    Fl_Box *m_Empty;
    Fl_Scrollbar *m_Horizontal;
    Fl_Scrollbar *m_Vertical;
    Fl_Double_Window *m_Previous = nullptr;
    bool m_LayingOut = false;
    void Layout();
    static void ScrollCB( Fl_Widget *, void * );
};

struct QaspTabLabel
{
    std::string title;
    std::string tooltip;
    bool dirty = false;
};

class QaspWorkspace : public Fl_Group
{
public:
    enum Panel { Components = 0, Properties, Analysis, Learn, PanelCount };
    enum Action
    {
        NewProject = 100, OpenProject, SaveProject, FitModel, TopView, FrontView,
        IsoView, FieldGuide, CloseProject,
        MassProperties = 200, ParasiteDrag, VSPAERO, CFDMesh, Structures
    };
    QaspWorkspace( int x, int y, int w, int h );
    ~QaspWorkspace() override;
    void resize( int x, int y, int w, int h ) override;
    void ShowPanel( Panel panel );
    void TogglePanel( Panel panel );
    void HidePanel();
    void ToggleSidebar();
    void SetSidebarWidth( int width );
    int SidebarWidth() const;
    int CurrentPanel() const { return m_CurrentPanel; }
    Fl_Group *Canvas() const { return m_Canvas; }
    void SetViewport( Fl_Widget *viewport );
    QaspDockArea *ComponentsArea() const { return m_Components; }
    QaspDockArea *PropertiesArea() const { return m_Properties; }
    void Synchronize();
    void SetTabs( const std::vector<QaspTabLabel> &tabs, std::size_t active );
    void SetProjectCaption( const std::string &caption );
    void OnAction( std::function<void( int )> callback ) { m_OnAction = std::move( callback ); }
    void OnSelectTab( std::function<void( std::size_t )> callback );
    void OnCloseTab( std::function<void( std::size_t )> callback );
private:
    Fl_Group *m_Rail;
    QaspIconButton *m_Activity[PanelCount];
    QaspTabStrip *m_Tabs;
    Fl_Group *m_Toolbar;
    std::vector<Fl_Widget *> m_ToolButtons;
    Fl_Box *m_Caption;
    Fl_Box *m_PanelTitle;
    QaspIconButton *m_ClosePanel;
    QaspSidebarDivider *m_Divider;
    QaspDockArea *m_Components;
    QaspDockArea *m_Properties;
    Fl_Group *m_Analysis;
    Fl_Group *m_Learn;
    Fl_Group *m_Canvas;
    Fl_Widget *m_Viewport = nullptr;
    int m_CurrentPanel = -1; // Start with maximum canvas space; icons open panels.
    int m_LastPanel = Components;
    int m_Widths[PanelCount] = { 300, 460, 300, 320 };
    std::function<void( int )> m_OnAction;
    std::unordered_map<Fl_Widget *, int> m_Commands;
    void Layout();
    void Bind( Fl_Widget *widget, int command );
    static void ButtonCB( Fl_Widget *widget, void *data );
};
#endif
