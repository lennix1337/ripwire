#pragma once
// listedin.h — LISTED-IN: the build / package / registry manifest lines that list a SHOWN file, on a --for answer whose task is a
// creation task (lane manifest-noncode; CONTRIBUTING family 10, slice 10c, beside epmanifest.h's Python entry points).
//
// WHY. "Add a new tmux command / a new hono middleware / a new Envoy extension" is answered by the code pattern AND by the
// non-code files every new instance must also be listed in: automake's `dist_tmux_SOURCES` list, package.json "exports" +
// "typesVersions" and jsr.json "exports", Bazel's extensions_build_config.bzl. No code edge reaches them (Makefile.am, .bzl and
// CODEOWNERS are not crawled; package.json keys are indexed as data rows that never rank for the task), so the answer stopped at
// the source files. Measured before this code (reports/manifest-noncode.md Stage 0): mf1-tmux-01, mf1-hono-03 and ld-envoy-07
// printed no manifest line although the shown exemplar files ARE listed in one.
//
// WHAT. <listed p= of= by=[ also= also_total=][ lines_total= capped="1"]><ln l=>LINE</ln>…</listed>, after the file tail and
// beside <ep>: one element per manifest (at the root or an ancestor directory of a shown file) with at least one line naming a
// shown file. of= the FIRST shown file (rank order) the manifest names, and every line naming it, byte-exact (XML-escaped), each
// preceded once by its nearest less-indented line above (the stanza it sits in: the `dist_tmux_SOURCES = \` assignment, the
// `"./timing": {` key, the `EXTENSIONS = {` dict). by="path" the line names of='s path (relative to the manifest's directory, or
// root-relative); by="dir" it names of='s directory by a tail of two or more path segments (package.json lists a build output
// under that directory, a .bzl a Bazel label) — the file's directory, not the file. also= the other shown files the same manifest
// names (by either rule), at most kListedAlsoNames, also_total= all of them. A path token matches whole segments only (bounded by a
// non-name character on both sides), so `middleware/log` never matches `middleware/logger`. gen="1": the manifest says it was
// generated (an automake Makefile.in, a configure-written Makefile) — the file to edit is its source.
//
// GATE (present-only): the task has a creation word (new, add, adding, register, registration, create, creating, plugin, plugins,
// extension, extensions) as a whole word (not inside an identifier: `Plan::AddTarget`, `new_local`, `createX` do not fire); the DEFAULT regime and the XML dialect only, as the caller hop and
// <ep>. RIPWIRE_NO_LISTED_IN (any value) turns it off — the A/B handle.
// FLOORS: a manifest that lists a file by a glob or a variable (`$(wildcard src/*.c)`, `glob(["*.cc"])`), a list keyed by a
// registered NAME string (a man page's command list, a docs index — mf1-lit's literal anchors), a manifest outside the shown
// files' ancestor directories, a multi-root run. Files under vendor/, third_party/, node_modules/ are never read (vendored).

#include "docparse.h"     // readRegularFile — a fixed-name file in the tree, refused when it is not a regular file
#include "model.h"        // IngestResult, rootRelPath, diskPath
#include "serialize.h"    // escapeXml, truncateUtf8WithEllipsis

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace rw
{

// Runaway guards, not size targets (knob census ADDENDUM): every manifest, line or name past them is dropped WITH a disclosure
// (manifests_capped="1" manifests_total= on the first element, capped="1" lines_total= on the element, also_total=).
inline constexpr std::size_t kListedManifests   = 4;     // manifests per answer
inline constexpr std::size_t kListedLines       = 8;     // printed lines per manifest (stanza openers included)
inline constexpr std::size_t kListedAlsoNames   = 4;     // also= names per manifest
inline constexpr std::size_t kListedShownFiles  = 16;    // distinct shown files whose ancestors are read
inline constexpr std::size_t kListedLineBytes   = 240;   // one printed line (UTF-8-safe cut + ellipsis)
inline constexpr std::size_t kListedBzlPerDir   = 8;     // *.bzl files read per directory

inline bool forListedInOn()
{
    return std::getenv( "RIPWIRE_NO_LISTED_IN" ) == nullptr;
}

struct ListedRender
{
    std::string xml;   // the <listed> elements; empty when no manifest lists a shown file (or the task is not a creation task)
};

namespace listeddetail
{

// A creation word as a whole word of the task, case-folded; words split on every character that is not alphanumeric or '_', so a
// creation word inside an identifier (`Plan::AddTarget`, `Workspace::new_local`, `createSingletons`) is not one: that task names a
// symbol, it does not ask to add one. "hono/<new-middleware>" → new.
inline bool isCreationTask( std::string_view task )
{
    static constexpr std::string_view kWords[] = { "new", "add", "adding", "register", "registration", "create", "creating",
                                                    "plugin", "plugins", "extension", "extensions" };
    std::string word;
    const auto  flush = [ & ]()
    {
        const bool hit = std::ranges::find( kWords, std::string_view( word ) ) != std::end( kWords );
        word.clear();
        return hit;
    };
    char prev = ' ';   // the previous ORIGINAL character (word holds the folded ones, so "NEW" is not a camel boundary)
    for( const char ch : task )
    {
        const unsigned char c = static_cast<unsigned char>( ch );
        const bool          camel = std::isupper( c ) && !word.empty() && std::islower( static_cast<unsigned char>( prev ) );
        prev = ch;
        if( !std::isalnum( c ) && ch != '_' )
        {
            if( flush() )
            {
                return true;
            }
            continue;
        }
        if( camel )
        {
            word += '\x01';   // a lower→upper camel boundary inside one word: the word is an identifier, never a creation word
        }
        word += char( std::tolower( c ) );
    }
    return flush();
}

inline bool isNameChar( char c ) noexcept
{
    const unsigned char u = static_cast<unsigned char>( c );
    return std::isalnum( u ) || c == '_' || c == '-';
}

// `tok` occurs in `line` bounded on both sides: the byte before is not a name character or '.', the byte after is not a name
// character (a '.' after is allowed only when the token itself ends in a path segment, never mid-name: "abi.h" vs "abi.hpp" is
// handled by the name rule since 'p' is a name character).
inline bool boundedFind( std::string_view line, std::string_view tok ) noexcept
{
    if( tok.empty() )
    {
        return false;
    }
    for( std::size_t at = line.find( tok ); at != std::string_view::npos; at = line.find( tok, at + 1 ) )
    {
        const bool leftOk  = at == 0 || ( !isNameChar( line[at - 1] ) && line[at - 1] != '.' );
        const std::size_t end = at + tok.size();
        const bool rightOk = end == line.size() || !isNameChar( line[end] );
        if( leftOk && rightOk )
        {
            return true;
        }
    }
    return false;
}

inline std::size_t segmentCount( std::string_view p ) noexcept
{
    return p.empty() ? 0u : std::size_t( std::ranges::count( p, '/' ) ) + 1u;
}

inline bool isVendoredDir( std::string_view dir ) noexcept
{
    for( const std::string_view seg : { std::string_view( "vendor" ), std::string_view( "third_party" ), std::string_view( "node_modules" ) } )
    {
        if( dir == seg || dir.starts_with( std::string( seg ) + "/" ) || dir.find( "/" + std::string( seg ) + "/" ) != std::string_view::npos
            || dir.ends_with( "/" + std::string( seg ) ) )
        {
            return true;
        }
    }
    return false;
}

struct Manifest
{
    std::string rel;    // root-relative path
    std::string dir;    // its root-relative directory ("" = the root)
    std::string text;
    bool        gen = false;
};

struct ManifestLine
{
    std::uint32_t    line = 0;
    std::string_view text;
};

inline bool saysGenerated( std::string_view text ) noexcept
{
    const std::string_view head = text.substr( 0, std::min<std::size_t>( text.size(), 400 ) );
    return head.find( "generated by" ) != std::string_view::npos || head.find( "Generated by" ) != std::string_view::npos
        || head.find( "DO NOT EDIT" ) != std::string_view::npos;
}

// The fixed-name manifests of one directory (and its *.bzl files, name order, at most kListedBzlPerDir).
inline void readDirManifests( std::string_view dir, const std::filesystem::path& disk, std::vector<Manifest>& out )
{
    static constexpr const char* kNames[] = { "Makefile.am", "Makefile", "GNUmakefile", "CMakeLists.txt", "meson.build", "BUILD",
                                              "BUILD.bazel", "package.json", "jsr.json", "deno.json", "Cargo.toml", "CODEOWNERS" };
    const auto add = [ & ]( const std::string& name )
    {
        const std::optional<std::string> text = docparse::detail::readRegularFile( name, ( disk / name ).string() );
        if( text && !text->empty() )
        {
            Manifest m{ dir.empty() ? name : std::string( dir ) + "/" + name, std::string( dir ), *text, saysGenerated( *text ) };
            out.push_back( std::move( m ) );
        }
    };
    bool haveAm = false;
    for( const char* name : kNames )
    {
        if( std::string_view( name ) == "Makefile" && haveAm )
        {
            continue;   // automake writes Makefile from Makefile.am: the source is the one to show
        }
        const std::size_t before = out.size();
        add( name );
        haveAm = haveAm || ( std::string_view( name ) == "Makefile.am" && out.size() > before );
    }
    if( dir.empty() )
    {
        for( const char* name : { ".github/CODEOWNERS", "docs/CODEOWNERS" } )
        {
            add( name );
        }
    }
    std::vector<std::string> bzl;
    std::error_code          ec;
    for( std::filesystem::directory_iterator it( disk, ec ), end; !ec && it != end; it.increment( ec ) )
    {
        const std::string name = it->path().filename().string();
        if( name.ends_with( ".bzl" ) )
        {
            bzl.push_back( name );
        }
    }
    std::ranges::sort( bzl );
    for( std::size_t i = 0; i < bzl.size() && i < kListedBzlPerDir; ++i )
    {
        add( bzl[i] );
    }
}

// The path tokens that name `fileRel` from a manifest in `mdir`, most specific first, each tagged path (the file) or dir.
struct Token
{
    std::string text;
    bool        isDir = false;
};

inline std::vector<Token> tokensFor( std::string_view fileRel, std::string_view mdir )
{
    std::vector<Token> t;
    const std::string_view below = mdir.empty() ? fileRel
                                 : ( fileRel.starts_with( mdir ) && fileRel.size() > mdir.size() && fileRel[mdir.size()] == '/' )
                                       ? fileRel.substr( mdir.size() + 1 ) : std::string_view();
    if( !below.empty() )
    {
        t.push_back( { std::string( below ), false } );
    }
    if( !mdir.empty() )
    {
        t.push_back( { std::string( fileRel ), false } );
    }
    const std::size_t slash = fileRel.rfind( '/' );
    // A manifest in the file's own directory names it by its file name only: a directory tail there is the package's own label
    // (a Bazel BUILD's `//pkg:dep` deps), not a listing of the file.
    const bool sameDir = slash == std::string_view::npos ? mdir.empty() : fileRel.substr( 0, slash ) == mdir;
    if( slash != std::string_view::npos && !sameDir )
    {
        std::string_view dir = fileRel.substr( 0, slash );
        // directory tails of two or more segments, longest first
        while( segmentCount( dir ) >= 2 )
        {
            t.push_back( { std::string( dir ), true } );
            dir = dir.substr( dir.find( '/' ) + 1 );
        }
    }
    return t;
}

inline std::vector<ManifestLine> splitLines( std::string_view text )
{
    std::vector<ManifestLine> lines;
    std::uint32_t             n = 0;
    std::size_t               start = 0;
    while( start <= text.size() )
    {
        std::size_t end = text.find( '\n', start );
        if( end == std::string_view::npos )
        {
            end = text.size();
        }
        std::string_view l = text.substr( start, end - start );
        if( !l.empty() && l.back() == '\r' )
        {
            l.remove_suffix( 1 );
        }
        ++n;
        if( !( end == text.size() && l.empty() ) )
        {
            lines.push_back( { n, l } );
        }
        if( end == text.size() )
        {
            break;
        }
        start = end + 1;
    }
    return lines;
}

inline std::size_t indentOf( std::string_view l ) noexcept
{
    std::size_t i = 0;
    while( i < l.size() && ( l[i] == ' ' || l[i] == '\t' ) )
    {
        ++i;
    }
    return i;
}

inline bool isCommentLine( std::string_view l ) noexcept
{
    const std::size_t i = indentOf( l );
    return i < l.size() && ( l[i] == '#' || l.substr( i ).starts_with( "//" ) );
}

// Which token of `toks` the line names (first match, specificity order), or -1.
inline int lineMatches( std::string_view line, const std::vector<Token>& toks )
{
    for( std::size_t k = 0; k < toks.size(); ++k )
    {
        if( boundedFind( line, toks[k].text ) )
        {
            return int( k );
        }
    }
    return -1;
}

}   // namespace listeddetail

// The <listed> elements for the shown rows (rank order). Empty unless the task is a creation task and a manifest names a shown file.
inline ListedRender renderForListedIn( const IngestResult& ing, std::span<const NodeId> shown, std::string_view task )
{
    using namespace listeddetail;
    ListedRender out;
    if( ing.crawlRoot.empty() || shown.empty() || !isCreationTask( task ) )
    {
        return out;
    }
    // distinct shown files, rank order
    std::vector<std::uint32_t> files;
    for( const NodeId id : shown )
    {
        if( id < ing.symbols.size() && std::ranges::find( files, ing.symbols[id].fileId ) == files.end() && files.size() < kListedShownFiles )
        {
            files.push_back( ing.symbols[id].fileId );
        }
    }
    // the manifests of every shown file's directory and ancestors, nearest first, each directory once
    std::vector<std::string> seenDirs;
    std::vector<Manifest>    manifests;
    for( const std::uint32_t f : files )
    {
        const std::string_view rel  = rootRelPath( ing, f );
        std::filesystem::path  disk = std::filesystem::path( diskPath( ing, f ) ).parent_path();
        std::string_view       dir  = rel.substr( 0, rel.rfind( '/' ) == std::string_view::npos ? 0 : rel.rfind( '/' ) );
        while( true )
        {
            if( std::ranges::find( seenDirs, dir ) == seenDirs.end() )
            {
                seenDirs.emplace_back( dir );
                if( !isVendoredDir( dir ) )
                {
                    readDirManifests( dir, disk, manifests );
                }
            }
            if( dir.empty() )
            {
                break;
            }
            const std::size_t up = dir.rfind( '/' );
            dir  = up == std::string_view::npos ? std::string_view() : dir.substr( 0, up );
            disk = disk.parent_path();
        }
    }
    struct Hit
    {
        std::size_t               m = 0;
        std::size_t               firstFile = 0;   // index into files
        bool                      byDir = false;
        std::vector<std::size_t>  lineIx;          // matched lines of the first file (indices into lines)
        std::vector<std::string>  also;            // other shown files named
        std::vector<ManifestLine> lines;
    };
    std::vector<Hit> hits;
    for( std::size_t mi = 0; mi < manifests.size(); ++mi )
    {
        Hit h;
        h.m     = mi;
        h.lines = splitLines( manifests[mi].text );
        bool found = false;
        for( std::size_t fi = 0; fi < files.size(); ++fi )
        {
            const std::string_view   rel  = rootRelPath( ing, files[fi] );
            if( rel == manifests[mi].rel )
            {
                continue;   // a manifest never lists itself
            }
            const std::vector<Token> toks = tokensFor( rel, manifests[mi].dir );
            std::vector<std::size_t> mine;
            bool                     dirOnly = true;
            for( std::size_t li = 0; li < h.lines.size(); ++li )
            {
                if( isCommentLine( h.lines[li].text ) )
                {
                    continue;
                }
                const int k = lineMatches( h.lines[li].text, toks );
                if( k >= 0 )
                {
                    mine.push_back( li );
                    dirOnly = dirOnly && toks[ std::size_t( k ) ].isDir;
                }
            }
            if( mine.empty() )
            {
                continue;
            }
            if( !found )
            {
                found       = true;
                h.firstFile = fi;
                h.byDir     = dirOnly;
                h.lineIx    = std::move( mine );
            }
            else
            {
                h.also.emplace_back( rel );
            }
        }
        if( found )
        {
            hits.push_back( std::move( h ) );
        }
    }
    if( hits.empty() )
    {
        return out;
    }
    // A manifest naming more shown files first (a registry of the exemplar's siblings), then nearest-first discovery order.
    std::ranges::stable_sort( hits, []( const Hit& a, const Hit& b ) { return a.also.size() > b.also.size(); } );
    const std::size_t total = hits.size();
    std::vector<char> esc;
    for( std::size_t hi = 0; hi < hits.size() && hi < kListedManifests; ++hi )
    {
        const Hit&      h   = hits[hi];
        const Manifest& man = manifests[h.m];
        // the printed lines: each matched line, preceded once by its stanza opener (nearest less-indented, non-blank line above)
        std::vector<std::size_t> print;
        for( const std::size_t li : h.lineIx )
        {
            const std::size_t ind = indentOf( h.lines[li].text );
            if( ind > 0 )
            {
                for( std::size_t up = li; up-- > 0; )
                {
                    const std::string_view t = h.lines[up].text;
                    if( indentOf( t ) < t.size() && indentOf( t ) < ind )
                    {
                        if( std::ranges::find( print, up ) == print.end() )
                        {
                            print.push_back( up );
                        }
                        break;
                    }
                }
            }
            if( std::ranges::find( print, li ) == print.end() )
            {
                print.push_back( li );
            }
        }
        std::ranges::sort( print );
        const std::size_t printTotal = print.size();
        out.xml += std::format( "<listed p=\"{}\"", escapeXml( man.rel, esc ) );
        out.xml += std::format( " of=\"{}\" by=\"{}\"", escapeXml( rootRelPath( ing, files[h.firstFile] ), esc ), h.byDir ? "dir" : "path" );
        if( man.gen )
        {
            out.xml += " gen=\"1\"";
        }
        if( !h.also.empty() )
        {
            std::string also;
            for( std::size_t a = 0; a < h.also.size() && a < kListedAlsoNames; ++a )
            {
                also += ( a ? "," : "" ) + h.also[a];
            }
            out.xml += std::format( " also=\"{}\"", escapeXml( also, esc ) );
            if( h.also.size() > kListedAlsoNames )
            {
                out.xml += std::format( " also_total=\"{}\"", h.also.size() );
            }
        }
        if( printTotal > kListedLines )
        {
            out.xml += std::format( " capped=\"1\" lines_total=\"{}\"", printTotal );
        }
        if( hi == 0 && total > kListedManifests )
        {
            out.xml += std::format( " manifests_capped=\"1\" manifests_total=\"{}\"", total );
        }
        out.xml += ">";
        for( std::size_t pi = 0; pi < print.size() && pi < kListedLines; ++pi )
        {
            std::string text( h.lines[ print[pi] ].text );
            truncateUtf8WithEllipsis( text, kListedLineBytes );
            out.xml += std::format( "<ln l=\"{}\">{}</ln>", h.lines[ print[pi] ].line, escapeXml( text, esc ) );
        }
        out.xml += "</listed>";
    }
    return out;
}

// The element's legend clauses, present-only beside it: the full and the compact dialect. No "--": they ride inside an XML comment.
inline constexpr std::string_view kForListedInLegend =
    "; listed p= of= by=: a build, package or registry manifest (Makefile.am, CMakeLists.txt, meson.build, BUILD, *.bzl, package.json, "
    "jsr.json, deno.json, Cargo.toml, CODEOWNERS) at the root or above a shown file that lists of=, the first shown file it names, "
    "on a creation task; ln l=: each line naming of= byte-exact, after the less-indented line it sits under; by=path the line names "
    "the file, by=dir only its directory (2+ path segments); also=: other shown files the manifest names, also_total= all; gen=1 the "
    "manifest says it is generated; capped=1 lines_total=, manifests_capped=1 manifests_total=: cut, the total; a glob or "
    "variable listing is not matched (a floor)";
inline constexpr std::string_view kForCompactListedInLegend =
    "; listed p= of= by=path|dir: a manifest above a shown file listing of= (a creation task); ln l=: its lines byte-exact with the "
    "line they sit under; also=/also_total= other shown files it lists; gen=1 generated; capped=1 lines_total=/manifests_capped=1 if cut";

}   // namespace rw
