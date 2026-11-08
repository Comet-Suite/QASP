// NOSA 1.3. Focused state/filesystem tests; this is not the aircraft engine.
#include "QaspProjectSession.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;
int checks = 0;
void Check( bool condition, const std::string &message )
{
    ++checks;
    if ( !condition ) { throw std::runtime_error( message ); }
}
std::string Read( const fs::path &path )
{
    std::ifstream in( path, std::ios::binary );
    return std::string( std::istreambuf_iterator<char>( in ), {} );
}
void Put( const fs::path &path, const std::string &text )
{
    std::ofstream out( path, std::ios::binary );
    out << text;
}

class FakeHost : public QaspProjectHost
{
public:
    std::string model = "empty", filename, writeBase, loadBase, camera = "view 1";
    bool busy = false, failWrite = false, failLoadOnce = false, failLoadAlways = false, failEmpty = false;
    int loads = 0, writes = 0, failWritesFrom = -1;
    bool Busy( std::string &error ) override
    {
        if ( busy ) { error = "Solver running"; }
        return busy;
    }
    bool Validate( const fs::path &file, std::string &error ) override
    {
        std::string data = Read( file );
        if ( data.rfind( "VSP\n", 0 ) != 0 ) { error = "Invalid model"; return false; }
        return true;
    }
    bool Write( const fs::path &file, const std::string &logical, std::string &error ) override
    {
        ++writes;
        writeBase = logical;
        if ( failWrite || ( failWritesFrom > 0 && writes >= failWritesFrom ) ) { Put( file, "partial" ); error = "Disk full"; return false; }
        Put( file, "VSP\n" + model );
        return true;
    }
    bool Load( const fs::path &file, const std::string &logical, std::string &error ) override
    {
        ++loads;
        model = "PARTIALLY DECODED";
        if ( failLoadOnce || failLoadAlways )
        {
            failLoadOnce = false;
            error = "Decoder failed";
            return false;
        }
        std::string data = Read( file );
        if ( data.rfind( "VSP\n", 0 ) != 0 ) { error = "Bad input"; return false; }
        model = data.substr( 4 );
        loadBase = logical;
        filename = logical;
        return true;
    }
    bool Empty( const std::string &logical, std::string &error ) override
    {
        if ( failEmpty ) { model = "partially empty"; error = "Reset failed"; return false; }
        model = "empty";
        filename = logical;
        return true;
    }
    void SetFileName( const std::string &logical ) override { filename = logical; }
    std::string CaptureView() override { return camera; }
    void RestoreView( const std::string &view ) override { camera = view; }
};

int main()
{
    fs::path sandbox;
    try
    {
        FakeHost host;
        QaspProjectSession session( host );
        sandbox = session.WorkingDirectory();
        Check( !sandbox.empty(), "Private session root exists" );
        fs::path user = sandbox / "user";
        fs::create_directory( user );
        std::string err;
        Check( session.Adopt( "", err ), err );
        Check( session.Projects().size() == 1 && !session.Active().dirty, "Initial clean tab" );
        const std::string firstBase = session.Active().logicalFile;
        host.model = "aircraft A: unsaved wing and fuselage";
        host.camera = "camera A";
        session.MarkPossiblyChanged();
        Check( session.Active().possiblyChanged, "Immediate conservative dirty indicator" );
        Check( session.New( err ), err );
        Check( session.Projects().size() == 2 && host.model == "empty", "New means another model, not a filename change" );
        Check( session.Projects()[0].dirty, "Inactive unsaved model marked dirty" );
        host.model = "aircraft B: rotor";
        host.camera = "camera B";
        Check( session.Switch( 0, err ), err );
        Check( host.model == "aircraft A: unsaved wing and fuselage", "A contents preserved" );
        Check( host.camera == "camera A", "Camera restored" );
        Check( host.loadBase == firstBase, "Resource base retained, never replaced by snapshot filename" );
        Check( session.Active().dirty, "Loading an unsaved tab does not silently mark it saved" );
        Check( session.Switch( 1, err ), err );
        Check( host.model == "aircraft B: rotor" && host.camera == "camera B", "Independent B model and camera" );

        fs::path bFile = user / "B.vsp3";
        Check( session.Save( bFile.generic_u8string(), err ), err );
        Check( Read( bFile ) == "VSP\naircraft B: rotor", "Explicit file persisted" );
        Check( session.Active().file == bFile.generic_u8string() && !session.Active().dirty, "Save commits filename and clean state" );
        Check( host.writeBase == bFile.generic_u8string(), "Saved resource base is the chosen file" );
        std::string oldFile = session.Active().file;
        host.model = "B edited after save";
        host.failWrite = true;
        Check( !session.Save( bFile.generic_u8string(), err ), "A failed write must report failure" );
        Check( Read( bFile ) == "VSP\naircraft B: rotor", "Partial write does not truncate an existing user file" );
        Check( session.Active().file == oldFile && host.model == "B edited after save", "Failed save preserves current model and filename" );
        std::size_t active = session.ActiveIndex(), size = session.Projects().size();
        Check( !session.New( err ) && session.Projects().size() == size, "Cannot abandon model when its checkpoint fails" );
        Check( session.ActiveIndex() == active && host.model == "B edited after save", "Failed new leaves current content active" );
        Check( !session.Switch( 0, err ) && session.ActiveIndex() == active, "Failed checkpoint prevents switching" );
        host.failWrite = false;

        host.busy = true;
        int loadsBefore = host.loads, writesBefore = host.writes;
        fs::path cFile = user / "C.vsp3";
        Put( cFile, "VSP\naircraft C" );
        Check( !session.New( err ), "New blocked during analysis" );
        Check( !session.Open( cFile.generic_u8string(), err ), "Open blocked during analysis" );
        Check( !session.Switch( 0, err ), "Switch blocked during analysis" );
        Check( !session.Save( bFile.generic_u8string(), err ), "Save blocked during analysis" );
        Check( !session.CloseActive( err ), "Close blocked during analysis" );
        Check( !session.Checkpoint( err ), "Checkpoint blocked during analysis" );
        Check( host.loads == loadsBefore && host.writes == writesBefore, "No engine I/O while busy" );
        host.busy = false;

        fs::path invalid = user / "bad.vsp3";
        Put( invalid, "not an aircraft file" );
        Check( !session.Open( invalid.generic_u8string(), err ), "Invalid open rejected" );
        Check( host.model == "B edited after save" && host.loads == loadsBefore, "Validate before destructive read" );
        host.failLoadOnce = true;
        Check( !session.Open( cFile.generic_u8string(), err ), "Decoder failure reported" );
        Check( host.model == "B edited after save" && session.ActiveIndex() == active, "Failed open rolls back the unsaved model" );
        Check( !session.RecoveryRequired(), "Successful rollback permits continued work" );
        Check( session.Open( cFile.generic_u8string(), err ), err );
        Check( host.model == "aircraft C" && session.Active().title == "C.vsp3", "Valid open creates its own tab" );
        Check( session.Projects().size() == 3, "No replacement of existing tabs" );
        Check( session.Open( ( user / "." / "C.vsp3" ).generic_u8string(), err ), err );
        Check( session.Projects().size() == 3, "Opening equivalent path activates existing tab" );
        Check( !session.Save( bFile.generic_u8string(), err ), "Cannot overwrite the file belonging to a different open tab" );
        Check( Read( bFile ) == "VSP\naircraft B: rotor", "Other tab's disk file protected" );
        Check( session.Checkpoint( err ) && !session.Active().dirty, "Unchanged open stays clean" );
        host.model += " plus tail"; // No dirty hint, as with some script/name/attribute edits.
        Check( session.Checkpoint( err ) && session.Active().dirty, "Close-time serialization detects changes even without a UI hint" );
        Check( Read( sandbox / "session.json" ).find( "resourceBase" ) != std::string::npos, "Manifest records resource bases" );

        Check( session.CloseActive( err ), err );
        Check( session.Projects().size() == 2 && host.model == "B edited after save", "Close activates intact neighbouring model" );
        Check( session.CloseActive( err ), err );
        Check( session.Projects().size() == 1 && host.model == "aircraft A: unsaved wing and fuselage", "Closing B does not affect A" );
        Check( session.CloseActive( err ), err );
        Check( session.Projects().size() == 1 && host.model == "empty", "Closing last tab leaves a fresh empty document" );

        fs::path target = user / "atomic.vsp3";
        Put( target, "original" );
        Check( !QaspWriteFileAtomically( target, []( const fs::path &p, std::string & )
            { Put( p, "partial" ); return false; }, err ), "Atomic writer failure reported" );
        Check( Read( target ) == "original", "Atomic helper preserves original on false return" );
        Check( !QaspWriteFileAtomically( target, []( const fs::path &p, std::string & ) -> bool
            { Put( p, "partial" ); throw std::runtime_error( "write exception" ); }, err ), "Writer exception caught" );
        Check( Read( target ) == "original", "Atomic helper preserves original on exception" );
        Check( !QaspWriteFileAtomically( user, []( const fs::path &, std::string & )
            { return true; }, err ), "Directory is never accepted as save file" );
        Check( !QaspWriteFileAtomically( user / "missing" / "x.vsp3", []( const fs::path &, std::string & )
            { return true; }, err ), "Missing parent produces an error" );
        Check( !QaspWriteFileAtomically( target, [&]( const fs::path &p, std::string & )
            { Put( p, "our new data" ); Put( target, "external edit" ); return true; }, err ),
            "External change during staged save is not silently overwritten" );
        Check( Read( target ) == "external edit", "Concurrent editor's data retained" );
        fs::path appeared = user / "appeared.vsp3";
        Check( !QaspWriteFileAtomically( appeared, [&]( const fs::path &p, std::string & )
            { Put( p, "our data" ); Put( appeared, "another writer" ); return true; }, err ),
            "Newly appeared destination is not silently overwritten" );
        Check( Read( appeared ) == "another writer", "New competing destination retained" );
#ifndef _WIN32
        fs::permissions( target, fs::perms::owner_read | fs::perms::owner_write );
        Check( QaspWriteFileAtomically( target, []( const fs::path &p, std::string & )
            { Put( p, "permission test" ); return true; }, err ), err );
        Check( fs::status( target ).permissions() == ( fs::perms::owner_read | fs::perms::owner_write ),
               "Atomic replacement retains restrictive traditional file permissions" );
        fs::permissions( target, fs::perms::owner_read );
        Check( !QaspWriteFileAtomically( target, []( const fs::path &p, std::string & )
            { Put( p, "bypass readonly" ); return true; }, err ), "Read-only destination is not bypassed with rename" );
        Check( Read( target ) == "permission test", "Read-only original untouched" );
        fs::permissions( target, fs::perms::owner_read | fs::perms::owner_write );
#endif
        // Actual UTF-8 filesystem path, not just an in-memory tab label.
        fs::path unicode = user / fs::u8path( "दिल्ली_模型 with spaces.vsp3" );
        host.model = "Unicode path model";
        Check( session.Save( unicode.generic_u8string(), err ), err );
        Check( session.IsOpenFile( unicode.generic_u8string() ), "Open-file predicate protects the active full project" );
        Check( !session.IsOpenFile( unicode.generic_u8string(), true ), "Predicate can exclude the active tab" );
        Put( unicode, "VSP\nexternal changed model" );
        Check( !session.Save( unicode.generic_u8string(), err ), "Ordinary Save detects edits made on disk after opening/saving" );
        Check( Read( unicode ) == "VSP\nexternal changed model" && host.model == "Unicode path model",
               "External and live versions both survive a detected conflict" );
        session.CleanExit();
        Check( !fs::exists( sandbox ), "Working copies removed only after explicit clean exit" );

        FakeHost diskFull;
        QaspProjectSession resilient( diskFull );
        Check( resilient.Adopt( "", err ), err );
        diskFull.model = "startup unsaved design";
        diskFull.failWritesFrom = diskFull.writes + 2; // Old checkpoint succeeds; new checkpoint fails.
        Check( !resilient.New( err ), "New fails if the new model's snapshot cannot be created" );
        Check( diskFull.model == "startup unsaved design" && resilient.Projects().size() == 1,
               "Rollback restores old content even while serialization is still failing" );
        Check( !resilient.RecoveryRequired(), "Successful disk-free rollback is not mistaken for a partial model" );
        Check( resilient.Active().dirty, "Unknown reload baseline is conservatively dirty" );
        diskFull.failWritesFrom = -1;
        Check( resilient.New( err ), err );
        diskFull.model = "another unsaved model";
        diskFull.failWritesFrom = diskFull.writes + 2;
        Check( resilient.Switch( 0, err ), "Successful target load remains usable if its cache refresh fails" );
        Check( diskFull.model == "startup unsaved design" && !resilient.Warning().empty(),
               "Cache warning is explicit; last good model still loaded" );
        Check( resilient.Active().dirty, "Loaded model without a new baseline is not silently clean" );
        int unchangedLoads = diskFull.loads, unchangedWrites = diskFull.writes;
        fs::remove( resilient.Projects()[1].snapshot );
        Check( resilient.DiscardInactive( 1, err ), "Explicitly discarding an inactive unreadable tab needs no reload" );
        Check( diskFull.model == "startup unsaved design" && diskFull.loads == unchangedLoads && diskFull.writes == unchangedWrites,
               "Discarding an inactive tab never touches the active engine" );
        Check( !resilient.DiscardInactive( 0, err ), "Active model cannot be discarded through the inactive-only operation" );
        resilient.CleanExit();

        FakeHost populated;
        populated.model = "geometry already created before UI start";
        QaspProjectSession startup( populated );
        Check( startup.Adopt( "", err, true ) && startup.Active().dirty, "Populated startup model is not treated as a clean empty tab" );
        Check( startup.Checkpoint( err ) && startup.Active().dirty, "Startup dirty state survives a checkpoint without UI hints" );
        startup.CleanExit();

        FakeHost recovery;
        QaspProjectSession broken( recovery );
        fs::path recoveryRoot = broken.WorkingDirectory();
        Check( broken.Adopt( "", err ), err );
        recovery.model = "precious unsaved model";
        recovery.failEmpty = true;
        recovery.failLoadAlways = true;
        Check( !broken.New( err ) && broken.RecoveryRequired(), "Double failure enters explicit recovery state" );
        Check( fs::exists( broken.Active().snapshot ) && Read( broken.Active().snapshot ).find( "precious" ) != std::string::npos,
               "Last good snapshot retained after rollback failure" );
        Check( !broken.Save( ( recoveryRoot / "unsafe.vsp3" ).generic_u8string(), err ), "No overwriting originals from an uncertain partial model" );
        broken.CleanExit();

        FakeHost noStorage;
        QaspProjectSession unavailable( noStorage, fs::temp_directory_path() / "nonexistent-qasp-test-parent" );
        Check( !unavailable.Adopt( "", err ) && unavailable.Projects().size() == 1, "Unusable temp storage still adopts active model" );
        fs::path directSave = fs::temp_directory_path() / "qasp-test-direct-save.vsp3";
        noStorage.model = "still saveable";
        Check( unavailable.Save( directSave.generic_u8string(), err ), "Explicit save still works when tab cache is unavailable" );
        Check( Read( directSave ) == "VSP\nstill saveable" && !unavailable.Warning().empty(), "Actual save succeeds with clear cache warning" );
        fs::remove( directSave );
        std::cout << checks << " project/filesystem checks passed. Fake host; not a full-engine or Windows runtime test.\n";
        return 0;
    }
    catch ( const std::exception &e )
    {
        std::cerr << "FAILED after " << checks << " checks: " << e.what() << '\n';
        if ( !sandbox.empty() ) { std::cerr << "Working copies retained at " << sandbox << '\n'; }
        return 1;
    }
}
