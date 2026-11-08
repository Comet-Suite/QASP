// This file is released under the NASA Open Source Agreement (NOSA) 1.3.
// See LICENSE. QASP addition: single-engine, transactional project tabs.
#ifndef QASP_PROJECT_SESSION_H
#define QASP_PROJECT_SESSION_H

#include <cstddef>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

// The coordinator knows nothing about FLTK or Vehicle's global managers. Its
// host MUST refuse concurrent engine access and validate before destructive load.
class QaspProjectHost
{
public:
    virtual ~QaspProjectHost() = default;
    virtual bool Busy( std::string &reason ) = 0;
    virtual bool Validate( const std::filesystem::path &file, std::string &error ) = 0;
    virtual bool Write( const std::filesystem::path &file, const std::string &logicalFile,
                        std::string &error ) = 0;
    virtual bool Load( const std::filesystem::path &file, const std::string &logicalFile,
                       std::string &error ) = 0;
    virtual bool Empty( const std::string &logicalFile, std::string &error ) = 0;
    virtual void SetFileName( const std::string &file ) = 0;
    virtual std::string CaptureView() = 0;
    virtual void RestoreView( const std::string &view ) = 0;
};

struct QaspProject
{
    unsigned long long id = 0;
    std::string title;
    std::string file;          // Empty means not explicitly saved by the user.
    std::string logicalFile;   // Original resource/output base, NOT the snapshot path.
    std::filesystem::path snapshot;
    std::string view;
    std::string cleanFingerprint;
    std::string diskFingerprint;
    bool dirty = false;
    bool possiblyChanged = false;
};

// Writes beside the destination and replaces it only after the writer succeeds.
// Failure before replacement leaves an existing destination untouched. This is
// NOT a cross-filesystem transaction or a universal power-failure guarantee.
bool QaspWriteFileAtomically( const std::filesystem::path &destination,
    const std::function<bool( const std::filesystem::path &, std::string & )> &writer,
    std::string &error );

class QaspProjectSession
{
public:
    explicit QaspProjectSession( QaspProjectHost &host,
                                const std::filesystem::path &temporaryParent = {} );
    // Do not automatically delete working copies on an abnormal or API shutdown.
    ~QaspProjectSession() = default;

    bool Adopt( const std::string &file, std::string &error, bool initiallyDirty = false );
    bool New( std::string &error );
    bool Open( const std::string &file, std::string &error );
    bool Switch( std::size_t index, std::string &error );
    // Caller must obtain Save/Discard/Cancel permission before closing dirty work.
    bool CloseActive( std::string &error );
    bool DiscardInactive( std::size_t index, std::string &error );
    bool Save( const std::string &file, std::string &error );
    bool Checkpoint( std::string &error );
    void MarkPossiblyChanged();
    void CleanExit(); // Only after ALL user confirmations, never on Cancel/error.

    const std::vector<QaspProject> &Projects() const { return m_Projects; }
    std::size_t ActiveIndex() const { return m_Active; }
    const QaspProject &Active() const { return m_Projects.at( m_Active ); }
    const std::filesystem::path &WorkingDirectory() const { return m_Root; }
    bool IsOpenFile( const std::string &file, bool exceptActive = false ) const;
    bool RecoveryRequired() const { return m_RecoveryRequired; }
    bool IsOperating() const { return m_Operating; }
    const std::string &Warning() const { return m_Warning; }

private:
    bool Ready( std::string &error );
    bool PrepareProject( QaspProject &project, const std::string &file, std::string &error );
    bool Capture( QaspProject &project, std::string &error );
    bool Activate( QaspProject &project, std::string &error );
    bool RollBack( QaspProject &previous, const std::string &failure, std::string &error );
    void Manifest();
    QaspProjectHost &m_Host;
    std::vector<QaspProject> m_Projects;
    std::size_t m_Active = 0;
    unsigned long long m_NextID = 1;
    unsigned long long m_NextUntitled = 1;
    std::filesystem::path m_Root;
    std::string m_StorageError;
    std::string m_Warning;
    bool m_RecoveryRequired = false;
    bool m_Operating = false;
};
#endif
