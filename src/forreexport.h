#pragma once
// forreexport.h — RE-EXPORT HOP ROWS on a --for answer (lane alias-import; readout-40 family ALIAS-IMPORT, mf1 shape MF-a:
// "the answer reaches the definition and then stops").
//
// WHY. "Where is ShellError, used in ls.rs, defined — through which re-export hops?" The ranked answer prints the definition
// and stops: the `pub use` / `export { … } from` statements a reader must walk from the use site to it are never rows, because
// they define nothing and call nothing. An agent then greps each prelude and barrel by hand.
//
// WHEN. The task asks where something comes from — a cue word (`defined`, `import`, `export`, `re-export`, `hop(s)`) or the import
// spelling `NAME from <module>` (`verify from hono/jwt`) — or the chain reaches a file the task names (`ls.rs`, a `remove-emphasis`
// directory). Without either, a definition's re-exports are not what was asked: the fire table showed 25 fires across S1/MF1/LD/LD2
// before this gate, 20 of them on path/orientation questions that gained no asked-for name (angular barrels alone 9 rows, 10 KB).
// WHAT. For each task word that names an in-tree definition (a code-shaped word, or `NAME from <module>`), the re-export statements
// that make THAT definition reachable under another module path, walked outward from the definition (nearest first):
//   Rust  `pub use path::Item;`, `pub use path::{A, B};` (an item), `pub use path::module;` (a module that holds it) and
//         `pub use path::*` (a glob: hedged, glob="1" — the module may not supply the name). The path is resolved the way rustc
//         reads it (`crate` / `self` / `super` / a child module of the file / a crate of the tree by its Cargo name), so two
//         same-named items in two modules keep their module identity; a crate name that two manifests spell is cands="N".
//   TS/JS `export { a } from './m'`, `export { a as b } from './m'` (the row says as=), `export * from './m'` (hedged, glob="1"),
//         the module resolved by the import pin's own resolver (resolveJsNamedImportFile, unique-or-nothing).
// A file the task names (`ls.rs`) that imports the definition through one of those spellings adds its own `use`/`import` row
// (u, the start of the chain). Rows only: no row is a call edge, nothing here feeds reaches=, and a type-only re-export is a
// row like any other (it moves a name, not a call).
//
// NOT ON. A definition nothing re-exports (no block); a definition inside a function, impl or inline `mod { }` (its module
// path is not the file's: a disclosed floor); a Rust `pub use a as b` RENAME (counted as_skipped=, not followed: help-wanted
// #445 owns renames); an external target (the walk starts at an in-tree definition, so an outside crate is never a row);
// `export * as ns from` (a namespace, not a name). Cycles terminate (each public spelling is visited once).
// RIPWIRE_NO_REEXPORT_HOPS (any value) turns it off.

#include "depdialect.h"          // dependencyDialect — the Web dialect is the unit the ReExport records are written per
#include "docparse.h"            // detail::readWholeFile — the `pub` check and the row's line read the statement's file
#include "graph.h"               // tomlSectionNames, resolveJsNamedImportFile, lexicalNormalize
#include "infra/namesplit.h"     // isIdentChar
#include "model.h"
#include "serialize.h"           // escapeXml

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace rw
{

inline constexpr std::size_t kReexportNameCap = 3;   // task words served
inline constexpr std::size_t kReexportDefCap  = 2;   // definitions per word that carry a chain
inline constexpr std::size_t kReexportRowCap  = 8;   // hop rows per definition (BFS order); the rest counted in total=
static_assert( kReexportRowCap == 8, "kReexportLegend states the cap (8 shown): move the sentence with the constant" );
inline constexpr std::size_t kReexportUseCap  = 2;   // use-site rows per definition, from task-named files

inline bool forReexportOn()
{
    return std::getenv( "RIPWIRE_NO_REEXPORT_HOPS" ) == nullptr;
}

inline constexpr std::string_view kReexportLegend =
    "<!-- reexports: the statements that re-export a definition the task names, nearest the definition first. def p= l= the "
    "definition, at= its path, total= its rx rows (8 shown), same=1 the same rows as a def above; rx p= l= one `pub use`/`export … from` statement, use= its written path (or from= the module), "
    "at= the path it makes public, as= the new name; glob=1 a glob/star hop (hedged: the module may not supply the name); "
    "cands=N the path names N same-named crates (hedged); u p= l= a task-named file's own import of it (the chain's start); "
    "as_skipped=N renamed re-exports not followed; rows, not call edges -->";

namespace reexport_detail
{

// A public spelling of the definition: Rust = (crate, module path, name); Web = (file, exported name).
struct Spelling
{
    std::uint32_t            crate = kNoFile;   // Rust crate index, or the Web file
    std::vector<std::string> mods;
    std::string              name;
};

struct Row
{
    const Binding* b     = nullptr;
    bool           glob  = false;
    bool           use   = false;   // a task-named file's own import (no new spelling)
    std::size_t    cands = 1;
    Spelling       at;
};

struct Sources
{
    const IngestResult&                                ing;
    HashMap<std::uint32_t, std::optional<std::string>> text;

    const std::string* of( std::uint32_t f )
    {
        auto [ it, fresh ] = text.try_emplace( f );
        if( fresh )
        {
            it->second = docparse::detail::readWholeFile( diskPath( ing, f ) );
        }
        return it->second ? &*it->second : nullptr;
    }
    std::uint32_t lineOf( std::uint32_t f, std::uint32_t byte )
    {
        const std::string* t = of( f );
        if( t == nullptr || byte > t->size() )
        {
            return 0;
        }
        return std::uint32_t( std::count( t->begin(), t->begin() + byte, '\n' ) ) + 1;
    }
    // Is the statement at `byte` a `pub` / `pub(crate)` one? Unknown (unreadable, or not a statement start) reads as no.
    bool isPub( std::uint32_t f, std::uint32_t byte )
    {
        const std::string* t = of( f );
        return t != nullptr && byte < t->size() && std::string_view( *t ).substr( byte ).starts_with( "pub" )
            && byte + 3 < t->size() && !namesplit::isIdentChar( ( *t )[ byte + 3 ] );
    }
};

inline std::string joinPath( const std::vector<std::string>& mods, std::string_view name, std::string_view sep )
{
    std::string out;
    for( const std::string& m : mods )
    {
        out.append( m ).append( sep );
    }
    return out.append( name );
}

inline std::vector<std::string> splitColons( std::string_view path )
{
    std::vector<std::string> out;
    path.remove_prefix( path.starts_with( "::" ) ? 2 : 0 );
    for( std::size_t at = 0; at <= path.size(); )
    {
        const std::size_t end = std::min( path.find( "::", at ), path.size() );
        out.emplace_back( path.substr( at, end - at ) );
        at = end + 2;
    }
    return out;
}

// ── Rust: crates, module paths, `use` paths ─────────────────────────────────────────────────────────────────────────────────
struct RustCrates
{
    std::vector<std::string>                          names;      // crate index -> its (first) Cargo name, `_` for `-`
    std::vector<std::uint32_t>                        crateOf;    // file -> crate index (kNoFile: outside every crate's source root)
    std::vector<std::vector<std::string>>             modOf;      // file -> module path below its crate root
    HashMap<std::string, std::vector<std::uint32_t>> byName;      // Cargo name -> crate indices (two manifests may spell one)
    HashMap<std::string, char>                        modules;    // "<crate>#a::b" -> the module exists as a file

    [[nodiscard]] bool hasModule( std::uint32_t crate, const std::vector<std::string>& mods ) const
    {
        return modules.contains( std::to_string( crate ) + "#" + joinPath( mods, "", "::" ) );
    }
};

// The module path of `rel` below the crate root file `root` (`src/lib.rs`): `src/a/b.rs` and `src/a/b/mod.rs` -> [a, b], the
// root itself -> []. nullopt for a file outside the root's directory (tests/, examples/, build.rs: their own crate roots).
inline std::optional<std::vector<std::string>> rustModulePath( std::string_view rel, std::string_view root )
{
    const std::size_t      slash = root.rfind( '/' );
    const std::string_view dir   = slash == std::string_view::npos ? std::string_view() : root.substr( 0, slash + 1 );
    if( rel == root )
    {
        return std::vector<std::string>{};
    }
    if( !rel.starts_with( dir ) || !rel.ends_with( ".rs" ) || ( rel.substr( dir.size() ) == "main.rs" ) )
    {
        return rel.substr( dir.size() ) == "main.rs" ? std::optional<std::vector<std::string>>( std::vector<std::string>{} ) : std::nullopt;
    }
    std::string_view below = rel.substr( dir.size(), rel.size() - dir.size() - 3 );
    std::vector<std::string> mods;
    for( std::size_t at = 0; at <= below.size(); )
    {
        const std::size_t end = std::min( below.find( '/', at ), below.size() );
        mods.emplace_back( below.substr( at, end - at ) );
        at = end + 1;
    }
    if( !mods.empty() && mods.back() == "mod" )
    {
        mods.pop_back();
    }
    return mods;
}

inline RustCrates buildRustCrates( const IngestResult& ing, const HashMap<std::string, std::uint32_t>& files )
{
    RustCrates                          rc;
    HashMap<std::string, std::uint32_t> crateAtDir;   // manifest directory ("" or "a/b/") -> crate index
    std::vector<std::string>            roots;
    for( std::uint32_t f = 0; f < ing.files.size(); ++f )
    {
        const std::string_view rel = rootRelPath( ing, f );
        if( rel != "Cargo.toml" && !rel.ends_with( "/Cargo.toml" ) )
        {
            continue;
        }
        const std::string_view            dir  = rel.substr( 0, rel.size() - 10 );
        const std::optional<std::string>  text = docparse::detail::readWholeFile( diskPath( ing, f ) );
        std::vector<std::string>          names, libPath;
        if( text )
        {
            tomlSectionNames( *text, { "[package]", "[lib]" }, names );
            tomlSectionNames( *text, { "[lib]" }, libPath, "path" );
        }
        std::string root = std::string( dir ) + ( libPath.empty() ? "src/lib.rs" : lexicalNormalize( libPath.front() ) );
        if( !files.contains( root ) && libPath.empty() )
        {
            root = std::string( dir ) + "src/main.rs";
        }
        const std::uint32_t k = std::uint32_t( roots.size() );
        roots.push_back( std::move( root ) );
        rc.names.emplace_back();
        for( std::string& n : names )
        {
            std::replace( n.begin(), n.end(), '-', '_' );
            if( rc.names.back().empty() )
            {
                rc.names.back() = n;
            }
            std::vector<std::uint32_t>& owners = rc.byName[ n ];
            if( std::ranges::find( owners, k ) == owners.end() )
            {
                owners.push_back( k );
            }
        }
        crateAtDir.try_emplace( std::string( dir ), k );
    }
    rc.crateOf.assign( ing.files.size(), kNoFile );
    rc.modOf.resize( ing.files.size() );
    for( std::uint32_t f = 0; f < ing.files.size() && !roots.empty(); ++f )
    {
        const std::string_view rel = rootRelPath( ing, f );
        if( !rel.ends_with( ".rs" ) )
        {
            continue;
        }
        for( std::size_t slash = rel.rfind( '/' );; slash = rel.rfind( '/', slash - 1 ) )   // the nearest manifest above the file
        {
            const std::string_view dir = slash == std::string_view::npos ? std::string_view() : rel.substr( 0, slash + 1 );
            if( const auto hit = crateAtDir.find( std::string( dir ) ); hit != crateAtDir.end() )
            {
                if( std::optional<std::vector<std::string>> mods = rustModulePath( rel, roots[ hit->second ] ) )
                {
                    rc.crateOf[ f ] = hit->second;
                    rc.modOf[ f ]   = std::move( *mods );
                }
                break;
            }
            if( slash == std::string_view::npos || slash == 0 )
            {
                break;
            }
        }
        if( rc.crateOf[ f ] == kNoFile )
        {
            continue;
        }
        std::vector<std::string> prefix;
        rc.modules.try_emplace( std::to_string( rc.crateOf[ f ] ) + "#", '\0' );
        for( const std::string& m : rc.modOf[ f ] )
        {
            prefix.push_back( m );
            rc.modules.try_emplace( std::to_string( rc.crateOf[ f ] ) + "#" + joinPath( prefix, "", "::" ), '\0' );
        }
    }
    return rc;
}

// Where a `use` path written in file `f` points: the crates it may name (empty = outside the tree, or unreadable) and the
// whole segment path below that crate's root. `crate`/`self`/`super` stay in the file's crate; a first segment that is a child
// module of the file's own module is local (2018 paths); otherwise it is a crate of the tree by its Cargo name.
struct RustTarget
{
    std::vector<std::uint32_t> crates;
    std::vector<std::string>   segs;
};

inline RustTarget resolveRustPath( const RustCrates& rc, std::uint32_t f, std::string_view path )
{
    RustTarget                     t;
    const std::vector<std::string> segs = splitColons( path );
    if( segs.empty() || segs.front().empty() || f >= rc.crateOf.size() )
    {
        return t;
    }
    const std::uint32_t      own  = rc.crateOf[ f ];
    std::vector<std::string> base = own == kNoFile ? std::vector<std::string>{} : rc.modOf[ f ];
    std::vector<std::string> child = base;
    child.push_back( segs.front() );
    const bool crateHead = segs.front() == "crate" || segs.front() == "self" || segs.front() == "super";
    if( !crateHead && ( own == kNoFile || path.starts_with( "::" ) || !rc.hasModule( own, child ) ) )
    {
        const auto hit = rc.byName.find( segs.front() );   // a crate of the tree by its Cargo name (else outside the tree)
        if( hit != rc.byName.end() )
        {
            t.crates = hit->second;
            t.segs.assign( segs.begin() + 1, segs.end() );
        }
        return t;
    }
    if( own == kNoFile )
    {
        return t;
    }
    std::size_t next = crateHead ? 1 : 0;
    if( segs.front() == "crate" )
    {
        base.clear();
    }
    else if( segs.front() == "super" )
    {
        for( next = 0; next < segs.size() && segs[ next ] == "super"; ++next )
        {
            if( base.empty() )
            {
                return t;
            }
            base.pop_back();
        }
    }
    t.crates = { own };
    t.segs   = std::move( base );
    t.segs.insert( t.segs.end(), segs.begin() + std::ptrdiff_t( next ), segs.end() );
    return t;
}

// ── the walk ────────────────────────────────────────────────────────────────────────────────────────────────────────────────
struct Walk
{
    const IngestResult&                         ing;
    Sources&                                    src;
    const RustCrates&                           rc;
    const HashMap<std::string, std::uint32_t>&  files;
    HashMap<std::string, std::vector<const Binding*>> rustByVar;     // Rust TypeImport records by bound name ("*" = globs)
    HashMap<std::string, std::vector<const Binding*>> webByTaken;    // Web ReExport records by the name they take ("*" = stars)
    HashMap<const Binding*, RustTarget>               rustTargets;   // memo
    HashMap<const Binding*, std::uint32_t>            webTargets;    // memo: the module's file, kNoFile when not unique
    std::vector<char>                                 taskFile;      // file -> the task names it (use-site rows)

    const RustTarget& rustTarget( const Binding* b )
    {
        auto [ it, fresh ] = rustTargets.try_emplace( b );
        if( fresh )
        {
            it->second = resolveRustPath( rc, b->fileId, b->importedName );
        }
        return it->second;
    }
    std::uint32_t webTarget( const Binding* b )
    {
        auto [ it, fresh ] = webTargets.try_emplace( b, kNoFile );
        if( fresh )
        {
            const auto [ file, ambiguous ] = resolveJsNamedImportFile( lexicalNormalize( rootRelPath( ing, b->fileId ) ), b->typeName, files, nullptr, b->fileId );
            it->second = ambiguous ? kNoFile : file;
        }
        return it->second;
    }

    // Rust: does record `b` re-export spelling `s`, and under what new spelling? `asSkipped` counts a matching rename.
    std::optional<Row> rustMatch( const Binding* b, const Spelling& s, std::size_t& asSkipped )
    {
        const RustTarget& t = rustTarget( b );
        if( std::ranges::find( t.crates, s.crate ) == t.crates.end() )
        {
            return std::nullopt;
        }
        const std::uint32_t g = b->fileId;
        Row                 row{ .b = b, .cands = t.crates.size() };
        row.at.crate = rc.crateOf[ g ];
        row.at.name  = s.name;
        row.at.mods  = rc.modOf[ g ];
        const bool renamed = b->var != "*" && b->var != b->typeName;
        if( b->var == "*" )
        {
            if( t.segs != s.mods )
            {
                return std::nullopt;
            }
            row.glob = true;
            return row;
        }
        std::vector<std::string> item = s.mods;
        item.push_back( s.name );
        if( t.segs == item && b->typeName == s.name )
        {
            if( renamed )
            {
                ++asSkipped;
                return std::nullopt;
            }
            return row;
        }
        if( t.segs.empty() || t.segs.size() > s.mods.size() || !std::equal( t.segs.begin(), t.segs.end(), s.mods.begin() ) || b->typeName != t.segs.back() )
        {
            return std::nullopt;
        }
        if( renamed )
        {
            ++asSkipped;
            return std::nullopt;
        }
        row.at.mods.push_back( b->var );   // a module re-export: the rest of the path rides below the new module name
        row.at.mods.insert( row.at.mods.end(), s.mods.begin() + std::ptrdiff_t( t.segs.size() ), s.mods.end() );
        return row;
    }

    // Web: does record `b` re-export spelling `s` (file + exported name)?
    std::optional<Row> webMatch( const Binding* b, const Spelling& s )
    {
        if( webTarget( b ) != s.crate )
        {
            return std::nullopt;
        }
        Row row{ .b = b };
        row.glob     = b->importedName == "*";
        row.at.crate = b->fileId;
        row.at.name  = row.glob ? s.name : b->var;
        return row;
    }
};

inline std::string spellingKey( const Spelling& s )
{
    return std::to_string( s.crate ) + "#" + joinPath( s.mods, s.name, "::" );
}

struct Chain
{
    const Symbol*    def = nullptr;
    bool             rust = false;
    Spelling         start;
    std::vector<Row> rows;   // BFS order
    std::vector<Row> uses;
    std::size_t      asSkipped = 0;
};

// The statements that may re-export spelling `s`: by its name, every glob, and (Rust) by each module segment of its path.
inline std::vector<const Binding*> reexportCandidates( const Walk& w, bool rust, const Spelling& s )
{
    std::vector<const Binding*> cand;
    const HashMap<std::string, std::vector<const Binding*>>& by = rust ? w.rustByVar : w.webByTaken;
    const auto take = [ & ]( const std::string& key )
    {
        if( const auto hit = by.find( key ); hit != by.end() )
        {
            cand.insert( cand.end(), hit->second.begin(), hit->second.end() );
        }
    };
    take( s.name );
    take( "*" );
    for( const std::string& m : rust ? s.mods : std::vector<std::string>{} )
    {
        take( m );
    }
    std::ranges::sort( cand );
    cand.erase( std::unique( cand.begin(), cand.end() ), cand.end() );
    return cand;
}

// A matching statement that publishes nothing (not `pub`, or outside every crate root) is a use-site row when the task names its file.
inline void noteReexportUse( const Walk& w, Chain& c, Row row )
{
    const Binding* b = row.b;
    if( b->fileId < w.taskFile.size() && w.taskFile[ b->fileId ] && c.uses.size() < kReexportUseCap
        && std::ranges::none_of( c.uses, [ & ]( const Row& r ) { return r.b == b; } ) )
    {
        row.use = true;
        c.uses.push_back( std::move( row ) );
    }
}

// Web use sites: a task-named file's named import of any spelling the walk reached.
inline void webReexportUses( Walk& w, Chain& c, const HashMap<std::string, char>& reached )
{
    for( const Binding& b : w.ing.bindings )
    {
        if( c.uses.size() >= kReexportUseCap || b.kind != LocalBindKind::JsImport || b.fileId >= w.taskFile.size() || !w.taskFile[ b.fileId ] || b.importedName.empty() )
        {
            continue;
        }
        const std::uint32_t file = w.webTarget( &b );
        if( file != kNoFile && reached.contains( spellingKey( Spelling{ .crate = file, .name = b.importedName } ) ) )
        {
            c.uses.push_back( Row{ .b = &b, .use = true } );
        }
    }
}

// Breadth-first from the definition: each statement that re-exports a reached spelling is a row and publishes a new spelling
// (visited once: cycles terminate); a statement already printed for another spelling is not printed twice.
inline void walkChain( Walk& w, Chain& c )
{
    HashMap<std::string, char> seen;
    seen.try_emplace( spellingKey( c.start ), '\0' );
    std::vector<Spelling> frontier{ c.start };
    for( std::size_t at = 0; at < frontier.size() && frontier.size() < 256; ++at )
    {
        const Spelling s = frontier[ at ];
        for( const Binding* b : reexportCandidates( w, c.rust, s ) )
        {
            std::optional<Row> row = c.rust ? w.rustMatch( b, s, c.asSkipped ) : w.webMatch( b, s );
            const bool publishes = row && ( !c.rust || ( w.src.isPub( b->fileId, b->startByte ) && w.rc.crateOf[ b->fileId ] != kNoFile ) );
            if( row && !publishes )
            {
                noteReexportUse( w, c, std::move( *row ) );
            }
            if( !publishes || !seen.try_emplace( spellingKey( row->at ), '\0' ).second )
            {
                continue;
            }
            frontier.push_back( row->at );
            if( std::ranges::none_of( c.rows, [ & ]( const Row& r ) { return r.b == b; } ) )
            {
                c.rows.push_back( std::move( *row ) );
            }
        }
    }
    if( !c.rust )
    {
        webReexportUses( w, c, seen );
    }
}

// The task split at blanks, quotes, brackets (one break set for both loops: a break the skip did not eat would never advance),
// trailing `,.?:` trimmed: `ls.rs`, `hono/jwt`, `remove-emphasis`, `from`.
inline std::vector<std::string_view> reexportTaskTokens( std::string_view task )
{
    static constexpr std::string_view kBreak = " \t\n`'\"()[]<>";
    std::vector<std::string_view>     tokens;
    for( std::size_t at = 0; at < task.size(); )
    {
        at                   = std::min( task.find_first_not_of( kBreak, at ), task.size() );
        const std::size_t end = std::min( task.find_first_of( kBreak, at ), task.size() );
        std::string_view  tok = task.substr( at, end - at );
        while( !tok.empty() && ( tok.back() == ',' || tok.back() == '.' || tok.back() == '?' || tok.back() == ':' ) )
        {
            tok.remove_suffix( 1 );
        }
        if( !tok.empty() )
        {
            tokens.push_back( tok );
        }
        at = end;
    }
    return tokens;
}

// `NAME from M` with M spelled as a module (`hono/jwt`, `@angular/core`, `./x`): the import spelling, not `path from A to B`.
inline bool reexportFromModule( const std::vector<std::string_view>& tokens, std::size_t i )
{
    if( i + 2 >= tokens.size() || tokens[ i + 1 ] != "from" )
    {
        return false;
    }
    const std::string_view m = tokens[ i + 2 ];
    return m.find( '/' ) != std::string_view::npos || m.starts_with( '@' ) || m.starts_with( '.' );
}

// Does the task ask where something comes from (a definition's location, an import or re-export, its hops)?
inline bool reexportTaskCued( const std::vector<std::string_view>& tokens )
{
    static constexpr std::string_view kCue[] = { "defined", "re-export", "re-exports", "re-exported", "reexport", "export", "exports", "exported",
                                                 "import", "imports", "imported", "hop", "hops" };
    for( std::size_t i = 0; i < tokens.size(); ++i )
    {
        if( std::ranges::find( kCue, tokens[ i ] ) != std::end( kCue ) || reexportFromModule( tokens, i ) )
        {
            return true;
        }
    }
    return false;
}

// The task's words that may name a definition: code-shaped (an upper-case letter, `_` or `$` inside, or a capital first
// letter) or written `NAME from M` with M a module spelling.
inline std::vector<std::string> reexportTaskNames( std::string_view task )
{
    const std::vector<std::string_view> tokens = reexportTaskTokens( task );
    std::vector<std::string>            out;
    const auto add = [ & ]( std::string_view w )
    {
        if( out.size() < kReexportNameCap && w.size() >= 3 && std::ranges::find( out, w ) == out.end() )
        {
            out.emplace_back( w );
        }
    };
    for( std::size_t i = 0; i < tokens.size(); ++i )
    {
        const std::string_view tok = tokens[ i ];
        for( std::size_t at = 0; at < tok.size(); )   // the identifier words inside one token (`Owner::name`, `a/b`)
        {
            std::size_t end = at;
            while( end < tok.size() && ( namesplit::isIdentChar( tok[ end ] ) || tok[ end ] == '$' ) )
            {
                ++end;
            }
            const std::string_view w = tok.substr( at, end - at );
            const bool code = std::ranges::any_of( w.substr( std::min<std::size_t>( 1, w.size() ) ), []( char ch ) { return ( ch >= 'A' && ch <= 'Z' ) || ch == '_' || ch == '$'; } )
                           || ( !w.empty() && w.front() >= 'A' && w.front() <= 'Z' );
            if( code || ( end == tok.size() && at == 0 && reexportFromModule( tokens, i ) ) )
            {
                add( w );
            }
            at = end + 1;
        }
    }
    return out;
}

// Files the task names: a token with an extension by basename (`ls.rs`), a `-`/`_` token by a directory segment (`remove-emphasis`).
inline std::vector<char> reexportTaskFiles( const IngestResult& ing, const std::vector<std::string_view>& tokens )
{
    std::vector<std::string_view> bases, dirs;
    for( const std::string_view tok : tokens )
    {
        if( tok.size() >= 4 && tok.find( '.' ) != std::string_view::npos && tok.find( '/' ) == std::string_view::npos )
        {
            bases.push_back( tok );
        }
        else if( tok.size() >= 4 && tok.find_first_of( "-_" ) != std::string_view::npos && tok.find_first_of( "./" ) == std::string_view::npos )
        {
            dirs.push_back( tok );
        }
    }
    std::vector<char> out( ing.files.size(), 0 );
    for( std::uint32_t f = 0; f < ing.files.size() && !( bases.empty() && dirs.empty() ); ++f )
    {
        const std::string_view rel   = rootRelPath( ing, f );
        const std::size_t      slash = rel.rfind( '/' );
        const std::string_view base  = rel.substr( slash == std::string_view::npos ? 0 : slash + 1 );
        bool                   hit   = std::ranges::find( bases, base ) != bases.end();
        for( std::size_t at = 0; !hit && slash != std::string_view::npos && at < slash; )
        {
            const std::size_t end = std::min( rel.find( '/', at ), slash );
            hit                   = std::ranges::find( dirs, rel.substr( at, end - at ) ) != dirs.end();
            at                    = end + 1;
        }
        out[ f ] = char( hit );
    }
    return out;
}

inline bool reexportDefKind( const Symbol& s )
{
    switch( s.kind )
    {
        case SymKind::Function:
        case SymKind::Class:
        case SymKind::Struct:
        case SymKind::Interface:
        case SymKind::Var:
        case SymKind::NamedType:
        case SymKind::Alias:
        case SymKind::FuncType: return s.scope.empty();
        default: return false;
    }
}

// The XML writer: one <def> row per printed chain, its rx rows and its u rows (a chain that repeats an earlier one's statements
// prints its def row alone, same="1").
struct ReexportXml
{
    const IngestResult&                      ing;
    const RustCrates&                        rc;
    Sources&                                 src;
    std::string                              out;
    std::vector<char>                        esc;
    std::vector<std::vector<const Binding*>> printed;   // the row statements of every chain already printed

    void attr( std::string_view k, std::string_view v )
    {
        out.append( " " ).append( k ).append( "=\"" ).append( escapeXml( v, esc ) ).append( "\"" );
    }
    void num( std::string_view k, std::size_t v )
    {
        out.append( " " ).append( k ).append( "=\"" ).append( std::to_string( v ) ).append( "\"" );
    }
    std::string spelled( const Spelling& s ) const
    {
        return rc.names[ s.crate ] + "::" + joinPath( s.mods, s.name, "::" );
    }
    void row( const Chain& c, const Row& r, std::string_view tag )
    {
        out.append( "<" ).append( tag );
        attr( "p", rootRelPath( ing, r.b->fileId ) );
        num( "l", src.lineOf( r.b->fileId, r.b->startByte ) );
        if( c.rust )
        {
            attr( "use", r.b->var == "*" ? r.b->importedName + "::*" : r.b->importedName );
        }
        else
        {
            attr( "from", r.b->typeName );
        }
        if( c.rust && !r.use )
        {
            attr( "at", spelled( r.at ) );
        }
        if( !c.rust && !r.use && !r.glob && r.b->var != r.b->importedName )
        {
            attr( "as", r.b->var );
        }
        out.append( r.glob ? " glob=\"1\"" : "" );
        if( r.cands > 1 )
        {
            num( "cands", r.cands );
        }
        out.append( "/>" );
    }
    // false when the chain's statements were already printed (its def row then says same="1")
    bool defRow( const Chain& c )
    {
        out.append( out.empty() ? std::string( "<reexports>" ).append( kReexportLegend ) : std::string() ).append( "<def" );
        attr( "n", c.def->name );
        attr( "p", rootRelPath( ing, c.def->fileId ) );
        num( "l", c.def->line );
        if( c.rust )
        {
            attr( "at", spelled( c.start ) );
        }
        num( "total", c.rows.size() );
        if( c.asSkipped > 0 )
        {
            num( "as_skipped", c.asSkipped );
        }
        std::vector<const Binding*> sig;   // a same-named definition reached through the very same statements prints them once
        for( const std::vector<Row>* part : { &c.rows, &c.uses } )
        {
            std::ranges::transform( *part, std::back_inserter( sig ), []( const Row& r ) { return r.b; } );
        }
        const bool same = std::ranges::find( printed, sig ) != printed.end();
        out.append( same ? " same=\"1\"/>" : "/>" );
        if( !same )
        {
            printed.push_back( std::move( sig ) );
        }
        return !same;
    }
    void chain( const Chain& c )
    {
        if( !defRow( c ) )
        {
            return;
        }
        for( std::size_t i = 0; i < c.rows.size() && i < kReexportRowCap; ++i )
        {
            row( c, c.rows[ i ], "rx" );
        }
        for( const Row& u : c.uses )
        {
            row( c, u, "u" );
        }
    }
};

// The definitions a task word names, in a language whose re-exports this file reads (Rust; the Web dialect).
inline std::vector<const Symbol*> reexportDefs( const IngestResult& ing, const std::vector<std::string>& names )
{
    std::vector<const Symbol*> defs;
    for( const Symbol& s : ing.symbols )
    {
        const bool lang = s.lang == Lang::Rust || dependencyDialect( s.lang ) == DepDialect::Web;
        if( lang && reexportDefKind( s ) && std::ranges::find( names, s.name ) != names.end() )
        {
            defs.push_back( &s );
        }
    }
    return defs;
}

// The candidate statements by the name they bind or take: Rust `use` records (a rename also under the name it takes, to be
// counted as_skipped=), Web ReExport records by the name they take ("*" = globs / stars).
inline void indexReexportRecords( const IngestResult& ing, bool rust, bool web, Walk& w )
{
    for( const Binding& b : ing.bindings )
    {
        if( rust && b.kind == LocalBindKind::TypeImport && b.fileId < ing.files.size() && rootRelPath( ing, b.fileId ).ends_with( ".rs" ) )
        {
            w.rustByVar[ b.var ].push_back( &b );
            if( b.var != "*" && b.var != b.typeName )
            {
                w.rustByVar[ b.typeName ].push_back( &b );
            }
        }
        else if( web && b.kind == LocalBindKind::ReExport )
        {
            w.webByTaken[ b.importedName ].push_back( &b );
        }
    }
}

// Every chain of the definitions named `name` that has a hop: one reaching a task-named file first, then the longer, then path order.
inline std::vector<Chain> reexportChains( Walk& w, const std::vector<const Symbol*>& defs, std::string_view name )
{
    std::vector<Chain> chains;
    for( const Symbol* d : defs )
    {
        const bool rust = d->lang == Lang::Rust;
        if( d->name != name || ( rust && w.rc.crateOf[ d->fileId ] == kNoFile ) )
        {
            continue;
        }
        Chain c{ .def = d, .rust = rust };
        c.start = rust ? Spelling{ w.rc.crateOf[ d->fileId ], w.rc.modOf[ d->fileId ], d->name } : Spelling{ .crate = d->fileId, .name = d->name };
        walkChain( w, c );
        if( !c.rows.empty() )
        {
            chains.push_back( std::move( c ) );
        }
    }
    std::ranges::stable_sort( chains, []( const Chain& a, const Chain& b ) { return a.uses.size() != b.uses.size() ? a.uses.size() > b.uses.size() : a.rows.size() > b.rows.size(); } );
    return chains;
}

}   // namespace reexport_detail

// The <reexports> block for `task` ("" when no task word names a definition that something re-exports).
inline std::string renderForReexports( const IngestResult& ing, std::string_view task )
{
    using namespace reexport_detail;
    const std::vector<std::string>   names = reexportTaskNames( task );
    const std::vector<const Symbol*> defs  = names.empty() ? std::vector<const Symbol*>{} : reexportDefs( ing, names );
    if( defs.empty() )
    {
        return {};
    }
    const bool rust = std::ranges::any_of( defs, []( const Symbol* d ) { return d->lang == Lang::Rust; } );
    const bool web  = std::ranges::any_of( defs, []( const Symbol* d ) { return d->lang != Lang::Rust; } );
    HashMap<std::string, std::uint32_t> files;
    files.reserve( ing.files.size() );
    for( std::uint32_t f = 0; f < ing.files.size(); ++f )
    {
        files.emplace( lexicalNormalize( rootRelPath( ing, f ) ), f );
    }
    const RustCrates rc = rust ? buildRustCrates( ing, files ) : RustCrates{ .crateOf = std::vector<std::uint32_t>( ing.files.size(), kNoFile ) };
    Sources          src{ ing, {} };
    const std::vector<std::string_view> tokens = reexportTaskTokens( task );
    const bool                          cued   = reexportTaskCued( tokens );   // otherwise only a chain that reaches a task-named file prints
    Walk             w{ ing, src, rc, files, {}, {}, {}, {}, reexportTaskFiles( ing, tokens ) };
    indexReexportRecords( ing, rust, web, w );
    ReexportXml xml{ ing, rc, src, {}, {}, {} };
    for( const std::string& name : names )
    {
        const std::vector<Chain> chains = reexportChains( w, defs, name );
        for( std::size_t k = 0; k < chains.size() && k < kReexportDefCap; ++k )
        {
            if( cued || !chains[ k ].uses.empty() )
            {
                xml.chain( chains[ k ] );
            }
        }
    }
    return xml.out.empty() ? xml.out : xml.out.append( "</reexports>" );
}

}   // namespace rw
