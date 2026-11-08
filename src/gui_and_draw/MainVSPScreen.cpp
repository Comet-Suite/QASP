//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//

//
// Programmatic main GUI window
// Rob McDonald
//
//////////////////////////////////////////////////////////////////////

#ifdef WIN32
#undef _HAS_STD_BYTE
#define _HAS_STD_BYTE 0
#endif

#include <cmath>
#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <locale>
#include <sstream>
#include <FL/Fl_Terminal.H>
#include <FL/Fl_Text_Editor.H>

#include "main.h"
#include "MainVSPScreen.h"
#include "QaspFieldGuide.h"
#include "QaspWorkspace.h"
#include "QaspProjectSession.h"
#include "QaspVehicleProjectHost.h"
#include "MainGLWindow.h"
#include "ScreenMgr.h"
#include "GraphicEngine.h"
#include "Display.h"
#include "Camera.h"
#include "LayoutMgr.h"
#include "Viewport.h"
#include "Background.h"
#include <FL/fl_ask.H>
#include "CloneDeleteDialog.h"
#include "ManageCORScreen.h"
#include "ManageGeomScreen.h"
#include "ManageViewScreen.h"
#include "UnitConversion.h"
#include "FileUtil.h"

#include "CfdMeshScreen.h"
#include "GeometryAnalysisScreen.h"
#include "ParmMgr.h"
#include "SurfaceIntersectionScreen.h"
#include "StructScreen.h"
#include "StructAssemblyScreen.h"

using namespace vsp;

using namespace vsp;

namespace { std::string QaspModelNames( Vehicle *vehicle ); }

//==== Constructor ====//
MainVSPScreen::MainVSPScreen( ScreenMgr* mgr ) : ActionScreen( mgr )
{
    m_FLTK_Window = nullptr;
    m_GlWin = nullptr;
    m_DumpName = "Main";

    m_ShowXYZArrow = true;
    m_ShowBorder = true;

    int x, y, w, h;
    w = 500;
    h = 500;

    //==== Window ====//
    m_FLTK_Window = new VSP_Window( w, h );
    m_FLTK_Window->resizable( m_FLTK_Window );
    VspScreen::SetFlWindow( m_FLTK_Window );

    // Lay the windows out on the work area of the screen under the mouse, which leaves out the taskbar
    // or menu bar.  Before it is shown, a window's coordinates are taken in the scale of its assigned
    // screen, so the main window is assigned to this one.
    int mx, my;
    Fl::get_mouse( mx, my );
    int screen = Fl::screen_num( mx, my );
    Fl::screen_work_area( x, y, w, h, screen );
    m_FLTK_Window->screen_num( screen );

    // QASP uses one workspace instead of reserving desktop space for floating
    // tree/editor windows. Panels share the main window and can be hidden.
    int frame = 16;
    int w_allow = std::max( 640, w - 2 * frame );
    int h_allow = std::max( 420, static_cast<int>( 0.9 * h ) );
    m_FLTK_Window->resize( x + frame, y + 20, w_allow, h_allow );
    m_FLTK_Window->size_range( 640, 420 );

    m_FLTK_Window->callback( staticCloseCB, this );

    Fl_Group * grp = new Fl_Group( 0, 0, w_allow, h_allow);
    m_FLTK_Window->add( grp );
    m_GenLayout.SetGroupAndScreen( grp, this );

    m_MenuBar = m_GenLayout.AddFlMenuBar( 26 );

    m_NewMenuItem.Init( this, m_MenuBar, "File/New Project", FL_COMMAND + 'n' );
    m_OpenMenuItem.Init( this, m_MenuBar, "File/Open Project...", FL_COMMAND + 'o' );
    m_CloseProjectMenuItem.Init( this, m_MenuBar, "File/Close Project", FL_COMMAND + 'w' );
    m_SaveMenuItem.Init( this, m_MenuBar, "File/Save...", FL_COMMAND + 's' );
    m_SaveAsMenuItem.Init( this, m_MenuBar, "File/Save As...", FL_COMMAND + FL_SHIFT + 's' );
    m_SaveSetMenuItem.Init( this, m_MenuBar, "File/Save Set..." );
    m_InsertMenuItem.Init( this, m_MenuBar, "File/Insert..." );
    m_ImportMenuItem.Init( mgr, m_MenuBar, "File/Import...", vsp::VSP_IMPORT_SCREEN );
    m_ExportMenuItem.Init( mgr, m_MenuBar, "File/Export...", vsp::VSP_EXPORT_SCREEN );
    // m_SetTempDirMenuItem.Init( this, m_MenuBar, "File/Set Temp Dir..." );
    m_RunScriptMenuItem.Init( this, m_MenuBar, "File/Run Script..." );
    m_PreferencesMenuItem.Init( mgr, m_MenuBar, "File/Preferences...", vsp::VSP_PREFERENCES_SCREEN );
    m_ExitMenuItem.Init( this, m_MenuBar, "File/Exit" );
    m_StopGUIMenuItem.Init( this, m_MenuBar, "File/Stop GUI" );
    m_StopGUIMenuItem.Hide();

    m_UndoMenuItem.Init( this, m_MenuBar, "Edit/Undo Parameter Change", FL_COMMAND + 'z' );
    m_CutMenuItem.Init( this, m_MenuBar, "Edit/Cut", FL_COMMAND + 'x' );
    m_CopyMenuItem.Init( this, m_MenuBar, "Edit/Copy", FL_COMMAND + 'c' );
    m_PasteMenuItem.Init( this, m_MenuBar, "Edit/Paste", FL_COMMAND + 'v' );
    m_DeleteMenuItem.Init( this, m_MenuBar, "Edit/Delete" );
    m_SelAllMenuItem.Init( this, m_MenuBar, "Edit/Select All", FL_COMMAND + 'a' );
    m_PickMenuItem.Init( this, m_MenuBar, "Edit/Toggle Pick Mode", 'p' );

    m_OneMenuItem.Init( this, m_MenuBar, "Window/One" );
    m_FourMenuItem.Init( this, m_MenuBar, "Window/Four" );
    m_TwoHMenuItem.Init( this, m_MenuBar, "Window/Two Horizontal" );
    m_TwoVMenuItem.Init( this, m_MenuBar, "Window/Two Vertical" );
    m_BackgroundMenuItem.Init( mgr, m_MenuBar, "Window/Background...", vsp::VSP_BACKGROUND_SCREEN );
    m_Background3DMenuItem.Init( mgr, m_MenuBar, "Window/3D Background...", vsp::VSP_BACKGROUND3D_SCREEN );
    m_ScreenshotMenuItem.Init( mgr, m_MenuBar, "Window/Screenshot...", vsp::VSP_SCREENSHOT_SCREEN );
    m_VectorScreenshotMenuItem.Init( mgr, m_MenuBar, "Window/Vector Screenshot...", vsp::VSP_VECTOR_SCREENSHOT_SCREEN );
    m_AxisMenuItem.Init( this, m_MenuBar, "Window/Axis Toggle" );
    m_AxisMenuItem.Update( m_ShowXYZArrow );
    m_BorderMenuItem.Init( this, m_MenuBar, "Window/Border Toggle" );
    m_BorderMenuItem.Update( m_ShowBorder );

    m_TopMenuItem.Init( this, m_MenuBar, "View/Top", FL_F + 5 );
    m_FrontMenuItem.Init( this, m_MenuBar, "View/Front", FL_F + 6 );
    m_LeftSideMenuItem.Init( this, m_MenuBar, "View/Left", FL_F + 7 );
    m_LeftIsoMenuItem.Init( this, m_MenuBar, "View/Left Iso", FL_F + 8 );
    m_BottomMenuItem.Init( this, m_MenuBar, "View/Bottom", FL_F + 9 );
    m_RearMenuItem.Init( this, m_MenuBar, "View/Rear", FL_F + 10 );
    m_RightSideMenuItem.Init( this, m_MenuBar, "View/Right", FL_F + 11 );
    m_RightIsoMenuItem.Init( this, m_MenuBar, "View/Right Iso", FL_F + 12 );
    m_SetCORMenuItem.Init( this, m_MenuBar, "View/Set Rotation Center", 'r' );
    m_SetVNMenuItem.Init( this, m_MenuBar, "View/View Normal To", 'n' );
    m_AlignGeomMenuItem.Init( this, m_MenuBar, "View/View Front Of", 'x' );
    m_CenterMenuItem.Init( this, m_MenuBar, "View/Center", 'c' );
    m_CenterAllMenuItem.Init( this, m_MenuBar, "View/Center All", 'C' );
    m_FitViewMenuItem.Init( this, m_MenuBar, "View/Fit On Screen", 'f' );
    m_FitAllViewMenuItem.Init( this, m_MenuBar, "View/Fit All On Screen", 'F' );
    m_ResetAllViewMenuItem.Init( this, m_MenuBar, "View/Reset All", 'R' );
    m_AdjustMenuItem.Init( mgr, m_MenuBar, "View/Adjust...", vsp::VSP_VIEW_SCREEN );
    // m_AntialiasMenuItem.Init( this, m_MenuBar, "View/New" );

    m_GeomMenuItem.Init( mgr, m_MenuBar, "Model/Geometry...", vsp::VSP_MANAGE_GEOM_SCREEN, vsp::VSP_NERF_MANAGE_GEOM_SCREEN );
    m_SetEditMenuItem.Init( mgr, m_MenuBar, "Model/Set Editor...", vsp::VSP_SET_EDITOR_SCREEN );
    m_VarPresetMenuItem.Init( mgr, m_MenuBar, "Model/Variable Presets...", vsp::VSP_VAR_PRESET_SCREEN );
    m_ModeEditMenuItem.Init( mgr, m_MenuBar, "Model/Mode Editor...", vsp::VSP_MODE_EDITOR_SCREEN );
    m_LinkMenuItem.Init( mgr, m_MenuBar, "Model/Link...", vsp::VSP_PARM_LINK_SCREEN );
    m_DesVarMenuItem.Init( mgr, m_MenuBar, "Model/Design Variables...", vsp::VSP_DESIGN_VAR_SCREEN );
    m_LabelMenuItem.Init( mgr, m_MenuBar, "Model/Measure...", vsp::VSP_MEASURE_SCREEN );
    m_LightMenuItem.Init( mgr, m_MenuBar, "Model/Lighting...", vsp::VSP_LIGHTING_SCREEN );
    m_ClipMenuItem.Init( mgr, m_MenuBar, "Model/Clipping...", vsp::VSP_CLIPPING_SCREEN );
    m_TextureMenuItem.Init( mgr, m_MenuBar, "Model/Texture...", vsp::VSP_MANAGE_TEXTURE_SCREEN );
    m_AdvLinkMenuItem.Init( mgr, m_MenuBar, "Model/Adv Link...", vsp::VSP_ADV_LINK_SCREEN );
    m_UserParmMenuItem.Init( mgr, m_MenuBar, "Model/User Parms...", vsp::VSP_USER_PARM_SCREEN );
    m_AttributeMenuItem.Init( mgr, m_MenuBar, "Model/Attribute Explorer...", vsp::VSP_ATTRIBUTE_EXPLORER_SCREEN );
    m_VehNotesMenuItem.Init( mgr, m_MenuBar, "Model/Vehicle Notes...", vsp::VSP_VEH_NOTES_SCREEN );
    m_FitModelMenuItem.Init( mgr, m_MenuBar, "Model/Fit Model...", vsp::VSP_FIT_MODEL_SCREEN );
    m_SnapToMenuItem.Init( mgr, m_MenuBar, "Model/Snap To...", vsp::VSP_SNAP_TO_SCREEN );

    m_CompGeomMenuItem.Init( mgr, m_MenuBar, "Analysis/CompGeom...", vsp::VSP_COMP_GEOM_SCREEN );
    m_PSliceMenuItem.Init( mgr, m_MenuBar, "Analysis/Planar Slice...", vsp::VSP_PSLICE_SCREEN );
    m_MassPropMenuItem.Init( mgr, m_MenuBar, "Analysis/Mass Prop...", vsp::VSP_MASS_PROP_SCREEN );
    m_ProjectionMenuItem.Init( mgr, m_MenuBar, "Analysis/Projected Area...", vsp::VSP_PROJECTION_SCREEN );
    m_InterferenceMenuItem.Init( mgr, m_MenuBar, "Analysis/Geometry Analysis...", vsp::VSP_GEOMETRY_ANALYSIS_SCREEN );
    m_DegenGeomMenuItem.Init( mgr, m_MenuBar, "Analysis/DegenGeom...", vsp::VSP_DEGEN_GEOM_SCREEN );
    m_SurfIntMenuItem.Init( mgr, m_MenuBar, "Analysis/Trimmed Surfaces...", vsp::VSP_SURFACE_INTERSECTION_SCREEN );
    m_CFDMeshMenuItem.Init( mgr, m_MenuBar, "Analysis/CFD Mesh...", vsp::VSP_CFD_MESH_SCREEN );

    m_StructMenuItem.Init( mgr, m_MenuBar, "Analysis/Structure/FEA Structure...", vsp::VSP_STRUCT_SCREEN );
    m_StructAssemblyMenuItem.Init( mgr, m_MenuBar, "Analysis/Structure/FEA Assembly...", vsp::VSP_STRUCT_ASSEMBLY_SCREEN );
    m_AeroStructMenuItem.Init( mgr, m_MenuBar, "Analysis/Structure/Aero Structural...", vsp::VSP_AERO_STRUCT_SCREEN );

    m_VSPAEROMenuItem.Init( mgr, m_MenuBar, "Analysis/Aero/VSPAERO...", vsp::VSP_VSPAERO_SCREEN );
    m_WaveDragMenuItem.Init( mgr, m_MenuBar, "Analysis/Aero/Wave Drag...", vsp::VSP_WAVEDRAG_SCREEN );
    m_ParasiteDragMenuItem.Init( mgr, m_MenuBar, "Analysis/Aero/Parasite Drag...", vsp::VSP_PARASITE_DRAG_SCREEN );


    // m_AboutMenuItem.Init( mgr, m_MenuBar, "Help/About...", ScreenMgr:: );
    m_NextProjectMenuItem.Init( this, m_MenuBar, "Window/Next Project", FL_CTRL + FL_Tab );
    m_PreviousProjectMenuItem.Init( this, m_MenuBar, "Window/Previous Project", FL_CTRL + FL_SHIFT + FL_Tab );
    m_ToggleSidebarMenuItem.Init( this, m_MenuBar, "Window/Toggle Sidebar", FL_COMMAND + 'b' );
    m_FieldGuideMenuItem.Init( this, m_MenuBar, "Help/Field Guide..." );
    m_RisksMenuItem.Init( this, m_MenuBar, "Help/Risks and Limitations..." );
    m_OnlineHelpMenuItem.Init( this, m_MenuBar, "Help/Reference Manual..." );
    // m_VersionMenuItem.Init( mgr, m_MenuBar, "Help/Check Latest Version...", ScreenMgr:: );

    // Add some hidden test menu entries.
    // m_TestParmMapMenuItem.Init( this, m_MenuBar, "Test/Test Parm Map..." );
    // m_TestParmContainerMapMenuItem.Init( this, m_MenuBar, "Test/Test ParmContainer Map..." );

    BoldEntries( "File" );
    BoldEntries( "Edit" );
    BoldEntries( "Window" );
    BoldEntries( "View" );
    BoldEntries( "Model" );
    BoldEntries( "Analysis" );
    BoldEntries( "Undo" );
    BoldEntries( "Help" );

    grp->begin();
    m_Workspace = new QaspWorkspace( 0, 26, w_allow, h_allow - 44 );
    Fl_Group *canvas = m_Workspace->Canvas();
    m_GlWin = new VSPGUI::VspGlWindow( canvas->x(), canvas->y(), canvas->w(), canvas->h(),
                                     mgr, DrawObj::VSP_MAIN_SCREEN );
    m_Workspace->SetViewport( m_GlWin );
    m_Workspace->OnAction( [this]( int action ) { WorkspaceAction( action ); } );
    m_Workspace->OnSelectTab( [this]( std::size_t index ) { SelectProject( index ); } );
    m_Workspace->OnCloseTab( [this]( std::size_t index ) { CloseProject( index ); } );

    grp->begin();
    m_GlWin->setWindowLayout( 1, 1 );

    // Compact 18-pixel strip, not a second large filename dock.
    int filew = static_cast<int>( w_allow * 0.74 );
    m_FileNameBox = new Fl_Box( 0, h_allow - 18, filew, 18 );
    m_FileNameBox->box( FL_FLAT_BOX );
    m_FileNameBox->color( fl_rgb_color( 233, 238, 245 ) );
    m_FileNameBox->labelsize( 11 );
    m_FileNameBox->labelcolor( fl_rgb_color( 73, 88, 108 ) );
    m_FileNameBox->align( FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP );
    m_TitleBox = new Fl_Box( filew, h_allow - 18, w_allow - filew, 18, "QASP  |  Conceptual design  |  AS IS" );
    m_TitleBox->box( FL_FLAT_BOX );
    m_TitleBox->color( fl_rgb_color( 233, 238, 245 ) );
    m_TitleBox->labelsize( 10 );
    m_TitleBox->labelcolor( fl_rgb_color( 73, 88, 108 ) );
    m_TitleBox->align( FL_ALIGN_RIGHT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP );
    grp->end();
    grp->resizable( m_Workspace );
    m_FLTK_Window->resizable( grp );
    m_FLTK_Window->end();
}

MainVSPScreen::~MainVSPScreen()
{
    m_Projects.reset();
    m_ProjectHost.reset();
    // The GL window removes itself from the canvas; hosted screen windows are
    // detached (or already deleted by their owners) before the workspace dies.
    delete m_GlWin;
    m_GlWin = nullptr;
    delete m_Workspace;
    m_Workspace = nullptr;
    delete m_FLTK_Window;
    m_FLTK_Window = nullptr;
}

//==== Show Main VSP Screen ====//
void MainVSPScreen::Show()
{
    if ( m_FLTK_Window )
    {
        ActionScreen::Show();
    }

    if ( m_GlWin )
    {
        m_GlWin->show();
    }
}

void MainVSPScreen::Hide()
{
    if ( m_FLTK_Window )
    {
        m_FLTK_Window->hide();
    }

    if ( m_GlWin )
    {
        m_GlWin->ResetDrawObjsGeomChangedFlags( true );
        m_GlWin->clearScene();
        m_GlWin->hide();
    }
}

//==== Update Main VSP Screen ====//
bool MainVSPScreen::Update()
{
    ActionScreen::Update();

    Vehicle *veh = VehicleMgr.GetVehicle();

    if ( veh )
    {
        if ( veh->m_ViewDirty && m_ScreenMgr && m_GlWin  )
        {
            ManageViewScreen* mvs = dynamic_cast<ManageViewScreen*>( m_ScreenMgr->GetScreen( vsp::VSP_VIEW_SCREEN ) );

            if ( mvs )
            {
                // Resize Viewport and window to your maximum screen size. Achieves any ratio.
                if ( veh->m_ViewportSizeXValue.Get() > 0 )
                {
                    ResizeWindow( veh->m_ViewportSizeXValue.Get(), veh->m_ViewportSizeYValue.Get() );
                }

                //===== Update Center of Rotation =====//
                m_GlWin->setCOR( glm::vec3( veh->m_CORXValue.Get(), veh->m_CORYValue.Get(), veh->m_CORZValue.Get() ) );

                m_GlWin->relativePan( veh->m_PanXPosValue.Get(), veh->m_PanYPosValue.Get() );

                m_GlWin->relativeZoom( veh->m_ZoomValue.Get() );

                //===== LookAt Point Method =====//
                m_GlWin->rotateSphere( veh->m_XRotationValue.Get() * ( M_PI / 180.0 ),
                                       veh->m_YRotationValue.Get() * ( M_PI / 180.0 ),
                                       veh->m_ZRotationValue.Get() * ( M_PI / 180.0 ) );
            }
            veh->m_ViewDirty = false;
        }
    }

    EnsureProjectSession();
    if ( m_Projects && !m_Projects->IsOperating() )
    {
        const auto ids = veh->GetGeomVec();
        for ( const auto &id : ids )
        {
            if ( std::find( m_LastGeometryIDs.begin(), m_LastGeometryIDs.end(), id ) == m_LastGeometryIDs.end() )
            { m_AutoFitRequested = true; break; }
        }
    }
    if ( m_AutoFitRequested )
    {
        std::string busyReason;
        if ( !m_ProjectHost || !m_ProjectHost->Busy( busyReason ) )
        {
            m_AutoFitRequested = false;
            veh->Update();
            FitView( true );
        }
    }
    m_GlWin->update();
    m_GlWin->redraw();
    if ( m_Projects && !m_Projects->IsOperating() )
    {
        if ( m_LastObservedParmChanges >= 0 &&
             ( m_LastObservedParmChanges != ParmMgr.GetNumParmChanges() || m_LastModelNames != QaspModelNames( veh ) ) )
        { m_Projects->MarkPossiblyChanged(); }
        RefreshProjects( false );
    }
    else { SetFileLabel( veh->GetVSP3FileName() ); }
    m_Workspace->Synchronize();

    m_AxisMenuItem.Update( m_ShowXYZArrow );
    m_BorderMenuItem.Update( m_ShowBorder );

    m_FLTK_Window->redraw();
    return true;
}

//==== Resize the main window ====//
void MainVSPScreen::ResizeWindow( int pixel_w, int pixel_h )
{
    double factor = m_GlWin->pixels_per_unit();
    int oldw = m_GlWin->pixel_w() / factor;
    int oldh = m_GlWin->pixel_h() / factor;

    int dw = m_FLTK_Window->w() - oldw;
    int dh = m_FLTK_Window->h() - oldh;

    m_FLTK_Window->size( pixel_w / factor + dw, pixel_h / factor + dh );
}

void MainVSPScreen::BoldEntries( const string &mpath )
{
    Fl_Menu_Item* item = nullptr;
    item = (Fl_Menu_Item*) m_MenuBar->find_item( mpath.c_str() );
    if ( item )
    {
        item->labelfont( FL_HELVETICA_BOLD );
    }
}

void MainVSPScreen::CallBack( Fl_Widget* w )
{
    assert( m_ScreenMgr );
    m_ScreenMgr->SetUpdateFlag( true );
}

void MainVSPScreen::GuiDeviceCallBack( GuiDevice* device )
{
    assert( m_ScreenMgr );
    m_ScreenMgr->SetUpdateFlag( true );
}

void MainVSPScreen::CloseCallBack( Fl_Widget *w )
{
    if ( m_StopGUIMenuItem.IsShown() )
    {
        m_ScreenMgr->APIHideScreens();       // Runs in main thread, is blocking.
        m_ScreenMgr->SetRunGui( false );
    }
    else if ( m_ExitMenuItem.IsShown() )
    {
        ExitVSP();
    }
}

void MainVSPScreen::SetFileLabel( const string &fname )
{
    string label = "  " + fname;
    m_FileNameBox->copy_label( label.c_str() );
    m_FileNameBox->copy_tooltip( fname.c_str() );
    if ( !m_Projects )
    {
        string title = string( VSPVERSION2 ) + " — " + GetFilename( fname );
        m_FLTK_Window->copy_label( title.c_str() );
    }
}

namespace
{
std::string QaspModelNames( Vehicle *vehicle )
{
    std::string value = vehicle->GetName();
    for ( const std::string &id : vehicle->GetGeomVec() )
    {
        value += '\0'; value += id;
        if ( Geom *geom = vehicle->FindGeom( id ) ) { value += '\0'; value += geom->GetName(); }
    }
    return value;
}
void QaspClearReadOnlyOutput( Fl_Group *group )
{
    if ( !group ) { return; }
    for ( int i = 0; i < group->children(); ++i )
    {
        Fl_Widget *widget = group->child( i );
        if ( auto *terminal = dynamic_cast<Fl_Terminal *>( widget ) )
        { terminal->clear(); terminal->clear_history(); }
        else if ( auto *display = dynamic_cast<Fl_Text_Display *>( widget ) )
        {
            // Never clear an editor/notes field, which can write back into a model.
            if ( !dynamic_cast<Fl_Text_Editor *>( widget ) && display->buffer() )
            { display->buffer()->text( "" ); }
        }
        else if ( widget->as_group() && !widget->as_gl_window() ) { QaspClearReadOnlyOutput( widget->as_group() ); }
    }
}
}

void MainVSPScreen::InitializeWorkspace()
{
    if ( m_WorkspaceInitialized || !m_Workspace ) { return; }
    m_WorkspaceInitialized = true;
    auto *browser = dynamic_cast<ManageGeomScreen *>( m_ScreenMgr->GetScreen( vsp::VSP_MANAGE_GEOM_SCREEN ) );
    if ( browser )
    {
        if ( auto *window = browser->GetVSPWindow() )
        { window->SetGeomScreenFlag( false ); window->SetParent( nullptr ); }
        m_Workspace->ComponentsArea()->Host( browser->GetFlWindow() );
        for ( VspScreen *screen : browser->GetGeomScreenVec() )
        {
            if ( auto *window = screen->GetVSPWindow() )
            { window->SetGeomScreenFlag( false ); window->SetParent( nullptr ); }
            m_Workspace->PropertiesArea()->Host( screen->GetFlWindow() );
        }
    }
    if ( VspScreen *screen = m_ScreenMgr->GetScreen( vsp::VSP_VEH_SCREEN ) )
    {
        if ( auto *window = screen->GetVSPWindow() )
        { window->SetGeomScreenFlag( false ); window->SetParent( nullptr ); }
        m_Workspace->PropertiesArea()->Host( screen->GetFlWindow() );
    }
    if ( m_ScreenMgr->IsGUIScreenDisabled( vsp::VSP_MANAGE_GEOM_SCREEN ) )
    {
        if ( VspScreen *alternate = m_ScreenMgr->GetScreen( vsp::VSP_NERF_MANAGE_GEOM_SCREEN ) )
        { m_Workspace->ComponentsArea()->Host( alternate->GetFlWindow() ); }
    }
    // Keep the drawing area open by default. The left icons reveal the tree
    // or editor on demand; Ctrl+B restores the last panel and toggles it closed.
    m_Workspace->HidePanel();
    if ( !VehicleMgr.GetVehicle()->GetGeomVec().empty() ) { FitView( true ); }
    EnsureProjectSession();
}

void MainVSPScreen::EnsureProjectSession()
{
    if ( m_Projects ) { return; }
    if ( !m_ProjectHost ) { m_ProjectHost.reset( new QaspVehicleProjectHost( *m_ScreenMgr, *this ) ); }
    std::string error;
    if ( m_ProjectHost->Busy( error ) ) { return; }
    m_Projects.reset( new QaspProjectSession( *m_ProjectHost ) );
    std::string file = VehicleMgr.GetVehicle()->GetVSP3FileName();
    if ( file == "Unnamed.vsp3" ) { file.clear(); }
    // A startup file may already have been altered by a command-line script.
    // There is no trustworthy pre-script baseline; conservatively offer Save.
    bool unsavedModel = !VehicleMgr.GetVehicle()->GetGeomVec().empty();
    if ( !m_Projects->Adopt( file, error, unsavedModel ) )
    {
        m_ScreenMgr->Alert( "Project tabs cannot allocate a complete working copy. "
            "Your current model is still active; save it explicitly.\n" + error );
    }
    RefreshProjects();
}

bool MainVSPScreen::ProjectReady()
{
    EnsureProjectSession();
    std::string error;
    if ( m_ProjectHost->Busy( error ) ) { m_ScreenMgr->Alert( error ); return false; }
    if ( !m_Projects || m_Projects->IsOperating() ) { return false; }
    if ( m_Projects->RecoveryRequired() )
    {
        m_ScreenMgr->Alert( "A project restore failed. Do not overwrite an original file. "
            "Keep the working copies for recovery:\n" + m_Projects->WorkingDirectory().generic_u8string() );
        return false;
    }
    // Commit a pending typed numeric/name field before taking a snapshot. A
    // keyboard shortcut must not save the previous value while text is uncommitted.
    if ( auto *input = dynamic_cast<Fl_Input_ *>( Fl::focus() ) )
    {
        if ( input->changed() ) { input->do_callback(); }
    }
    return true;
}

void MainVSPScreen::RefreshProjects( bool requestUpdate )
{
    if ( !m_Projects || m_Projects->Projects().empty() ) { return; }
    std::vector<QaspTabLabel> labels;
    for ( const QaspProject &project : m_Projects->Projects() )
    {
        bool dirty = project.dirty || project.possiblyChanged;
        labels.push_back( { project.title, project.file.empty() ? project.title + " — not explicitly saved" : project.file, dirty } );
    }
    m_Workspace->SetTabs( labels, m_Projects->ActiveIndex() );
    const QaspProject &active = m_Projects->Active();
    SetFileLabel( active.file.empty() ? "Unsaved — " + active.title : active.file );
    std::string title = "QASP — " + active.title + ( active.dirty || active.possiblyChanged ? " *" : "" );
    m_FLTK_Window->copy_label( title.c_str() );
    m_Workspace->SetProjectCaption( std::to_string( VehicleMgr.GetVehicle()->GetGeomVec().size() ) + " components  |  " + active.title );
    m_LastObservedParmChanges = ParmMgr.GetNumParmChanges();
    m_LastModelNames = QaspModelNames( VehicleMgr.GetVehicle() );
    m_LastGeometryIDs = VehicleMgr.GetVehicle()->GetGeomVec();
    m_Workspace->Synchronize();
    if ( requestUpdate ) { m_ScreenMgr->SetUpdateFlag( true ); }
}

bool MainVSPScreen::ConfirmProjectReload()
{
    if ( m_ReloadAcknowledged ) { return true; }
    int answer = fl_choice( "%s", "Cancel", "Continue", nullptr,
        "Each tab keeps its own model and project settings. Only one model is active in the engine.\n\n"
        "Changing projects reloads the engine and clears generated results/mesh previews, parameter undo, "
        "and the clipboard. Export any results you need first.\n\n"
        "Working snapshots are not guaranteed autosave or crash recovery. Save project files deliberately." );
    m_ReloadAcknowledged = answer == 1;
    return m_ReloadAcknowledged;
}

void MainVSPScreen::ShowWorkspacePanel( int panel )
{
    if ( !m_Workspace ) { return; }
    m_Workspace->ShowPanel( static_cast<QaspWorkspace::Panel>( panel ) );
    if ( panel == QaspWorkspace::Properties )
    {
        if ( auto *browser = dynamic_cast<ManageGeomScreen *>( m_ScreenMgr->GetScreen( vsp::VSP_MANAGE_GEOM_SCREEN ) ) )
        { browser->SetNeedsShowHideGeoms(); }
    }
    m_ScreenMgr->SetUpdateFlag( true );
}

void MainVSPScreen::RequestAutoFit()
{
    m_AutoFitRequested = true;
    if ( m_Projects ) { m_Projects->MarkPossiblyChanged(); }
    m_ScreenMgr->SetUpdateFlag( true );
}

void MainVSPScreen::PrepareProjectChange()
{
    m_AutoFitRequested = false;
    m_ScreenMgr->m_ShowPlotScreenOnce = false;
    m_GlWin->ResetDrawObjsGeomChangedFlags( true );
    m_GlWin->clearScene();
    for ( VspScreen *screen : m_ScreenMgr->GetAllScreens() )
    {
        if ( !screen || screen == this || screen->GetScreenType() == vsp::VSP_COR_SCREEN ) { continue; }
        screen->ResetProjectBinding();
        if ( screen->GetScreenType() != vsp::VSP_MANAGE_GEOM_SCREEN && screen->GetScreenType() != vsp::VSP_NERF_MANAGE_GEOM_SCREEN )
        { screen->Hide(); QaspClearReadOnlyOutput( screen->GetFlWindow() ); }
    }
    if ( auto *browser = dynamic_cast<ManageGeomScreen *>( m_ScreenMgr->GetScreen( vsp::VSP_MANAGE_GEOM_SCREEN ) ) )
    {
        browser->ResetProjectSelection();
        for ( VspScreen *screen : browser->GetGeomScreenVec() ) { screen->ResetProjectBinding(); screen->Hide(); }
    }
}

std::string MainVSPScreen::CaptureProjectView()
{
    m_GlWin->UpdateAllViewParms();
    auto *layout = m_GlWin->getGraphicEngine()->getDisplay()->getLayoutMgr();
    std::vector<VSPGraphic::Viewport *> views = layout->getViewports();
    int selected = 0;
    for ( std::size_t i = 0; i < views.size(); ++i ) { if ( views[i] == layout->getViewport() ) { selected = static_cast<int>( i ); } }
    std::ostringstream out;
    out.imbue( std::locale::classic() ); out << std::setprecision( 17 );
    out << "1 " << layout->row() << ' ' << layout->column() << ' ' << selected << ' ' << m_ShowXYZArrow << ' ' << m_ShowBorder;
    for ( auto *view : views )
    {
        auto *camera = view->getCamera();
        glm::vec3 cor = camera->getCOR(), rotation = camera->getRotationEulerAngles();
        glm::vec2 pan = camera->getPanValues();
        out << ' ' << cor.x << ' ' << cor.y << ' ' << cor.z << ' ' << pan.x << ' ' << pan.y << ' '
            << camera->getRelativeZoomValue() << ' ' << rotation.x << ' ' << rotation.y << ' ' << rotation.z;
    }
    return out.str();
}

void MainVSPScreen::RestoreProjectView( const std::string &view )
{
    std::istringstream in( view ); in.imbue( std::locale::classic() );
    int version, rows, columns, selected; bool axis, borders;
    if ( !( in >> version >> rows >> columns >> selected >> axis >> borders ) || version != 1 ||
         rows < 1 || rows > 2 || columns < 1 || columns > 2 || selected < 0 || selected >= rows * columns ) { return; }
    std::vector<std::vector<double>> values;
    for ( int i = 0; i < rows * columns; ++i )
    {
        std::vector<double> camera( 9 );
        for ( double &value : camera ) { if ( !( in >> value ) || !std::isfinite( value ) ) { return; } }
        if ( camera[5] <= 0 ) { return; }
        values.push_back( camera );
    }
    SetWindowLayout( rows, columns ); SetViewAxis( axis ); SetShowBorders( borders );
    auto *layout = m_GlWin->getGraphicEngine()->getDisplay()->getLayoutMgr();
    auto views = layout->getViewports();
    for ( std::size_t i = 0; i < values.size(); ++i )
    {
        auto *camera = views[i]->getCamera(); const auto &v = values[i];
        camera->setCOR( v[0], v[1], v[2] ); camera->relativePan( v[3], v[4] );
        camera->relativeZoom( v[5] ); camera->rotateSphere( v[6], v[7], v[8] );
    }
    layout->selectViewport( selected ); m_GlWin->UpdateAllViewParms();
    VehicleMgr.GetVehicle()->m_ViewDirty = false;
}

bool MainVSPScreen::SaveProject( bool saveAs )
{
    if ( !ProjectReady() ) { return false; }
    const QaspProject &project = m_Projects->Active();
    std::string file = saveAs ? "" : project.file;
    if ( file.empty() )
    {
        file = m_ScreenMgr->FileChooser( saveAs ? "Save Project As" : "Save Project", "*.vsp3", vsp::SAVE, project.title );
        if ( file.empty() ) { return false; } // Cancel is never permission to close.
    }
    std::string error;
    if ( !m_Projects->Save( file, error ) ) { m_ScreenMgr->Alert( error ); RefreshProjects(); return false; }
    if ( !m_Projects->Warning().empty() ) { m_ScreenMgr->Alert( m_Projects->Warning() ); }
    RefreshProjects();
    return true;
}

void MainVSPScreen::NewModel()
{
    if ( !ProjectReady() || !ConfirmProjectReload() ) { return; }
    std::string error;
    if ( !m_Projects->New( error ) ) { m_ScreenMgr->Alert( error ); }
    RefreshProjects();
}

void MainVSPScreen::OpenProject()
{
    if ( !ProjectReady() ) { return; }
    std::string file = m_ScreenMgr->FileChooser( "Open Project in a Tab", "*.vsp3" );
    if ( file.empty() || !ConfirmProjectReload() ) { return; }
    std::string error;
    if ( !m_Projects->Open( file, error ) ) { m_ScreenMgr->Alert( error ); }
    else if ( VehicleMgr.GetVehicle()->m_ShowNotesScreenParm.Get() ) { m_ScreenMgr->ShowScreen( vsp::VSP_VEH_NOTES_SCREEN ); }
    RefreshProjects();
}

bool MainVSPScreen::SelectProject( std::size_t index )
{
    if ( !ProjectReady() || index >= m_Projects->Projects().size() ) { return false; }
    if ( index == m_Projects->ActiveIndex() ) { return true; }
    if ( !ConfirmProjectReload() ) { return false; }
    std::string error;
    bool ok = m_Projects->Switch( index, error );
    if ( !ok ) { m_ScreenMgr->Alert( error ); }
    else if ( !m_Projects->Warning().empty() ) { m_ScreenMgr->Alert( m_Projects->Warning() ); }
    RefreshProjects();
    return ok;
}

void MainVSPScreen::CloseProject( std::size_t index )
{
    if ( !ProjectReady() || index >= m_Projects->Projects().size() ) { return; }
    std::string error;
    if ( index != m_Projects->ActiveIndex() )
    {
        const QaspProject &target = m_Projects->Projects()[index];
        int answer = 1; // A known-clean inactive tab needs no engine reload.
        if ( target.dirty || target.possiblyChanged )
        {
            std::string prompt = "Save changes to " + target.title + " before closing this project?";
            answer = fl_choice( "%s", "Cancel", "Discard", "Save", prompt.c_str() );
        }
        if ( answer == 0 ) { return; }
        if ( answer == 1 )
        {
            if ( !m_Projects->DiscardInactive( index, error ) ) { m_ScreenMgr->Alert( error ); }
            RefreshProjects(); return;
        }
        if ( !SelectProject( index ) || !SaveProject() ) { return; }
    }
    if ( !m_Projects->Checkpoint( error ) ) { m_ScreenMgr->Alert( error ); return; }
    if ( m_Projects->Active().dirty )
    {
        std::string prompt = "Save changes to " + m_Projects->Active().title + " before closing this project?";
        int answer = fl_choice( "%s", "Cancel", "Discard", "Save", prompt.c_str() );
        if ( answer == 0 || ( answer == 2 && !SaveProject() ) ) { return; }
    }
    if ( !ConfirmProjectReload() ) { return; }
    if ( !m_Projects->CloseActive( error ) ) { m_ScreenMgr->Alert( error ); }
    RefreshProjects();
}

void MainVSPScreen::ExitVSP()
{
    EnsureProjectSession();
    if ( m_Projects && m_Projects->RecoveryRequired() )
    {
        std::string prompt = "A project restore failed. Do not overwrite original files. Working copies remain in:\n" +
            m_Projects->WorkingDirectory().generic_u8string() + "\n\nExit without saving the uncertain active model?";
        if ( fl_choice( "%s", "Cancel", "Exit without saving", nullptr, prompt.c_str() ) == 1 ) { exit( 0 ); }
        return;
    }
    if ( !ProjectReady() ) { return; }
    std::string error;
    if ( !m_Projects->Checkpoint( error ) )
    {
        std::string prompt = "Cannot check or cache the current work:\n" + error +
            "\n\nExit anyway? Unsaved changes in ANY tab may be lost. Choose Cancel to try saving explicitly.";
        if ( fl_choice( "%s", "Cancel", "Exit without saving", nullptr, prompt.c_str() ) != 1 ) { return; }
        // Preserve any working copies on this failure path; no cleanup claim.
        exit( 0 );
    }
    std::size_t original = m_Projects->ActiveIndex();
    // Do not close/delete any tab on an intermediate Discard: the user may
    // still Cancel on a later project or cancel that project's save dialog.
    for ( std::size_t i = 0; i < m_Projects->Projects().size(); ++i )
    {
        if ( !m_Projects->Projects()[i].dirty ) { continue; }
        if ( !SelectProject( i ) )
        {
            if ( fl_choice( "%s", "Cancel", "Exit without saving", nullptr,
                 "One project could not be loaded for saving. Exit anyway? Unsaved changes in ANY tab may be lost. "
                 "Working copies will be retained rather than deliberately deleted." ) == 1 ) { exit( 0 ); }
            return;
        }
        std::string prompt = "QASP is exiting. Save changes to " + m_Projects->Active().title + "?";
        int answer = fl_choice( "%s", "Cancel", "Discard", "Save", prompt.c_str() );
        if ( answer == 0 || ( answer == 2 && !SaveProject() ) )
        { SelectProject( original ); return; }
    }
    m_Projects->CleanExit();
    exit( 0 );
}

void MainVSPScreen::WorkspaceAction( int action )
{
    switch ( action )
    {
    case QaspWorkspace::NewProject: NewModel(); break;
    case QaspWorkspace::OpenProject: OpenProject(); break;
    case QaspWorkspace::SaveProject: SaveProject(); break;
    case QaspWorkspace::FitModel: FitView( true ); break;
    case QaspWorkspace::TopView: SetView( VSPGraphic::Common::VSP_CAM_TOP ); break;
    case QaspWorkspace::FrontView: SetView( VSPGraphic::Common::VSP_CAM_FRONT ); break;
    case QaspWorkspace::IsoView: SetView( VSPGraphic::Common::VSP_CAM_LEFT_ISO ); break;
    case QaspWorkspace::FieldGuide: QaspFieldGuide::Show(); break;
    case QaspWorkspace::MassProperties: m_ScreenMgr->ShowScreen( vsp::VSP_MASS_PROP_SCREEN ); break;
    case QaspWorkspace::ParasiteDrag: m_ScreenMgr->ShowScreen( vsp::VSP_PARASITE_DRAG_SCREEN ); break;
    case QaspWorkspace::VSPAERO: m_ScreenMgr->ShowScreen( vsp::VSP_VSPAERO_SCREEN ); break;
    case QaspWorkspace::CFDMesh: m_ScreenMgr->ShowScreen( vsp::VSP_CFD_MESH_SCREEN ); break;
    case QaspWorkspace::Structures: m_ScreenMgr->ShowScreen( vsp::VSP_STRUCT_SCREEN ); break;
    case QaspWorkspace::Properties:
        if ( auto *browser = dynamic_cast<ManageGeomScreen *>( m_ScreenMgr->GetScreen( vsp::VSP_MANAGE_GEOM_SCREEN ) ) )
        { browser->SetNeedsShowHideGeoms(); }
        break;
    default: break;
    }
    m_ScreenMgr->SetUpdateFlag( true );
}

void MainVSPScreen::ActionCB( void * data )
{
    if ( !m_GlWin || !m_FLTK_Window || !m_ScreenMgr )
        return;

    if ( data == &m_NewMenuItem )
    {
        NewModel();
    }
    else if ( data == &m_OpenMenuItem ) { OpenProject(); }
    else if ( data == &m_CloseProjectMenuItem )
    {
        EnsureProjectSession();
        if ( m_Projects ) { CloseProject( m_Projects->ActiveIndex() ); }
    }
    else if ( data == &m_NextProjectMenuItem || data == &m_PreviousProjectMenuItem )
    {
        EnsureProjectSession();
        if ( m_Projects )
        {
            std::size_t count = m_Projects->Projects().size();
            std::size_t offset = data == &m_NextProjectMenuItem ? 1 : count - 1;
            SelectProject( ( m_Projects->ActiveIndex() + offset ) % count );
        }
    }
    else if ( data == &m_ToggleSidebarMenuItem )
    {
        m_Workspace->ToggleSidebar();
    }
    else if ( data == &m_SaveMenuItem ) { SaveProject(); }
    else if ( data == &m_SaveAsMenuItem ) { SaveProject( true ); }
    else if ( data == &m_SaveSetMenuItem )
    {
        if ( !ProjectReady() ) { return; }
        int set = m_ScreenMgr->GetPickSetScreen()->PickSet( "Pick Save Set" );
        if ( set >= 0 )
        {
            string file = m_ScreenMgr->FileChooser( "Save VSP Set File As", "*.vsp3", vsp::SAVE );
            if ( !file.empty() )
            {
                if ( m_Projects->IsOpenFile( file ) )
                {
                    m_ScreenMgr->Alert( "Save Set exports only selected components. Choose a filename that is not an open full project." );
                    return;
                }
                std::string error;
                bool saved = QaspWriteFileAtomically( std::filesystem::u8path( file ),
                    [&]( const std::filesystem::path &stage, std::string &err )
                    {
                        bool ok = VehicleMgr.GetVehicle()->WriteXMLFileWithBase( stage.generic_u8string(), file, set );
                        if ( !ok ) { err = "The selected set could not be written."; return false; }
                        return m_ProjectHost->Validate( stage, err );
                    }, error );
                if ( !saved ) { m_ScreenMgr->Alert( error ); }
            }
        }
    }
    else if ( data == &m_InsertMenuItem )
    {
        if ( !ProjectReady() ) { return; }
        string openfile = m_ScreenMgr->FileChooser( "Insert VSP File", "*.vsp3" );
        if ( openfile.compare( "" ) != 0 )
        {
            std::string error;
            if ( !m_ProjectHost->Validate( std::filesystem::u8path( openfile ), error ) || !m_Projects->Checkpoint( error ) )
            { m_ScreenMgr->Alert( error ); }
            else if ( VehicleMgr.GetVehicle()->ReadXMLFileGeomsOnly( openfile ) != 0 )
            { m_ScreenMgr->Alert( "Insert failed. The last working snapshot was retained; do not overwrite your original if the model looks incomplete." ); }
            else { RequestAutoFit(); }
        }
    }
    // else if ( data == &m_SetTempDirMenuItem )
    // {
    // }
    else if ( data == &m_RunScriptMenuItem )
    {
        if ( !ProjectReady() ) { return; }
        string scriptfile = m_ScreenMgr->FileChooser( "Script File To Run", "*.vspscript" );
        if ( scriptfile.compare( "" ) != 0 )
        {
            std::string error;
            if ( !m_Projects->Checkpoint( error ) ) { m_ScreenMgr->Alert( error ); }
            else { VehicleMgr.GetVehicle()->RunScript( scriptfile ); }
        }
    }
    else if ( data == &m_PreferencesMenuItem )
    {

    }
    else if ( data == &m_ExitMenuItem )
    {
        ExitVSP();
    }
    else if ( data == &m_StopGUIMenuItem )
    {
        m_ScreenMgr->APIHideScreens();          // Runs in main thread, is blocking.
        m_ScreenMgr->SetRunGui( false );
    }
    else if ( data == &m_UndoMenuItem )
    {
        VehicleMgr.GetVehicle()->UnDo();
    }
    else if ( data == &m_CutMenuItem )
    {
        DeleteOrCutActiveGeomVec( VehicleMgr.GetVehicle(), true );
    }
    else if ( data == &m_CopyMenuItem )
    {
        VehicleMgr.GetVehicle()->CopyActiveGeomVec();
    }
    else if ( data == &m_PasteMenuItem )
    {
        VehicleMgr.GetVehicle()->PasteClipboard();
    }
    else if ( data == &m_DeleteMenuItem )
    {
        DeleteOrCutActiveGeomVec( VehicleMgr.GetVehicle(), false );
    }
    else if ( data == &m_SelAllMenuItem )
    {
        VehicleMgr.GetVehicle()->SetActiveGeomVec( VehicleMgr.GetVehicle()->GetGeomVec() );

        ManageGeomScreen * geomScreen = nullptr;
        geomScreen = dynamic_cast<ManageGeomScreen *>
        ( m_ScreenMgr->GetScreen( vsp::VSP_MANAGE_GEOM_SCREEN ) );
        if( geomScreen )
        {
            geomScreen->SetNeedsShowHideGeoms();
        }
    }
    else if ( data == &m_PickMenuItem )
    {
        ManageGeomScreen * geomScreen = nullptr;
        geomScreen = dynamic_cast<ManageGeomScreen *>
        ( m_ScreenMgr->GetScreen( vsp::VSP_MANAGE_GEOM_SCREEN ) );
        if( geomScreen )
        {
            geomScreen->TriggerPickSwitch();
        }
    }
    else if ( data == &m_OneMenuItem )
    {
        SetWindowLayout( 1, 1 );
    }
    else if ( data == &m_FourMenuItem )
    {
        SetWindowLayout( 2, 2 );
    }
    else if ( data == &m_TwoHMenuItem )
    {
        SetWindowLayout( 1, 2 );
    }
    else if ( data == &m_TwoVMenuItem )
    {
        SetWindowLayout( 2, 1 );
    }
    else if ( data == &m_AxisMenuItem )
    {
        SetViewAxis( !m_ShowXYZArrow );
    }
    else if ( data == &m_BorderMenuItem )
    {
        SetShowBorders( !m_ShowBorder );
    }
    else if ( data == &m_TopMenuItem )
    {
        SetView( VSPGraphic::Common::VSP_CAM_TOP );
    }
    else if ( data == &m_FrontMenuItem )
    {
        SetView( VSPGraphic::Common::VSP_CAM_FRONT );
    }
    else if ( data == &m_LeftSideMenuItem )
    {
        SetView( VSPGraphic::Common::VSP_CAM_LEFT );
    }
    else if ( data == &m_LeftIsoMenuItem )
    {
        SetView( VSPGraphic::Common::VSP_CAM_LEFT_ISO );
    }
    else if ( data == &m_BottomMenuItem )
    {
        SetView( VSPGraphic::Common::VSP_CAM_BOTTOM );
    }
    else if ( data == &m_RearMenuItem )
    {
        SetView( VSPGraphic::Common::VSP_CAM_REAR );
    }
    else if ( data == &m_RightSideMenuItem )
    {
        SetView( VSPGraphic::Common::VSP_CAM_RIGHT );
    }
    else if ( data == &m_RightIsoMenuItem )
    {
        SetView( VSPGraphic::Common::VSP_CAM_RIGHT_ISO );
    }
    else if ( data == &m_CenterMenuItem )
    {
        SetView( VSPGraphic::Common::VSP_CAM_CENTER );
    }
    else if ( data == &m_CenterAllMenuItem )
    {
        std::vector< VSPGraphic::Viewport *> vpts = m_GlWin->getGraphicEngine()->getDisplay()->getLayoutMgr()->getViewports();

        for ( int i = 0; i < vpts.size(); i++ )
        {
            vpts[i]->getCamera()->changeView( VSPGraphic::Common::VSP_CAM_CENTER );
        }

        m_GlWin->UpdateAllViewParms();
    }
    else if ( data == &m_SetCORMenuItem )
    {
        ManageCORScreen * corScreen = nullptr;
        corScreen = dynamic_cast<ManageCORScreen *>
        ( m_ScreenMgr->GetScreen( vsp::VSP_COR_SCREEN ) );
        if( corScreen )
        {
            corScreen->EnableCORSelection();
        }
    }
    else if ( data == &m_SetVNMenuItem )
    {
        ManageCORScreen * corScreen = NULL;
        corScreen = dynamic_cast<ManageCORScreen *>
        ( m_ScreenMgr->GetScreen( vsp::VSP_COR_SCREEN ) );
        if( corScreen )
        {
            corScreen->EnableVNSelection();
        }
    }
    else if ( data == &m_AlignGeomMenuItem )
    {
        Vehicle *veh = VehicleMgr.GetVehicle();

        if ( veh )
        {
            vector < string > sel_geoms = veh->GetActiveGeomVec();
            if ( sel_geoms.size() == 1 )
            {
                AlignViewToGeom( sel_geoms[0] );
            }
        }
    }
    else if ( data == &m_FitViewMenuItem || data == &m_FitAllViewMenuItem )
    {
        FitView( data == &m_FitAllViewMenuItem );
    }
    else if ( data == &m_ResetAllViewMenuItem )
    {
        ResetViews();
    }
    else if ( data == &m_RisksMenuItem )
    {
        fl_message( "%s", "QASP is an unofficial OpenVSP modification, provided AS IS.\n\n"
            "No promise of crash-free operation, preserved files, correct calculations, security, compatibility, "
            "airworthiness, certification or suitability for a particular purpose. Save separate backups. "
            "Validate inputs, units, geometry, solver assumptions and results independently.\n\n"
            "Tabs keep separate models, not simultaneous engineering engines. Switching clears generated results, "
            "mesh previews, undo and clipboard. Working copies are not guaranteed crash recovery.\n\n"
            "Scripts and external tools are not sandboxed. The NOSA 1.3 licence remains unchanged; applicable "
            "non-waivable rights are not removed.\n\n"
            "Read RISKS_AND_LIMITATIONS.md and LICENSE in the portable folder for the full notice." );
    }
    else if ( data == &m_FieldGuideMenuItem )
    {
        // QASP: in-application plain-English help.  Deliberately independent of
        // the ScreenMgr screen list so it cannot interfere with the GUI.
        QaspFieldGuide::Show();
    }
    else if ( data == &m_OnlineHelpMenuItem )
    {
        if ( m_ScreenMgr )
        {
            m_ScreenMgr->HelpSystemDefaultBrowser( "index.html" );
        }
    }
    else if ( data == &m_TestParmMapMenuItem )
    {
        if ( !ParmMgr.ValidateParmMap() )
        {
            m_ScreenMgr->Alert( "Parm Map is not valid.");
        }
    }
    else if ( data == &m_TestParmContainerMapMenuItem )
    {
        if ( !ParmMgr.ValidateParmContainerMap() )
        {
            m_ScreenMgr->Alert( "ParmContainer Map is not valid.");
        }
    }
    // else if ( m_AntialiasMenuItem )
    // {
    // }

    m_ScreenMgr->SetUpdateFlag( true );
}

void MainVSPScreen::ScreenGrab( const string & fname, int w, int h, bool transparentBG, bool autocrop )
{
    if ( m_GlWin )
    {
        // The screenshot is rendered with the on-screen viewport's projection, whose
        // aspect ratio comes from the GL window's pixel dimensions (the screenshot path,
        // Viewport::resizeViewport, deliberately does not update the camera).  So a
        // requested w/h that does not match the screen aspect ratio squishes the image.
        // Adjust the requested dimensions to the screen aspect ratio, keeping whichever
        // of w/h yields the larger image, so the capture is undistorted.
        int sw = m_GlWin->pixel_w();
        int sh = m_GlWin->pixel_h();
        if ( sw > 0 && sh > 0 )
        {
            double screen_ar = ( double )sw / ( double )sh;
            if ( ( double )w / ( double )h < screen_ar )
            {
                // Requested box is narrower than the screen -- widen to match, keep h.
                w = ( int )( h * screen_ar + 0.5 );
            }
            else
            {
                // Requested box is wider than the screen -- grow height to match, keep w.
                h = ( int )( w / screen_ar + 0.5 );
            }
        }

        bool framebufferSupported = true;
        if ( !glewIsSupported( "GL_ARB_framebuffer_object" ) )
        {
            framebufferSupported = false;
        }

        m_GlWin->getGraphicEngine()->dumpScreenImage( fname, w, h, transparentBG, autocrop, framebufferSupported, VSPGraphic::GraphicEngine::PNG );
    }
}

bool MainVSPScreen::VectorScreenGrab( const string & fname, int filetype, bool autocrop, bool scaletopage,
                                      double pagewidth, double pageheight, int pageunits )
{
    if ( !m_GlWin )
    {
        return false;
    }

    int type = VSPGraphic::GraphicEngine::SVG;
    if ( filetype == vsp::VEC_PDF )
    {
        type = VSPGraphic::GraphicEngine::PDF;
    }

    double scale = VectorPageScale( m_GlWin->pixel_w(), m_GlWin->pixel_h(), scaletopage, pagewidth, pageheight,
                                    pageunits );

    return m_GlWin->WriteVector( fname, type, autocrop, scale );
}

double MainVSPScreen::VectorPageScale( int w, int h, bool scaletopage, double pagewidth, double pageheight,
                                       int pageunits )
{
    if ( !scaletopage || w <= 0 || h <= 0 )
    {
        return 1.0;
    }

    // 72 points to the inch
    double pw = ConvertLength( pagewidth, pageunits, vsp::LEN_IN ) * 72.0;
    double ph = ConvertLength( pageheight, pageunits, vsp::LEN_IN ) * 72.0;
    double scale = pw / w;
    if ( ph / h < scale )
    {
        scale = ph / h;
    }
    return scale;
}

void MainVSPScreen::SetViewAxis( bool vaxis )
{
    if ( m_GlWin )
    {
        m_ShowXYZArrow = vaxis;

        vector< VSPGraphic::Viewport * > vports = m_GlWin->getGraphicEngine()->getDisplay()->getLayoutMgr()->getViewports();
        for ( int i = 0; i < vports.size(); i++ )
        {
            if ( vports[i] )
            {
                vports[i]->showXYZArrows( m_ShowXYZArrow );
            }
        }
    }
}

void MainVSPScreen::SetShowBorders( bool brdr )
{
    if ( m_GlWin )
    {
        m_ShowBorder = brdr;

        vector< VSPGraphic::Viewport * > vports = m_GlWin->getGraphicEngine()->getDisplay()->getLayoutMgr()->getViewports();
        for ( int i = 0; i < vports.size(); i++ )
        {
            if ( vports[i] )
            {
                vports[i]->showBorders( m_ShowBorder );
            }
        }
    }
}

void MainVSPScreen::SetBackground( double r, double g, double b )
{
    if ( m_GlWin ) {

        vector< VSPGraphic::Viewport * > vports = m_GlWin->getGraphicEngine()->getDisplay()->getLayoutMgr()->getViewports();
        for ( int i = 0; i < vports.size(); i++ )
        {
            if ( vports[i] )
            {
                vports[i]->getBackground()->setRed((float) (r));
                vports[i]->getBackground()->setGreen((float) (g));
                vports[i]->getBackground()->setBlue((float) (b));
            }
        }
    }
}

int MainVSPScreen::ConvertViewEnums( int v )
{
    return  VSPGraphic::Common::VSP_CAM_TOP + v - vsp::CAM_TOP;
}

void MainVSPScreen::SetAllViews( int view )
{
    std::vector< VSPGraphic::Viewport *> vpts = m_GlWin->getGraphicEngine()->getDisplay()->getLayoutMgr()->getViewports();

    for ( int i = 0; i < vpts.size(); i++ )
    {
        vpts[i]->getCamera()->changeView( static_cast < VSPGraphic::Common::VSPenum > ( view ) );
    }

    m_GlWin->UpdateAllViewParms();
}

void MainVSPScreen::SetView( int viewport, int view )
{
    std::vector< VSPGraphic::Viewport *> vpts = m_GlWin->getGraphicEngine()->getDisplay()->getLayoutMgr()->getViewports();

    if ( viewport >= 0 && viewport < vpts.size() )
    {
        vpts[ viewport ]->getCamera()->changeView( static_cast < VSPGraphic::Common::VSPenum > ( view ) );
    }

    m_GlWin->UpdateAllViewParms();
}

void MainVSPScreen::SetView( int view )
{
    m_GlWin->setView( static_cast < VSPGraphic::Common::VSPenum > ( view ) );

    m_GlWin->UpdateAllViewParms();
}

void MainVSPScreen::FitView( bool all )
{
    BndBox bbox;

    bool vehvisible = VehicleMgr.GetVehicle()->GetVisibleBndBox( bbox );

    bool cfdvisible = false;
    // Load visible boundign box from CfdMeshScreen.
    CfdMeshScreen * cfdScreen = dynamic_cast< CfdMeshScreen* >
    ( m_ScreenMgr->GetScreen( vsp::VSP_CFD_MESH_SCREEN ) );
    if( cfdScreen )
    {
        cfdvisible = cfdScreen->GetVisBndBox( bbox );
    }

    bool surfvisible = false;
    // Load visible boundign box from SurfaceIntersectionScreen.
    SurfaceIntersectionScreen * surfScreen = dynamic_cast< SurfaceIntersectionScreen* >
    ( m_ScreenMgr->GetScreen( vsp::VSP_SURFACE_INTERSECTION_SCREEN ) );
    if( surfScreen )
    {
        surfvisible = surfScreen->GetVisBndBox( bbox );
    }

    bool structvisible = false;
    // Load visible boundign box from FeaStructScreen.
    StructScreen * structScreen = dynamic_cast< StructScreen* >
    ( m_ScreenMgr->GetScreen( vsp::VSP_STRUCT_SCREEN ) );
    if( structScreen )
    {
        structvisible = structScreen->GetVisBndBox( bbox );
    }

    bool assemblyvisible = false;
    // Load visible boundign box from FeaStructAssemblyScreen.
    StructAssemblyScreen * structAssemblyScreen = dynamic_cast< StructAssemblyScreen* >
    ( m_ScreenMgr->GetScreen( vsp::VSP_STRUCT_ASSEMBLY_SCREEN ) );
    if( structAssemblyScreen )
    {
        assemblyvisible = structAssemblyScreen->GetVisBndBox( bbox );
    }

    bool geometryanalysisvisible = false;
    // Load visible boundign box from GeometryAnalysisScreen.
    GeometryAnalysisScreen * geometryAnalysisScreen = dynamic_cast< GeometryAnalysisScreen* >
    ( m_ScreenMgr->GetScreen( vsp::VSP_GEOMETRY_ANALYSIS_SCREEN ) );
    if( geometryAnalysisScreen )
    {
        geometryanalysisvisible = geometryAnalysisScreen->GetVisBndBox( bbox );
    }

    if ( vehvisible || cfdvisible || surfvisible || structvisible || assemblyvisible || geometryanalysisvisible ) // Check for any visible objects
    {
        vec3d p = bbox.GetCenter();
        double d = bbox.DiagDist();

        std::vector< VSPGraphic::Viewport *> vpts = m_GlWin->getGraphicEngine()->getDisplay()->getLayoutMgr()->getViewports();

        int wid = m_GlWin->pixel_w();
        int ht = m_GlWin->pixel_h();

        for ( int i = 0; i < vpts.size(); i++ )
        {
            if ( vpts[i]->width() > 0 ) { wid = MIN( wid, vpts[i]->width() ); }
            if ( vpts[i]->height() > 0 ) { ht = MIN( ht, vpts[i]->height() ); }
        }

        if ( wid <= 0 || ht <= 0 || !std::isfinite( d ) || d <= 1e-12 ||
             !std::isfinite( p.x() ) || !std::isfinite( p.y() ) || !std::isfinite( p.z() ) ) { return; }
        float z = d * 1.08 * ( wid < ht ? 1.f / wid : 1.f / ht );
        if ( !std::isfinite( z ) || z <= 0 ) { return; }

        if ( all )
        {
            for ( int i = 0; i < vpts.size(); i++ )
            {
                vpts[i]->getCamera()->setCOR( -p.x(), -p.y(), -p.z() );
                vpts[i]->getCamera()->relativePan( 0.0f, 0.0f );
                vpts[i]->getCamera()->relativeZoom( z );
            }
        }
        else
        {
            m_GlWin->getGraphicEngine()->getDisplay()->setCOR( -p.x(), -p.y(), -p.z() );
            m_GlWin->getGraphicEngine()->getDisplay()->relativePan( 0.0f, 0.0f );
            m_GlWin->getGraphicEngine()->getDisplay()->getCamera()->relativeZoom( z );
        }

        m_GlWin->UpdateAllViewParms();
    }
}

void MainVSPScreen::AlignView( const vec3d & v )
{
    const vec3d worldUp = vec3d(0., 0., 1.);
    const vec3d camDir = -v;
    vec3d camRight = cross(worldUp, camDir);
    vec3d camUp = cross(camDir, camRight);

    camRight.normalize();
    camUp.normalize();

    Matrix4d norm_basis;
    norm_basis.setBasis( camRight, camUp, camDir );

    const vec3d norm_angles = norm_basis.getArcballAngles();

    m_GlWin->rotateSphere(
        norm_angles.x(),
        norm_angles.y(),
        norm_angles.z()
    );

    m_GlWin->UpdateAllViewParms();
}

void MainVSPScreen::AlignViewToGeom( string geom_id )
{
    Vehicle* veh = VehicleMgr.GetVehicle();
    if ( !veh )
    {
        return;
    }

    // try to find the geom by the given ID, default to base vehicle orientation
    Matrix4d model_matrix;

    Geom* geom = veh->FindGeom( geom_id );
    if ( geom )
    {
        // get geom model matrix
        model_matrix = geom->getModelMatrix();
    }

    // rotate to face front of object
    model_matrix.rotateY( -90. );
    model_matrix.rotateZ( -90. );

    vec3d model_angles = model_matrix.getArcballAngles();
    vec3d model_xyz = model_matrix.getTranslation();

    // rotate view to front of selected geom
    m_GlWin->rotateSphere(
        model_angles.x(),
        model_angles.y(),
        model_angles.z()
    );

    // center on geom's xyz
    m_GlWin->setCOR( glm::vec3( model_xyz.x(), model_xyz.y(), model_xyz.z() ) );

    m_GlWin->UpdateAllViewParms();
}

void MainVSPScreen::ResetViews()
{
    std::vector< VSPGraphic::Viewport *> vpts = m_GlWin->getGraphicEngine()->getDisplay()->getLayoutMgr()->getViewports();
    int nview = vpts.size();

    std::vector < VSPGraphic::Common::VSPenum > vOrder;
    // Set View order for viewports.
    // Note:  This order is duplicated from LayoutMgr::LayoutMgr()
    switch ( nview )
    {
        case 1:
            vOrder.push_back( VSPGraphic::Common::VSP_CAM_TOP );
            break;
        case 2:
            vOrder.push_back( VSPGraphic::Common::VSP_CAM_FRONT );
            vOrder.push_back( VSPGraphic::Common::VSP_CAM_TOP );
            break;
        case 4:
        default:
            vOrder.push_back( VSPGraphic::Common::VSP_CAM_LEFT_ISO );
            vOrder.push_back( VSPGraphic::Common::VSP_CAM_TOP );
            vOrder.push_back( VSPGraphic::Common::VSP_CAM_FRONT );
            vOrder.push_back( VSPGraphic::Common::VSP_CAM_LEFT );
            break;
    }

    for ( int i = 0; i < nview; i++ )
    {
        vpts[i]->getCamera()->changeView( vOrder[ i ] );
    }

    m_GlWin->UpdateAllViewParms();
}

void MainVSPScreen::SetWindowLayout( int r, int c )
{
    m_GlWin->setWindowLayout( r, c );
    SetViewAxis( m_ShowXYZArrow );
    SetShowBorders( m_ShowBorder );
}

void MainVSPScreen::ShowReturnToAPIImplementation()
{
    m_StopGUIMenuItem.Show();
#if FL_API_VERSION >= 10400 || (defined(FL_ABI_VERSION) && FL_ABI_VERSION >= 10304) // Before this version, update() was private.
    m_MenuBar->update();
#endif
}

void MainVSPScreen::HideReturnToAPIImplementation()
{
    m_StopGUIMenuItem.Hide();
#if FL_API_VERSION >= 10400 || (defined(FL_ABI_VERSION) && FL_ABI_VERSION >= 10304)
    m_MenuBar->update();
#endif
}

void MainVSPScreen::ShowExitImplementation()
{
    m_ExitMenuItem.Show();
#if FL_API_VERSION >= 10400 || (defined(FL_ABI_VERSION) && FL_ABI_VERSION >= 10304)
    m_MenuBar->update();
#endif
}

void MainVSPScreen::HideExitImplementation()
{
    m_ExitMenuItem.Hide();
#if FL_API_VERSION >= 10400 || (defined(FL_ABI_VERSION) && FL_ABI_VERSION >= 10304)
    m_MenuBar->update();
#endif
}

void ShowReturnToAPIHandler( void * data )
{
    MainVSPScreen * main_scr = (MainVSPScreen*) data;
    if ( main_scr )
    {
        main_scr->ShowReturnToAPIImplementation();
    }
}

void HideReturnToAPIHandler( void * data )
{
    MainVSPScreen * main_scr = (MainVSPScreen*) data;
    if ( main_scr )
    {
        main_scr->HideReturnToAPIImplementation();
    }
}

void ShowExitHandler( void * data )
{
    MainVSPScreen * main_scr = (MainVSPScreen*) data;
    if ( main_scr )
    {
        main_scr->ShowExitImplementation();
    }
}

void HideExitHandler( void * data )
{
    MainVSPScreen * main_scr = (MainVSPScreen*) data;
    if ( main_scr )
    {
        main_scr->HideExitImplementation();
    }
}

void MainVSPScreen::EnableStopGUIMenuItem()
{
    Fl::awake( ShowReturnToAPIHandler, ( void* )this );
}

void MainVSPScreen::DisableStopGUIMenuItem()
{
    Fl::awake( HideReturnToAPIHandler, ( void* )this );
}

void MainVSPScreen::EnableExitMenuItem()
{
    Fl::awake( ShowExitHandler, ( void* )this );
}

void MainVSPScreen::DisableExitMenuItem()
{
    Fl::awake( HideExitHandler, ( void* )this );
}
