// NOSA 1.3. Native FLTK component tests; NOT a complete QASP runtime test.
#include "QaspWorkspace.h"
#include <FL/Fl.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/Fl_Widget.H>
#include <iostream>
#include <stdexcept>
#include <string>

static int checks = 0;
static void Check( bool value, const char *message )
{
    ++checks;
    if ( !value ) { throw std::runtime_error( message ); }
}
static Fl_Widget *FindTooltip( Fl_Group *group, const std::string &prefix )
{
    for ( int i = 0; i < group->children(); ++i )
    {
        Fl_Widget *w = group->child( i );
        if ( w->tooltip() && std::string( w->tooltip() ).rfind( prefix, 0 ) == 0 ) { return w; }
        if ( w->as_group() ) { if ( auto *found = FindTooltip( w->as_group(), prefix ) ) { return found; } }
    }
    return nullptr;
}
static void Pump() { Fl::check(); Fl::flush(); Fl::check(); }

int main()
{
    try
    {
        Fl_Double_Window root( 1200, 760, "QASP workspace native component tests (not the full application)" );
        root.begin();
        auto *workspace = new QaspWorkspace( 0, 26, 1200, 716 );
        root.end(); root.resizable( workspace );
        root.show(); Pump();
        auto *tree = new Fl_Double_Window( 275, 671, "Native tree test window" ); tree->end();
        auto *properties = new Fl_Double_Window( 460, 700, "Native editor test window" ); properties->end();
        Fl_Widget_Tracker treeTracker( tree ), propertyTracker( properties );
        workspace->ComponentsArea()->Host( tree );
        workspace->PropertiesArea()->Host( properties );
        tree->show(); workspace->Synchronize(); Pump();
        Check( workspace->CurrentPanel() == -1, "Sidebar starts closed for more drawing room" );
        Check( workspace->Canvas()->x() == 48 && workspace->Canvas()->w() == 1152, "Full available width on startup" );
        Check( workspace->Canvas()->y() == 90 && workspace->Canvas()->h() == 652, "Compact toolbar gives the canvas eight extra vertical pixels" );
        Check( !tree->visible_r(), "A hosted screen cannot force the initial sidebar open" );
        auto *openTree = FindTooltip( workspace, "Components:" );
        Check( openTree != nullptr, "Components remain one icon click away" );
        openTree->do_callback(); Pump();
        Check( workspace->CurrentPanel() == QaspWorkspace::Components, "Components open on demand" );
        Check( workspace->ComponentsArea()->Contains( tree ), "Tree hosted" );
        Check( tree->parent() && tree->parent()->as_window(), "Tree lives in a real native clipping window" );
        Check( tree->shown() && tree->visible_r(), "Tree actually visible inside main window" );
        Check( tree->window() != &root, "Child window is not a second floating top-level window" );
        Check( tree->parent()->parent() == workspace->ComponentsArea() && tree->parent()->window() == &root,
               "Clipping window itself belongs to the workspace" );
        Check( workspace->Canvas()->x() == 353 && workspace->Canvas()->y() == 90, "Viewport accounts for rail, sidebar and tabs" );
        Check( workspace->Canvas()->w() == 847, "Viewport uses remaining width" );
        Fl_Widget *componentButton = FindTooltip( workspace, "Components:" );
        Check( componentButton != nullptr, "Components icon has descriptive tooltip" );
        componentButton->do_callback(); Pump();
        Check( workspace->CurrentPanel() == -1, "Click same activity icon hides panel" );
        Check( !tree->visible_r(), "Native child hidden with its ancestor panel" );
        Check( workspace->Canvas()->x() == 48 && workspace->Canvas()->w() == 1152, "Canvas reclaims sidebar space" );
        componentButton->do_callback(); Pump();
        Check( workspace->CurrentPanel() == QaspWorkspace::Components && tree->visible_r(), "Click again reopens tree" );

        // Model screens may call Show while their sidebar is closed. This must
        // not create a floating editor or force the sidebar open.
        properties->show(); workspace->Synchronize(); Pump();
        Check( properties->visible(), "Hidden-panel editor retains its own logical visibility" );
        Check( !properties->visible_r(), "Editor stays behind its hidden panel" );
        Check( workspace->CurrentPanel() == QaspWorkspace::Components, "Selection does not steal the tree sidebar" );
        Fl_Widget *propertyButton = FindTooltip( workspace, "Properties:" );
        Check( propertyButton != nullptr, "Properties icon discoverable" );
        propertyButton->do_callback(); workspace->Synchronize(); Pump();
        Check( properties->shown() && properties->visible_r(), "Properties become truly visible on opening panel" );
        Check( !tree->visible_r(), "Only the selected sidebar is visible" );
        Check( workspace->PropertiesArea()->ActiveWindow() == properties, "Logical active editor tracked" );
        workspace->SetSidebarWidth( 600 ); Pump();
        Check( workspace->SidebarWidth() == 600, "Sidebar expands" );
        Check( workspace->Canvas()->w() >= 320, "Expansion preserves minimum useful canvas width" );
        workspace->SetSidebarWidth( 270 ); Pump();
        Check( workspace->SidebarWidth() == 270, "Sidebar shrinks" );
        Check( properties->w() >= 460, "Narrow sidebar does not crush original editor layout" );
        Check( properties->parent()->w() < properties->w(), "Wide editor clipped by native viewport" );
        Fl_Scrollbar *horizontal = nullptr, *vertical = nullptr;
        for ( int i = 0; i < workspace->PropertiesArea()->children(); ++i )
        {
            auto *scrollbar = dynamic_cast<Fl_Scrollbar *>( workspace->PropertiesArea()->child( i ) );
            if ( scrollbar )
            {
                if ( scrollbar->type() == FL_HORIZONTAL ) { horizontal = scrollbar; }
                else { vertical = scrollbar; }
            }
        }
        Check( horizontal && horizontal->visible(), "Horizontal scrolling available for a wide editor" );
        Check( vertical && vertical->visible(), "Vertical scrolling available for a tall editor" );
        Check( horizontal->parent() != properties->parent(), "Native editor cannot cover its scrollbars" );
        horizontal->value( 100 ); horizontal->do_callback(); Pump();
        Check( workspace->PropertiesArea()->HorizontalOffset() == 100 && properties->x() == -100,
               "Horizontal scrollbar moves native content" );
        vertical->value( 80 ); vertical->do_callback(); Pump();
        Check( properties->y() == -80, "Vertical scrollbar moves native content" );
        workspace->SetSidebarWidth( 9999 ); Pump();
        Check( workspace->Canvas()->w() == 320, "Divider maximum clamped" );
        workspace->SetSidebarWidth( -99 ); Pump();
        Check( workspace->SidebarWidth() == 260, "Divider minimum clamped" );
        workspace->ShowPanel( QaspWorkspace::Components ); Pump();
        Check( workspace->SidebarWidth() == 300, "Each sidebar remembers its own width" );

        for ( int width : { 820, 1000, 1400, 640 } )
        {
            for ( int height : { 480, 700, 900 } )
            {
                root.resize( 0, 0, width, height );
                workspace->resize( 0, 26, width, height - 44 ); Pump();
                Check( workspace->Canvas()->w() >= 320, "Canvas remains usable at test window size" );
                Check( workspace->Canvas()->x() + workspace->Canvas()->w() == width, "No lost right-edge space" );
                Check( tree->parent()->x() >= 48, "Native panel stays clear of activity rail" );
            }
        }
        int command = 0;
        workspace->OnAction( [&]( int value ) { command = value; } );
        auto *save = FindTooltip( workspace, "Save active" );
        Check( save != nullptr, "Save toolbar icon exists" ); save->do_callback();
        Check( command == QaspWorkspace::SaveProject, "Save icon routes to host callback" );
        auto *analysis = FindTooltip( workspace, "Analysis:" ); analysis->do_callback(); Pump();
        Check( workspace->CurrentPanel() == QaspWorkspace::Analysis, "Analysis icon opens a real sidebar" );
        auto *learn = FindTooltip( workspace, "Learn:" ); learn->do_callback(); Pump();
        Check( workspace->CurrentPanel() == QaspWorkspace::Learn, "Learn icon opens a real sidebar" );
        auto *hide = FindTooltip( workspace, "Hide this panel" ); hide->do_callback(); Pump();
        Check( workspace->CurrentPanel() == -1, "Panel header close control works" );
        workspace->ToggleSidebar(); Pump();
        Check( workspace->CurrentPanel() == QaspWorkspace::Learn, "Keyboard toggle restores the last used panel, not a permanent tree" );
        workspace->ToggleSidebar(); Pump();
        Check( workspace->CurrentPanel() == -1, "Keyboard toggle hides an open panel" );
        workspace->ShowPanel( QaspWorkspace::Properties ); Pump();
        int rememberedWidth = workspace->SidebarWidth();
        workspace->ToggleSidebar(); workspace->ToggleSidebar(); Pump();
        Check( workspace->CurrentPanel() == QaspWorkspace::Properties && workspace->SidebarWidth() == rememberedWidth,
               "Hide/show keeps the chosen panel and its width" );
        workspace->SetTabs( { { "A.vsp3", "/models/A.vsp3", true }, { "B.vsp3", "/models/B.vsp3", false } }, 0 );
        workspace->SetTabs( { { "A.vsp3", "/models/A.vsp3", true }, { "B.vsp3", "/models/B.vsp3", false } }, 1 ); Pump();
        Check( !propertyTracker.deleted() && !treeTracker.deleted(), "Hiding/switching panels never deletes owned screens" );
        // ScreenMgr destroys many screen owners before MainVSPScreen. Trackers
        // must tolerate that order, rather than leaving a double-delete on exit.
        delete tree; Pump();
        Check( treeTracker.deleted(), "Screen owner can delete a hosted child first" );
        workspace->Synchronize();
        workspace->PropertiesArea()->DetachAll();
        Check( !propertyTracker.deleted() && !properties->parent(), "Detaching preserves original screen ownership" );
        delete properties;
        root.hide();
        std::cout << checks << " native FLTK workspace checks passed. Not a full QASP/Windows runtime test.\n";
        return 0;
    }
    catch ( const std::exception &e )
    {
        std::cerr << "FAILED after " << checks << " checks: " << e.what() << '\n'; return 1;
    }
}
