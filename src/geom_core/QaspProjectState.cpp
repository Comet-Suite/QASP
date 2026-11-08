// NOSA 1.3; see LICENSE. QASP addition for actual per-project setup preservation.
#if defined(_WIN32) || defined(WIN32)
#undef _HAS_STD_BYTE
#define _HAS_STD_BYTE 0
#endif
#include "QaspProjectState.h"
#include "DesignVarMgr.h"
#include "FitModelMgr.h"
#include "IDMgr.h"
#include "ParmMgr.h"
#include "VehicleMgr.h"
#include "Vehicle.h"
#include <cmath>
#include <memory>

namespace
{
string ParmReference( const string &oldID )
{
    string remapped = IDMgr.RemapRefID( oldID );
    if ( ParmMgr.FindParm( remapped ) ) { return remapped; }
    return ParmMgr.FindParm( oldID ) ? oldID : string();
}
}

void QaspEncodeProjectState( xmlNodePtr &root )
{
    xmlNodePtr state = xmlNewChild( root, nullptr, BAD_CAST "QaspProjectSetup", nullptr );
    if ( !state ) { return; }
    XmlUtil::AddIntNode( state, "Version", 1 );
    xmlNodePtr design = xmlNewChild( state, nullptr, BAD_CAST "DesignVariables", nullptr );
    if ( design )
    {
        XmlUtil::AddIntNode( design, "Selected", DesignVarMgr.GetCurrVarIndex() );
        for ( unsigned int i = 0; i < DesignVarMgr.GetNumVars(); ++i )
        {
            DesignVar *variable = DesignVarMgr.GetVar( i );
            if ( !variable ) { continue; }
            xmlNodePtr node = xmlNewChild( design, nullptr, BAD_CAST "Variable", nullptr );
            if ( !node ) { continue; }
            XmlUtil::AddStringNode( node, "ParmID", variable->m_ParmID );
            XmlUtil::AddIntNode( node, "Type", variable->m_XDDM_Type );
            XmlUtil::AddDoubleNode( node, "Lower", variable->m_LowerLimit );
            XmlUtil::AddDoubleNode( node, "Upper", variable->m_UpperLimit );
        }
    }
    xmlNodePtr fit = xmlNewChild( state, nullptr, BAD_CAST "FitModel", nullptr );
    if ( fit )
    {
        XmlUtil::AddIntNode( fit, "SelectedVariable", FitModelMgr.GetCurrVarIndex() );
        XmlUtil::AddIntNode( fit, "SelectedPoint", FitModelMgr.GetCurrTargetPtIndex() );
        XmlUtil::AddStringNode( fit, "SaveFile", FitModelMgr.GetSaveFitFileName() );
        XmlUtil::AddStringNode( fit, "LoadFile", FitModelMgr.GetLoadFitFileName() );
        for ( int i = 0; i < FitModelMgr.GetNumVars(); ++i )
        {
            xmlNodePtr node = xmlNewChild( fit, nullptr, BAD_CAST "Variable", nullptr );
            if ( node ) { XmlUtil::AddStringNode( node, "ParmID", FitModelMgr.GetVar( i ) ); }
        }
        for ( int i = 0; i < FitModelMgr.GetNumTargetPt(); ++i )
        {
            if ( TargetPt *point = FitModelMgr.GetTargetPt( i ) ) { point->WrapXml( fit ); }
        }
    }
}

void QaspDecodeProjectState( xmlNodePtr &root )
{
    xmlNodePtr state = XmlUtil::GetNode( root, "QaspProjectSetup", 0 );
    if ( !state || XmlUtil::FindInt( state, "Version", 0 ) != 1 ) { return; }
    DesignVarMgr.DelAllVars();
    FitModelMgr.DelAllVars(); FitModelMgr.DelAllTargetPts();
    xmlNodePtr design = XmlUtil::GetNode( state, "DesignVariables", 0 );
    if ( design )
    {
        vector<xmlNodePtr> variables = XmlUtil::GetNodes( design, "Variable" );
        for ( xmlNodePtr node : variables )
        {
            string id = ParmReference( XmlUtil::FindString( node, "ParmID", "" ) );
            int type = XmlUtil::FindInt( node, "Type", vsp::XDDM_VAR );
            double lower = XmlUtil::FindDouble( node, "Lower", 0 );
            double upper = XmlUtil::FindDouble( node, "Upper", 0 );
            if ( !id.empty() && ( type == vsp::XDDM_VAR || type == vsp::XDDM_CONST ) &&
                 std::isfinite( lower ) && std::isfinite( upper ) && lower <= upper )
            { DesignVarMgr.AddVar( id, type, lower, upper ); }
        }
        int index = XmlUtil::FindInt( design, "Selected", -1 );
        DesignVarMgr.SetCurrVarIndex( index >= 0 && index < static_cast<int>( DesignVarMgr.GetNumVars() ) ? index : -1 );
    }
    xmlNodePtr fit = XmlUtil::GetNode( state, "FitModel", 0 );
    if ( fit )
    {
        FitModelMgr.SetSaveFitFileName( XmlUtil::FindString( fit, "SaveFile", "DefaultFitModel.fit" ) );
        FitModelMgr.SetLoadFitFileName( XmlUtil::FindString( fit, "LoadFile", "" ) );
        for ( xmlNodePtr node : XmlUtil::GetNodes( fit, "Variable" ) )
        {
            string id = ParmReference( XmlUtil::FindString( node, "ParmID", "" ) );
            if ( !id.empty() ) { FitModelMgr.AddVar( id ); }
        }
        for ( xmlNodePtr node : XmlUtil::GetNodes( fit, "TargetPt" ) )
        {
            std::unique_ptr<TargetPt> point( new TargetPt );
            point->UnwrapXml( node );
            point->SetMatchGeom( IDMgr.RemapRefID( point->GetMatchGeom() ) );
            vec3d p = point->GetPt(); vec2d uv = point->GetUW();
            bool types = ( point->GetUType() == vsp::FIT_MODEL_FIXED || point->GetUType() == vsp::FIT_MODEL_FREE ) &&
                         ( point->GetWType() == vsp::FIT_MODEL_FIXED || point->GetWType() == vsp::FIT_MODEL_FREE );
            if ( types && std::isfinite( p.x() ) && std::isfinite( p.y() ) && std::isfinite( p.z() ) &&
                 std::isfinite( uv.x() ) && std::isfinite( uv.y() ) && point->IsValid() )
            { point->UpdateDist(); FitModelMgr.AddTargetPt( point.release() ); }
        }
        int variable = XmlUtil::FindInt( fit, "SelectedVariable", -1 );
        int point = XmlUtil::FindInt( fit, "SelectedPoint", -1 );
        FitModelMgr.SetCurrVarIndex( variable >= 0 && variable < FitModelMgr.GetNumVars() ? variable : -1 );
        FitModelMgr.SetCurrTargetPtIndex( point >= 0 && point < FitModelMgr.GetNumTargetPt() ? point : -1 );
    }
}
