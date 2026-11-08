// NOSA 1.3; see LICENSE. QASP bridge to the existing single Vehicle engine.
#ifndef QASP_VEHICLE_PROJECT_HOST_H
#define QASP_VEHICLE_PROJECT_HOST_H
#include "QaspProjectSession.h"
class ScreenMgr;
class MainVSPScreen;
class QaspVehicleProjectHost : public QaspProjectHost
{
public:
    QaspVehicleProjectHost( ScreenMgr &screens, MainVSPScreen &main ) : m_Screens( screens ), m_Main( main ) {}
    bool Busy( std::string &reason ) override;
    bool Validate( const std::filesystem::path &file, std::string &error ) override;
    bool Write( const std::filesystem::path &file, const std::string &logicalFile, std::string &error ) override;
    bool Load( const std::filesystem::path &file, const std::string &logicalFile, std::string &error ) override;
    bool Empty( const std::string &logicalFile, std::string &error ) override;
    void SetFileName( const std::string &file ) override;
    std::string CaptureView() override;
    void RestoreView( const std::string &view ) override;
private:
    ScreenMgr &m_Screens;
    MainVSPScreen &m_Main;
    void ResetTransientState();
};
#endif
