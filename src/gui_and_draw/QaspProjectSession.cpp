// This file is released under the NASA Open Source Agreement (NOSA) 1.3.
// See LICENSE. QASP addition; no extra licence or NASA endorsement implied.
#include "QaspProjectSession.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <system_error>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;
namespace
{
struct Operation
{
    bool &flag;
    explicit Operation( bool &value ) : flag( value ) { flag = true; }
    ~Operation() { flag = false; }
};

std::string Utf8( const fs::path &p ) { return p.generic_u8string(); }

fs::path Canonical( const std::string &file )
{
    std::error_code ec;
    fs::path p = fs::absolute( fs::u8path( file ), ec );
    if ( ec ) { return {}; }
    fs::path canonical = fs::weakly_canonical( p, ec );
    return ec ? p.lexically_normal() : canonical;
}

bool SameFile( const std::string &a, const std::string &b )
{
    if ( a.empty() || b.empty() ) { return false; }
    fs::path pa = Canonical( a ), pb = Canonical( b );
    if ( pa.empty() || pb.empty() ) { return false; }
    std::error_code ec;
    if ( fs::equivalent( pa, pb, ec ) && !ec ) { return true; }
#ifdef _WIN32
    return CompareStringOrdinal( pa.c_str(), -1, pb.c_str(), -1, TRUE ) == CSTR_EQUAL;
#else
    return pa == pb;
#endif
}

fs::path UniqueDirectory( const fs::path &parent, const std::string &prefix, std::string &error )
{
    static std::atomic<unsigned long long> serial{ 0 };
    try
    {
        std::random_device random;
        for ( int i = 0; i < 32; ++i )
        {
            std::ostringstream suffix;
            suffix << std::hex << random() << random() << ++serial;
            fs::path p = parent / fs::u8path( prefix + suffix.str() );
            std::error_code ec;
            if ( fs::create_directory( p, ec ) )
            {
#ifndef _WIN32
                fs::permissions( p, fs::perms::owner_all, fs::perm_options::replace, ec );
                if ( ec )
                {
                    fs::remove( p, ec );
                    error = "Cannot make a private working directory.";
                    return {};
                }
#endif
                return p;
            }
            if ( ec && ec != std::errc::file_exists )
            {
                error = "Cannot create working storage: " + ec.message();
                return {};
            }
        }
        error = "Could not allocate a unique working directory.";
    }
    catch ( const std::exception &e ) { error = e.what(); }
    return {};
}

void RemoveTree( const fs::path &path )
{
    if ( !path.empty() )
    {
        std::error_code ec;
        fs::remove_all( path, ec );
    }
}

bool FlushFile( const fs::path &path, std::string &error )
{
#ifdef _WIN32
    HANDLE handle = CreateFileW( path.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
                                nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
    if ( handle == INVALID_HANDLE_VALUE )
    {
        error = "Cannot open the staged file to flush it (Windows error " +
                std::to_string( GetLastError() ) + ").";
        return false;
    }
    bool ok = FlushFileBuffers( handle ) != 0;
    CloseHandle( handle );
#else
    int fd = ::open( path.c_str(), O_RDONLY );
    bool ok = fd >= 0 && ::fsync( fd ) == 0;
    if ( fd >= 0 ) { ::close( fd ); }
#endif
    if ( !ok ) { error = "Could not flush the staged file; the existing file was not replaced."; }
    return ok;
}

std::string Fingerprint( const fs::path &path )
{
    // A change detector, NOT a security/checksum claim. Include length as well.
    std::ifstream in( path, std::ios::binary );
    std::uint64_t hash = UINT64_C( 14695981039346656037 ), count = 0;
    char buffer[16384];
    while ( in )
    {
        in.read( buffer, sizeof buffer );
        std::streamsize n = in.gcount();
        count += static_cast<std::uint64_t>( n );
        for ( std::streamsize i = 0; i < n; ++i )
        {
            hash ^= static_cast<unsigned char>( buffer[i] );
            hash *= UINT64_C( 1099511628211 );
        }
    }
    if ( !in.eof() ) { return {}; }
    std::ostringstream out;
    out << std::hex << hash << ':' << count;
    return out.str();
}

std::string JsonString( const std::string &value )
{
    std::ostringstream out;
    out << '"';
    for ( unsigned char c : value )
    {
        if ( c == '"' || c == '\\' ) { out << '\\' << c; }
        else if ( c < 32 ) { out << "\\u" << std::hex << std::setw( 4 ) << std::setfill( '0' ) << int( c ); }
        else { out << c; }
    }
    out << '"';
    return out.str();
}
}

bool QaspWriteFileAtomically( const fs::path &destination,
    const std::function<bool( const fs::path &, std::string & )> &writer, std::string &error )
{
    error.clear();
    fs::path staging;
    try
    {
        std::error_code ec;
        fs::path target = Canonical( Utf8( destination ) );
        if ( target.empty() || target.filename().empty() ||
             !fs::is_directory( target.parent_path(), ec ) )
        {
            error = "The destination folder does not exist.";
            return false;
        }
        if ( fs::exists( target, ec ) && !fs::is_regular_file( target, ec ) )
        {
            error = "The destination is not a regular file.";
            return false;
        }
        bool existed = fs::exists( target, ec );
        std::string before = existed ? Fingerprint( target ) : std::string();
        fs::perms permissions = existed ? fs::status( target, ec ).permissions() : fs::perms::unknown;
        if ( existed && before.empty() ) { error = "Cannot read the existing destination safely."; return false; }
#ifndef _WIN32
        if ( existed && ::access( target.c_str(), W_OK ) != 0 )
        { error = "The existing destination is not writable. Choose a different filename."; return false; }
#endif
        staging = UniqueDirectory( target.parent_path(), ".qasp-write-", error );
        if ( staging.empty() ) { return false; }
        fs::path file = staging / "project.vsp3";
        if ( !writer( file, error ) || !fs::is_regular_file( file, ec ) ||
             fs::file_size( file, ec ) == 0 || ec || !FlushFile( file, error ) )
        {
            if ( error.empty() ) { error = "Writing the complete project failed."; }
            RemoveTree( staging );
            return false;
        }
        // Do not silently replace an externally edited file during a long write.
        if ( ( existed && Fingerprint( target ) != before ) || ( !existed && fs::exists( target, ec ) ) )
        {
            error = "The destination changed while saving. It was not replaced; choose a different filename.";
            RemoveTree( staging ); return false;
        }
#ifdef _WIN32
        // ReplaceFile preserves the existing file's security descriptor. A new
        // file uses a non-replacing move, so a newly appeared file is not clobbered.
        bool replaced = existed ? ReplaceFileW( target.c_str(), file.c_str(), nullptr, 0, nullptr, nullptr ) != 0 :
                                  MoveFileExW( file.c_str(), target.c_str(), MOVEFILE_WRITE_THROUGH ) != 0;
        if ( !replaced )
        {
            error = "Cannot replace the destination (Windows error " + std::to_string( GetLastError() ) +
                    "). The previous file was left in place.";
            RemoveTree( staging );
            return false;
        }
#else
        if ( existed && permissions != fs::perms::unknown )
        {
            fs::permissions( file, permissions, fs::perm_options::replace, ec );
            if ( ec ) { error = "Cannot preserve destination permissions: " + ec.message(); RemoveTree( staging ); return false; }
        }
        fs::rename( file, target, ec );
        if ( ec )
        {
            error = "Cannot replace the destination: " + ec.message();
            RemoveTree( staging );
            return false;
        }
        // Best-effort directory durability; filesystem/hardware semantics vary.
        int dir = ::open( target.parent_path().c_str(), O_RDONLY | O_DIRECTORY );
        if ( dir >= 0 ) { ::fsync( dir ); ::close( dir ); }
#endif
        RemoveTree( staging );
        return true;
    }
    catch ( const std::exception &e )
    {
        error = std::string( "Project write failed: " ) + e.what();
        RemoveTree( staging );
        return false;
    }
}

QaspProjectSession::QaspProjectSession( QaspProjectHost &host, const fs::path &temporaryParent ) : m_Host( host )
{
    try
    {
        fs::path parent = temporaryParent.empty() ? fs::temp_directory_path() : temporaryParent;
        m_Root = UniqueDirectory( parent, "qasp-session-", m_StorageError );
    }
    catch ( const std::exception &e ) { m_StorageError = e.what(); }
}

bool QaspProjectSession::Ready( std::string &error )
{
    error.clear();
    m_Warning.clear();
    if ( m_Operating ) { error = "A project operation is already in progress."; return false; }
    if ( m_RecoveryRequired )
    {
        error = "The last project could not be restored. Do not overwrite an original file. "
                "Working copies remain in: " + Utf8( m_Root );
        return false;
    }
    if ( m_Host.Busy( error ) )
    {
        if ( error.empty() ) { error = "Wait for the running solver or mesh operation to finish."; }
        return false;
    }
    return true;
}

bool QaspProjectSession::PrepareProject( QaspProject &p, const std::string &file, std::string &error )
{
    if ( m_Root.empty() ) { error = "Project working storage is unavailable: " + m_StorageError; return false; }
    p.id = m_NextID++;
    fs::path dir = m_Root / ( "project-" + std::to_string( p.id ) );
    std::error_code ec;
    if ( !fs::create_directory( dir, ec ) || ec )
    {
        error = "Cannot allocate a project working copy: " + ec.message();
        return false;
    }
    p.snapshot = dir / "snapshot.vsp3";
    p.file = file.empty() ? "" : Utf8( Canonical( file ) );
    p.title = p.file.empty() ? "Untitled " + std::to_string( m_NextUntitled++ ) + ".vsp3" :
                              Utf8( fs::u8path( p.file ).filename() );
    p.logicalFile = p.file.empty() ? Utf8( dir / fs::u8path( p.title ) ) : p.file;
    return true;
}

bool QaspProjectSession::Capture( QaspProject &p, std::string &error )
{
    p.view = m_Host.CaptureView();
    bool ok = QaspWriteFileAtomically( p.snapshot, [&]( const fs::path &file, std::string &err )
    {
        return m_Host.Write( file, p.logicalFile, err ) && m_Host.Validate( file, err );
    }, error );
    if ( !ok ) { return false; }
    std::string hash = Fingerprint( p.snapshot );
    if ( hash.empty() ) { error = "Cannot read the project working copy after writing it."; return false; }
    if ( !p.cleanFingerprint.empty() ) { p.dirty = hash != p.cleanFingerprint; }
    else { p.dirty = true; } // No known baseline: be conservative, never silently drop work.
    p.possiblyChanged = false;
    return true;
}

bool QaspProjectSession::Adopt( const std::string &file, std::string &error, bool initiallyDirty )
{
    if ( !m_Projects.empty() ) { error = "A project is already adopted."; return false; }
    QaspProject p;
    if ( !PrepareProject( p, file, error ) )
    {
        // Saving an already active model must remain possible even if temp storage fails.
        p.id = m_NextID++;
        p.file = file;
        p.logicalFile = file.empty() ? "Unnamed.vsp3" : file;
        p.title = file.empty() ? "Untitled.vsp3" : Utf8( fs::u8path( file ).filename() );
        p.dirty = true;
        m_Projects.push_back( p );
        return false;
    }
    m_Host.SetFileName( p.logicalFile );
    bool ok = Capture( p, error );
    if ( ok && !initiallyDirty ) { p.cleanFingerprint = Fingerprint( p.snapshot ); p.dirty = false; }
    else { p.dirty = true; }
    if ( !file.empty() ) { p.diskFingerprint = Fingerprint( fs::u8path( p.file ) ); }
    m_Projects.push_back( p );
    Manifest();
    return ok;
}

bool QaspProjectSession::Checkpoint( std::string &error )
{
    if ( !Ready( error ) || m_Projects.empty() ) { return false; }
    Operation operation( m_Operating );
    if ( !Capture( m_Projects[m_Active], error ) ) { return false; }
    Manifest();
    return true;
}

bool QaspProjectSession::Activate( QaspProject &p, std::string &error )
{
    if ( !m_Host.Validate( p.snapshot, error ) || !m_Host.Load( p.snapshot, p.logicalFile, error ) ) { return false; }
    m_Host.RestoreView( p.view );
    // Object IDs can be remapped by the engine on reload. Establish a new clean
    // baseline ONLY for a project that was already known clean; never erase dirt.
    bool wasDirty = p.dirty;
    p.cleanFingerprint.clear();
    if ( !Capture( p, error ) )
    {
        m_Warning = "The model was loaded, but its working copy could not be refreshed: " + error;
        p.dirty = true;
        error.clear();
        return true;
    }
    p.dirty = wasDirty;
    if ( !wasDirty ) { p.cleanFingerprint = Fingerprint( p.snapshot ); }
    return true;
}

bool QaspProjectSession::RollBack( QaspProject &previous, const std::string &failure, std::string &error )
{
    std::string restoreError;
    bool restored = m_Host.Validate( previous.snapshot, restoreError ) &&
                    m_Host.Load( previous.snapshot, previous.logicalFile, restoreError );
    if ( restored ) { m_Host.RestoreView( previous.view ); }
    if ( !restored )
    {
        m_RecoveryRequired = true;
        error = failure + "\nThe previous model could not be restored: " + restoreError +
                "\nDo not overwrite an original. Working copies: " + Utf8( m_Root );
    }
    else
    {
        previous.cleanFingerprint.clear();
        previous.dirty = true; // A remapped reload without a fresh baseline is conservatively unsaved.
        error = failure + "\nYour previous model has been restored.";
    }
    Manifest();
    return false;
}

bool QaspProjectSession::New( std::string &error )
{
    if ( !Ready( error ) || m_Projects.empty() ) { return false; }
    Operation operation( m_Operating );
    QaspProject p;
    if ( !PrepareProject( p, "", error ) ) { return false; }
    QaspProject &previous = m_Projects[m_Active];
    if ( !Capture( previous, error ) ) { RemoveTree( p.snapshot.parent_path() ); return false; }
    if ( !m_Host.Empty( p.logicalFile, error ) || !Capture( p, error ) )
    {
        std::string failure = error;
        RemoveTree( p.snapshot.parent_path() );
        return RollBack( previous, failure, error );
    }
    p.cleanFingerprint = Fingerprint( p.snapshot );
    p.dirty = false;
    m_Projects.push_back( p );
    m_Active = m_Projects.size() - 1;
    Manifest();
    return true;
}

bool QaspProjectSession::Open( const std::string &file, std::string &error )
{
    if ( !Ready( error ) || m_Projects.empty() ) { return false; }
    for ( std::size_t i = 0; i < m_Projects.size(); ++i )
    {
        if ( SameFile( file, m_Projects[i].file ) ) { return Switch( i, error ); }
    }
    Operation operation( m_Operating );
    fs::path input = Canonical( file );
    if ( input.empty() || !m_Host.Validate( input, error ) ) { return false; }
    QaspProject p;
    if ( !PrepareProject( p, Utf8( input ), error ) ) { return false; }
    if ( !QaspWriteFileAtomically( p.snapshot, [&]( const fs::path &stage, std::string &err )
    {
        std::error_code ec;
        fs::copy_file( input, stage, fs::copy_options::none, ec );
        if ( ec ) { err = "Cannot make a private copy of the opened file: " + ec.message(); return false; }
        return m_Host.Validate( stage, err );
    }, error ) ) { RemoveTree( p.snapshot.parent_path() ); return false; }
    p.diskFingerprint = Fingerprint( p.snapshot );
    QaspProject &previous = m_Projects[m_Active];
    if ( !Capture( previous, error ) ) { RemoveTree( p.snapshot.parent_path() ); return false; }
    if ( !m_Host.Load( p.snapshot, p.logicalFile, error ) || !Capture( p, error ) )
    {
        std::string failure = error;
        RemoveTree( p.snapshot.parent_path() );
        return RollBack( previous, failure, error );
    }
    p.cleanFingerprint = Fingerprint( p.snapshot );
    p.dirty = false;
    m_Projects.push_back( p );
    m_Active = m_Projects.size() - 1;
    Manifest();
    return true;
}

bool QaspProjectSession::Switch( std::size_t index, std::string &error )
{
    if ( !Ready( error ) || index >= m_Projects.size() ) { return false; }
    if ( index == m_Active ) { return true; }
    Operation operation( m_Operating );
    // Reject a missing/corrupt target BEFORE touching the current model.
    if ( !m_Host.Validate( m_Projects[index].snapshot, error ) ||
         !Capture( m_Projects[m_Active], error ) ) { return false; }
    if ( !Activate( m_Projects[index], error ) )
    {
        std::string failure = error;
        return RollBack( m_Projects[m_Active], failure, error );
    }
    m_Active = index;
    Manifest();
    return true;
}

bool QaspProjectSession::Save( const std::string &file, std::string &error )
{
    if ( !Ready( error ) || file.empty() || m_Projects.empty() ) { return false; }
    Operation operation( m_Operating );
    fs::path target = Canonical( file );
    if ( target.empty() ) { error = "Invalid save filename."; return false; }
    for ( std::size_t i = 0; i < m_Projects.size(); ++i )
    {
        if ( i != m_Active && SameFile( Utf8( target ), m_Projects[i].file ) )
        {
            error = "That file is already open in another tab. Choose a different filename.";
            return false;
        }
    }
    QaspProject &p = m_Projects[m_Active];
    std::error_code diskError;
    if ( SameFile( p.file, Utf8( target ) ) && !p.diskFingerprint.empty() && fs::exists( target, diskError ) &&
         Fingerprint( target ) != p.diskFingerprint )
    {
        error = "The project file changed on disk since it was opened or saved. Save As a different filename to keep both versions.";
        return false;
    }
    std::string savedFingerprint;
    if ( !QaspWriteFileAtomically( target, [&]( const fs::path &stage, std::string &err )
    {
        if ( !m_Host.Write( stage, Utf8( target ), err ) || !m_Host.Validate( stage, err ) ) { return false; }
        savedFingerprint = Fingerprint( stage );
        return !savedFingerprint.empty();
    }, error ) ) { return false; }
    p.diskFingerprint = savedFingerprint;
    // Do not mark clean or rename the tab until the destination was replaced.
    p.file = Utf8( target );
    p.logicalFile = p.file;
    p.title = Utf8( target.filename() );
    m_Host.SetFileName( p.file );
    p.dirty = false;
    p.possiblyChanged = false;
    p.cleanFingerprint.clear();
    std::string cacheError;
    if ( !p.snapshot.empty() && Capture( p, cacheError ) )
    {
        p.cleanFingerprint = Fingerprint( p.snapshot );
        p.dirty = false;
    }
    else
    {
        p.cleanFingerprint = p.diskFingerprint;
        m_Warning = "The project file was saved, but its tab working copy could not be refreshed. "
                    "Do not switch until working storage is available. " + cacheError;
    }
    Manifest();
    return true;
}

bool QaspProjectSession::CloseActive( std::string &error )
{
    if ( !Ready( error ) || m_Projects.empty() ) { return false; }
    std::size_t closing = m_Active;
    fs::path oldDirectory = m_Projects[closing].snapshot.parent_path();
    if ( m_Projects.size() == 1 )
    {
        if ( !New( error ) ) { return false; }
    }
    else if ( !Switch( closing ? closing - 1 : 1, error ) ) { return false; }
    m_Projects.erase( m_Projects.begin() + static_cast<std::ptrdiff_t>( closing ) );
    if ( m_Active > closing ) { --m_Active; }
    RemoveTree( oldDirectory );
    Manifest();
    return true;
}

bool QaspProjectSession::DiscardInactive( std::size_t index, std::string &error )
{
    if ( !Ready( error ) ) { return false; }
    if ( index >= m_Projects.size() || index == m_Active )
    { error = "Only an inactive tab can be discarded without reloading the engine."; return false; }
    fs::path directory = m_Projects[index].snapshot.parent_path();
    m_Projects.erase( m_Projects.begin() + static_cast<std::ptrdiff_t>( index ) );
    if ( m_Active > index ) { --m_Active; }
    RemoveTree( directory );
    Manifest();
    return true;
}

bool QaspProjectSession::IsOpenFile( const std::string &file, bool exceptActive ) const
{
    for ( std::size_t i = 0; i < m_Projects.size(); ++i )
    {
        if ( ( !exceptActive || i != m_Active ) && SameFile( file, m_Projects[i].file ) ) { return true; }
    }
    return false;
}

void QaspProjectSession::MarkPossiblyChanged()
{
    if ( !m_Projects.empty() && !m_Operating ) { m_Projects[m_Active].possiblyChanged = true; }
}

void QaspProjectSession::Manifest()
{
    if ( m_Root.empty() ) { return; }
    std::string ignored;
    QaspWriteFileAtomically( m_Root / "session.json", [&]( const fs::path &stage, std::string & )
    {
        std::ofstream out( stage, std::ios::binary );
        out << "{\"format\":1,\"note\":\"Working snapshots, not guaranteed recovery or self-contained backups.\","
               "\"active\":" << m_Active << ",\"projects\":[";
        for ( std::size_t i = 0; i < m_Projects.size(); ++i )
        {
            const QaspProject &p = m_Projects[i];
            if ( i ) { out << ','; }
            out << "{\"title\":" << JsonString( p.title ) << ",\"file\":" << JsonString( p.file )
                << ",\"resourceBase\":" << JsonString( p.logicalFile )
                << ",\"snapshot\":" << JsonString( Utf8( p.snapshot ) )
                << ",\"dirty\":" << ( p.dirty ? "true" : "false" ) << '}';
        }
        out << "]}\n";
        out.close();
        return !out.fail();
    }, ignored );
}

void QaspProjectSession::CleanExit() { RemoveTree( m_Root ); }
