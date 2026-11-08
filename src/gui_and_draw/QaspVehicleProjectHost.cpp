// NOSA 1.3; see LICENSE. QASP project coordinator's real engine adapter.
#ifdef WIN32
#undef _HAS_STD_BYTE
#define _HAS_STD_BYTE 0
#endif
#include "QaspVehicleProjectHost.h"
#include "ScreenMgr.h"
#include "MainVSPScreen.h"
#include "Vehicle.h"
#include "ParmMgr.h"
#include "MaterialMgr.h"
#include "AeroStructMgr.h"
#include "CfdMeshMgr.h"
#include "FeaMeshMgr.h"
#include "SurfaceIntersectionMgr.h"
#include "VSPAEROMgr.h"
#include "VSPAEROScreen.h"
#include "AeroStructScreen.h"
#include "StructScreen.h"
#include "StructAssemblyScreen.h"
#include "CfdMeshScreen.h"
#include "SurfaceIntersectionScreen.h"
#include "XmlUtil.h"
#include <libxml/parser.h>
#include <exception>

namespace fs = std::filesystem;
namespace
{
bool Running( ProcessUtil *process ) { return process && process->IsRunning(); }
}

bool QaspVehicleProjectHost::Busy( std::string &reason )
{
    bool running = VSPAEROMgr.m_SolverThreadActive.load() || CfdMeshMgr.GetMeshInProgress() ||
                   FeaMeshMgr.GetFeaMeshInProgress() || SurfaceIntersectionMgr.GetMeshInProgress();
    running = running || Running( VSPAEROMgr.GetSolverProcess() ) || Running( VSPAEROMgr.GetSlicerProcess() );
    if ( auto *screen = dynamic_cast<VSPAEROScreen *>( m_Screens.GetScreen( vsp::VSP_VSPAERO_SCREEN ) ) )
    {
        running = running || Running( screen->GetProcess( VSPAERO_SOLVER ) ) ||
                  Running( screen->GetProcess( VSPAERO_VIEWER ) );
    }
    if ( auto *screen = dynamic_cast<AeroStructScreen *>( m_Screens.GetScreen( vsp::VSP_AERO_STRUCT_SCREEN ) ) )
    { running = running || Running( screen->GetProcess() ); }
    if ( auto *screen = dynamic_cast<StructScreen *>( m_Screens.GetScreen( vsp::VSP_STRUCT_SCREEN ) ) )
    { running = running || Running( screen->getFeaMeshProcess() ); }
    if ( auto *screen = dynamic_cast<StructAssemblyScreen *>( m_Screens.GetScreen( vsp::VSP_STRUCT_ASSEMBLY_SCREEN ) ) )
    { running = running || Running( screen->getFeaMeshProcess() ); }
    if ( auto *screen = dynamic_cast<CfdMeshScreen *>( m_Screens.GetScreen( vsp::VSP_CFD_MESH_SCREEN ) ) )
    { running = running || Running( screen->getCfdMeshProcess() ); }
    if ( auto *screen = dynamic_cast<SurfaceIntersectionScreen *>( m_Screens.GetScreen( vsp::VSP_SURFACE_INTERSECTION_SCREEN ) ) )
    { running = running || Running( screen->getIntersectProcess() ); }
    if ( running )
    {
        reason = "Finish or stop the running solver, mesher or external viewer before changing or saving projects.";
    }
    return running;
}

bool QaspVehicleProjectHost::Validate( const fs::path &file, std::string &error )
{
    std::error_code ec;
    if ( !fs::is_regular_file( file, ec ) || ec )
    { error = "The project is not a readable regular file."; return false; }
    xmlParserCtxtPtr context = xmlNewParserCtxt();
    if ( !context ) { error = "Not enough memory to validate the project XML."; return false; }
    // No recovery parser, external DTD loading or network retrieval. This is
    // structural preflight, NOT an audit of every model value or custom script.
    xmlDocPtr doc = xmlCtxtReadFile( context, file.generic_u8string().c_str(), nullptr,
                                   XML_PARSE_NONET | XML_PARSE_NOERROR | XML_PARSE_NOWARNING );
    if ( !doc )
    {
        error = "The project XML is incomplete, invalid or exceeds the parser's supported limits.";
        xmlFreeParserCtxt( context ); return false;
    }
    xmlNodePtr root = xmlDocGetRootElement( doc );
    bool valid = !doc->intSubset && !doc->extSubset && root &&
                 xmlStrEqual( root->name, BAD_CAST "Vsp_Geometry" ) && XmlUtil::GetNode( root, "Vehicle", 0 );
    int version = valid ? XmlUtil::FindInt( root, "Version", 0 ) : 0;
    valid = valid && version >= MIN_FILE_VER && version <= CURRENT_FILE_VER;
    xmlNodePtr setup = root ? XmlUtil::GetNode( root, "QaspProjectSetup", 0 ) : nullptr;
    if ( setup && XmlUtil::FindInt( setup, "Version", 0 ) != 1 ) { valid = false; }
    if ( !valid )
    {
        error = "Not a supported complete .vsp3 project. External/internal DTDs and newer or obsolete file versions are not accepted.";
    }
    xmlFreeDoc( doc ); xmlFreeParserCtxt( context );
    return valid;
}

bool QaspVehicleProjectHost::Write( const fs::path &file, const std::string &logicalFile, std::string &error )
{
    try
    {
        if ( m_Screens.GetVehiclePtr()->WriteXMLFileWithBase( file.generic_u8string(), logicalFile, vsp::SET_ALL ) )
        { return true; }
        error = "Writing the complete .vsp3 project failed. Check free space and folder permissions.";
    }
    catch ( const std::exception &e ) { error = std::string( "Project serialization failed: " ) + e.what(); }
    catch ( ... ) { error = "Project serialization failed with an unexpected engine error."; }
    return false;
}

void QaspVehicleProjectHost::ResetTransientState()
{
    // Vehicle::Renew resets geometry, serialized managers, results and meshers.
    // These two global libraries also need explicit project boundaries.
    MaterialMgr.ClearUserMaterials();
    AeroStructMgr.ResetToInitVals();
    AeroStructMgr.m_ADBFileFound = AeroStructMgr.m_FEAMeshFileFound = false;
    AeroStructMgr.m_FEAInputFileFound = AeroStructMgr.m_FEASolutionFileFound = false;
    AeroStructMgr.m_ADBFile.clear(); AeroStructMgr.m_FEAMeshFile.clear();
    AeroStructMgr.m_FEAInputFile.clear(); AeroStructMgr.m_FEASolutionFile.clear();
    AeroStructMgr.m_StructAssyFlagVec.clear(); AeroStructMgr.m_StructAssyIDVec.clear();
    ParmMgr.ClearUndo(); ParmMgr.SetActiveParm( "" );
}

bool QaspVehicleProjectHost::Empty( const std::string &logicalFile, std::string &error )
{
    try
    {
        m_Main.PrepareProjectChange();
        m_Screens.GetVehiclePtr()->Renew(); ResetTransientState();
        SetFileName( logicalFile );
        m_Main.ResetViews();
        return true;
    }
    catch ( const std::exception &e ) { error = std::string( "Cannot initialise a new project: " ) + e.what(); }
    catch ( ... ) { error = "Cannot initialise a new project: unexpected engine error."; }
    return false;
}

bool QaspVehicleProjectHost::Load( const fs::path &file, const std::string &logicalFile, std::string &error )
{
    if ( !Validate( file, error ) ) { return false; }
    try
    {
        m_Main.PrepareProjectChange();
        Vehicle *vehicle = m_Screens.GetVehiclePtr();
        vehicle->Renew(); ResetTransientState();
        // Decode relative assets using the source document's directory, even
        // when the bytes being read are in a private tab working-copy folder.
        SetFileName( logicalFile );
        int result = vehicle->ReadXMLFile( file.generic_u8string() );
        if ( result != 0 )
        { error = "The engine could not load the project (code " + std::to_string( result ) + ")."; return false; }
        ParmMgr.ClearUndo(); ParmMgr.SetActiveParm( "" );
        m_Main.ResetViews(); m_Main.FitView( true );
        vehicle->m_ViewDirty = false;
        return true;
    }
    catch ( const std::exception &e ) { error = std::string( "Project decode failed: " ) + e.what(); }
    catch ( ... ) { error = "Project decode failed with an unexpected engine error."; }
    return false;
}
void QaspVehicleProjectHost::SetFileName( const std::string &file )
{
    if ( m_Screens.GetVehiclePtr()->GetVSP3FileName() != file )
    { m_Screens.GetVehiclePtr()->SetVSP3FileName( file ); }
}
std::string QaspVehicleProjectHost::CaptureView() { return m_Main.CaptureProjectView(); }
void QaspVehicleProjectHost::RestoreView( const std::string &view ) { m_Main.RestoreProjectView( view ); }
