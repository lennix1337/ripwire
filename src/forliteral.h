#pragma once
// forliteral.h — MF1-LIT: THE LITERAL ANCHOR of a --for answer whose task names a literal (an error string, an env var, a
// config key, an error code).
//
// WHY. A --for task such as `FZF_DEFAULT_COMMAND` or `"dependency cycle" error` names a STRING, not a symbol. The ranked answer
// reads it as words: name-exact finds no definition (`reason="no_candidates"`, empty <sigs>) or stops at the one variable spelled
// like it, and the place the literal is read, compared, built or raised is never printed. Measured before this code (ORCH
// reports/mf1-lit.md Stage 0): on the three MF1-DEV rows whose 12 missing items readout-40 files under LITERAL (fastify-02,
// fzf-05, textual-03) the default answer held none of the code that handles the literal, while a byte-exact scan finds it in
// one call (`--grep`).
//
// WHAT. <lit s= shape= hits= src= …> just before </ctx>, after every byte of the ordinary answer (RIPWIRE_NO_LITERAL_ANCHOR=1
// is the rest of it byte for byte): the grep verb's own byte-exact literal scan (search.h grepCollect + grepEnrich) over the
// index, then
//   * <ls> rows: the sites in SOURCE files (filter.h pathTierOf = Source, not a doc/shell/config file), each labelled by the
//     parse (ingest.h spanTierAt): k="string" inside a string literal, k="code" outside one (an identifier or key use),
//     k="?" a file past the classification budget or with no grammar (never dropped, never called code). One row per source
//     line; sites inside a function or method first, then string before code before unclassified, the scan's own path order
//     within each; in= the innermost definition holding the site and ink= its kind; a string site outside any function (an
//     error-code table entry) carries the kForLiteralContextLines lines after it (lines=), at most kForLiteralContextRows rows;
//   * counts beside them for what is NOT a handler: comment= (comment sites in source files, and statement-level triple-quoted
//     strings, a Python docstring), test= (test or bench paths),
//     doc= (docs, shell scripts, config files) — counted, never listed as rows here (the grep verb lists them: next=);
//   * <b> the whole body of the function or method holding a shown site, at most kForLiteralBodies owners in row order,
//     or <x> an excerpt of the lines around the site when that body is over kForLiteralBodyCapBytes (next= names the whole
//     body); a body this answer already printed is not repeated (body="above" on its rows).
// Several sites are always listed with counts: the block never picks one. floor="1" when the scan stopped early (budget,
// unreadable files): every count is then a floor. Nothing found, nothing printed (never a "the literal is unused" claim).
//
// WHEN. The default regime of the XML dialect, a single root: an explicit --token-budget, --json, --columnar and --candidates
// carry none. The literal is read from the task: the first "double-quoted" span of at least 6 bytes holding a letter and a space
// (a message fragment; a backticked span is code the ranked answer already reads as names, and measured as noise on S1), else the first CONST_SHAPED word (upper-case letters and digits joined by at least one underscore, 5 bytes or more,
// or 10 or more upper-case letters and digits). A task with neither carries no block. The block is not in est_tokens (the
// FOR-ROUTE-DECLINED precedent: the bytes are counted on the element instead, bytes=). MCP `for` is not changed.

#include "filter.h"          // pathTierOf — source / test / doc, the reader's tier
#include "hopbody.h"         // renderBodyElement — the --expand body element, no callee rows
#include "model.h"
#include "nextverb.h"        // nextFlag / nextAttrXml
#include "receiverevidence.h"   // ReceiverEvidence::callableKind
#include "sarif.h"           // rootPrefixOf / rootRelativeUri
#include "search.h"          // grepCollect / grepEnrich / isBareWordCodePath
#include "serialize.h"       // escapeXml / appendCdataSafe / truncateUtf8WithEllipsis

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace rw
{
inline constexpr std::size_t kForLiteralSiteRows      = 8;      // <ls> rows shown (a runaway guard: shown=/src= count, next= lists all)
inline constexpr std::size_t kForLiteralRankedRows    = 256;    // source sites ordered for the rows (past it: counted in src=, not ranked)
inline constexpr std::size_t kForLiteralContextRows   = 2;      // string sites outside any function served with the lines after them
inline constexpr std::uint32_t kForLiteralContextLines = 2;     // ...this many lines after the site
inline constexpr std::size_t kForLiteralBodies        = 3;      // owner bodies served, in row order
inline constexpr std::size_t kForLiteralBodyCapBytes  = 3000;   // a body over this is served as an excerpt <x> around its site
inline constexpr std::uint32_t kForLiteralExcerptLines = 6;     // lines either side of the site in an excerpt
inline constexpr std::size_t kForLiteralTierFiles     = 64;     // source hit files classified (k=); past it k="?"
inline constexpr std::size_t kForLiteralLineBytes     = 200;    // the matched line's text, cut on a UTF-8 boundary
inline constexpr std::size_t kForLiteralMinQuoted     = 6;
inline constexpr std::size_t kForLiteralMinConst      = 5;
inline constexpr std::size_t kForLiteralMinBareCaps   = 10;

inline bool forLiteralOn()
{
    return std::getenv( "RIPWIRE_NO_LITERAL_ANCHOR" ) == nullptr;
}

struct ForLiteral
{
    std::string text;
    std::string_view shape;   // "quoted" | "const"
};

namespace lit_detail
{
inline bool isUpperOrDigit( char c ) noexcept { return ( c >= 'A' && c <= 'Z' ) || ( c >= '0' && c <= '9' ); }
inline bool isAsciiLetter( char c ) noexcept { return ( c >= 'A' && c <= 'Z' ) || ( c >= 'a' && c <= 'z' ); }

// CONST_SHAPED: upper-case letters and digits, starting with a letter, joined by underscores (at least one, none doubled or at an
// end) and at least kForLiteralMinConst bytes; or no underscore and at least kForLiteralMinBareCaps bytes (PYTHONASYNCIODEBUG)
inline bool isConstShaped( std::string_view w ) noexcept
{
    if( w.empty() || !( w[0] >= 'A' && w[0] <= 'Z' ) || w.back() == '_' )
    {
        return false;
    }
    std::size_t underscores = 0;
    for( std::size_t i = 0; i < w.size(); ++i )
    {
        if( w[i] == '_' )
        {
            if( w[i - 1] == '_' )
            {
                return false;
            }
            ++underscores;
        }
        else if( !isUpperOrDigit( w[i] ) )
        {
            return false;
        }
    }
    return underscores > 0 ? w.size() >= kForLiteralMinConst : w.size() >= kForLiteralMinBareCaps;
}

inline std::optional<std::string> quotedSpan( std::string_view task, char q )
{
    for( std::size_t open = task.find( q ); open != std::string_view::npos; open = task.find( q, open + 1 ) )
    {
        const std::size_t close = task.find( q, open + 1 );
        if( close == std::string_view::npos )
        {
            return std::nullopt;
        }
        const std::string_view inner = task.substr( open + 1, close - open - 1 );
        bool letter = false;
        for( const char c : inner )
        {
            letter = letter || isAsciiLetter( c );
        }
        // a MESSAGE fragment: two words or more (a single quoted word is a value or a name, read as words by the ranked answer)
        if( inner.size() >= kForLiteralMinQuoted && letter && inner.find_first_not_of( " \t" ) == 0 && inner.back() != ' '
            && inner.find( ' ' ) != std::string_view::npos )
        {
            return std::string( inner );
        }
        open = close;
    }
    return std::nullopt;
}
}   // namespace lit_detail

// The literal the task names, or none. Quoted spans first (the task's own quoting is the strongest evidence), then the first
// CONST_SHAPED word (trailing/leading punctuation stripped).
inline std::optional<ForLiteral> forLiteralOf( std::string_view task )
{
    if( std::optional<std::string> s = lit_detail::quotedSpan( task, '"' ) )
    {
        return ForLiteral{ std::move( *s ), "quoted" };
    }
    std::size_t i = 0;
    while( i < task.size() )
    {
        while( i < task.size() && ( task[i] == ' ' || task[i] == '\t' ) )
        {
            ++i;
        }
        std::size_t j = i;
        while( j < task.size() && task[j] != ' ' && task[j] != '\t' )
        {
            ++j;
        }
        std::string_view w = task.substr( i, j - i );
        while( !w.empty() && std::string_view( "$(['{<,;:.!?)]}>'`" ).find( w.front() ) != std::string_view::npos )
        {
            w.remove_prefix( 1 );
        }
        while( !w.empty() && std::string_view( "(['{<,;:.!?)]}>'`" ).find( w.back() ) != std::string_view::npos )
        {
            w.remove_suffix( 1 );
        }
        if( lit_detail::isConstShaped( w ) )
        {
            return ForLiteral{ std::string( w ), "const" };
        }
        i = j;
    }
    return std::nullopt;
}

struct ForLiteralRender
{
    std::string xml;          // "" = nothing rides
    std::size_t sites = 0;    // src= (source code + string + unclassified sites)
    std::size_t bodies = 0;   // <b>/<x> elements served
};

namespace lit_detail
{
struct Site
{
    GrepRawHit raw;
    char       k = '?';   // 's' string, 'c' code, '?' unclassified
};

// a statement-level triple-quoted string opens at `start`: nothing but indentation before it on its line (a Python docstring)
inline bool isDocstringAt( const std::string& text, std::size_t start ) noexcept
{
    if( start + 3 > text.size() || !( text.compare( start, 3, "\"\"\"" ) == 0 || text.compare( start, 3, "'''" ) == 0 ) )
    {
        return false;
    }
    for( std::size_t i = start; i > 0; --i )
    {
        const char c = text[i - 1];
        if( c == '\n' )
        {
            return true;
        }
        if( c != ' ' && c != '\t' )
        {
            return false;
        }
    }
    return true;
}

inline std::string lineTextOf( const std::string& text, std::uint32_t line )
{
    std::size_t at = 0;
    for( std::uint32_t l = 1; l < line && at != std::string::npos; ++l )
    {
        at = text.find( '\n', at );
        at = at == std::string::npos ? at : at + 1;
    }
    if( at == std::string::npos || at > text.size() )
    {
        return {};
    }
    const std::size_t end = text.find( '\n', at );
    return text.substr( at, ( end == std::string::npos ? text.size() : end ) - at );
}

inline std::uint32_t lineOfByte( const std::string& text, std::size_t byte ) noexcept
{
    std::uint32_t line = 1;
    for( std::size_t i = 0; i < byte && i < text.size(); ++i )
    {
        line += text[i] == '\n' ? 1u : 0u;
    }
    return line;
}

// lines [a, b] of `text` (1-based, inclusive)
inline std::string linesOf( const std::string& text, std::uint32_t a, std::uint32_t b )
{
    std::string out;
    for( std::uint32_t l = a; l <= b; ++l )
    {
        out += lineTextOf( text, l );
        out += '\n';
    }
    return out;
}
}   // namespace lit_detail

// THE BLOCK. `served` = the answer's own bytes after <sigs> (bodies, fold, slot, hops): a body whose <b … l= p= n=> head is in it
// is not printed again. `useMemo` = the span-tier memo (false under --no-cache).
inline ForLiteralRender renderForLiteral( const IngestResult& ing, const ForLiteral& lit, std::string_view rootArg, std::string_view served,
                                          RedactCounts* redact, bool useMemo )
{
    ForLiteralRender out;
    if( lit.text.empty() || ing.files.empty() )
    {
        return out;
    }
    const GrepCollection collected = grepCollect( ing, lit.text, /*regex=*/false, /*noPrefilter=*/false );
    if( collected.raw.empty() )
    {
        return out;   // nothing found: no claim (a partial index cannot prove the literal unused)
    }
    const std::string rootPrefix = rootArg.empty() ? std::string() : rw::sarif::rootPrefixOf( rootArg );
    const auto relPath = [ & ]( std::uint32_t f ) -> std::string
    {
        return rootArg.empty() ? std::string( ing.files[f] ) : std::string( rw::sarif::rootRelativeUri( ing.files[f], rootPrefix ) );
    };
    // ── path tiers, then the span tier of every source hit file (bounded, in the scan's order: classified files first) ─────────
    std::size_t testHits = 0, docHits = 0, commentHits = 0;
    std::vector<lit_detail::Site> src;
    std::vector<std::uint32_t>    srcFiles;
    for( const GrepRawHit& r : collected.raw )
    {
        const std::string_view p = rootRelPath( ing, r.fileId );
        const PathTier         t = pathTierOf( p );
        if( t == PathTier::TestOrBench )
        {
            ++testHits;
        }
        else if( t == PathTier::Doc || isBareWordCodePath( p ) )
        {
            ++docHits;
        }
        else
        {
            if( srcFiles.empty() || srcFiles.back() != r.fileId )
            {
                srcFiles.push_back( r.fileId );
            }
            src.push_back( lit_detail::Site{ r, '?' } );
        }
    }
    std::vector<std::string> tierPaths;
    for( std::size_t i = 0; i < srcFiles.size() && i < kForLiteralTierFiles; ++i )
    {
        tierPaths.push_back( diskPath( ing, srcFiles[i] ) );
    }
    const SpanTierBatch batch = spanTiersOfFiles( std::span<const std::string>( tierPaths ), useMemo );
    std::uint32_t loadedFile = UINT32_MAX;
    std::string   loadedText;
    std::vector<lit_detail::Site> rows;
    for( lit_detail::Site& s : src )
    {
        std::size_t idx = SIZE_MAX;
        for( std::size_t i = 0; i < tierPaths.size(); ++i )
        {
            if( srcFiles[i] == s.raw.fileId )
            {
                idx = i;
                break;
            }
        }
        if( idx != SIZE_MAX && idx < batch.perFile.size() && batch.perFile[idx].isParsed )
        {
            const SpanTierMap& m = batch.perFile[idx];
            const SpanTier     t = spanTierAt( m, s.raw.byteOffset );
            if( t == SpanTier::Comment )
            {
                ++commentHits;
                continue;
            }
            if( t == SpanTier::String )
            {
                // a statement-level triple-quoted string (a Python docstring): documentation, counted with the comments
                const auto        it    = std::upper_bound( m.startByte.begin(), m.startByte.end(), s.raw.byteOffset );
                const std::size_t start = it == m.startByte.begin() ? 0 : m.startByte[std::size_t( it - m.startByte.begin() ) - 1];
                if( loadedFile != s.raw.fileId )
                {
                    loadedFile = s.raw.fileId;
                    loadedText = docparse::detail::readWholeFile( diskPath( ing, s.raw.fileId ) ).value_or( std::string() );
                }
                if( lit_detail::isDocstringAt( loadedText, start ) )
                {
                    ++commentHits;
                    continue;
                }
            }
            s.k = t == SpanTier::String ? 's' : 'c';
        }
        rows.push_back( s );
    }
    // one row per source LINE (a line holding the literal twice is one site), enriched for its owner, then ordered: sites inside a
    // function or method first, string before code before unclassified, the scan's own order within each class
    std::vector<GrepRawHit> all;
    {
        std::vector<lit_detail::Site> lines;
        for( const lit_detail::Site& r : rows )
        {
            if( lines.empty() || lines.back().raw.fileId != r.raw.fileId || lines.back().raw.line != r.raw.line )
            {
                lines.push_back( r );
            }
            else if( r.k == 's' )
            {
                lines.back().k = 's';
            }
        }
        rows = std::move( lines );
    }
    out.sites = rows.size();
    if( rows.empty() && testHits + docHits + commentHits == 0 )
    {
        return out;
    }
    const std::size_t ranked = std::min( rows.size(), kForLiteralRankedRows );
    for( std::size_t i = 0; i < ranked; ++i )
    {
        all.push_back( rows[i].raw );
    }
    std::vector<GrepHit> enriched = grepEnrich( ing, std::span<const GrepRawHit>( all ) );
    ASSUME( enriched.size() == all.size(), "grepEnrich returns one row per windowed hit" );
    std::vector<std::size_t> order( enriched.size() );
    for( std::size_t i = 0; i < order.size(); ++i )
    {
        order[i] = i;
    }
    const auto rankOf = [ & ]( std::size_t i )
    {
        const bool callable = enriched[i].enclosingId != kNoNode && ReceiverEvidence::callableKind( ing.symbols[enriched[i].enclosingId].kind );
        return ( callable ? 0 : 3 ) + ( rows[i].k == 's' ? 0 : ( rows[i].k == 'c' ? 1 : 2 ) );
    };
    std::stable_sort( order.begin(), order.end(), [ & ]( std::size_t a, std::size_t b ) { return rankOf( a ) < rankOf( b ); } );
    const std::size_t shown = std::min( order.size(), kForLiteralSiteRows );
    std::vector<GrepHit>          hits;
    std::vector<lit_detail::Site> shownRows;
    for( std::size_t i = 0; i < shown; ++i )
    {
        hits.push_back( std::move( enriched[order[i]] ) );
        shownRows.push_back( rows[order[i]] );
    }
    rows = std::move( shownRows );

    // ── the owner bodies: distinct callable owners of the shown rows, in row order ───────────────────────────────────────────
    std::vector<char>   esc;
    std::vector<NodeId> owners;
    std::vector<char>   ownerAbove;   // parallel: already printed by the answer
    std::string         bodiesXml;
    for( const GrepHit& h : hits )
    {
        if( h.enclosingId == kNoNode || !ReceiverEvidence::callableKind( ing.symbols[h.enclosingId].kind )
            || std::find( owners.begin(), owners.end(), h.enclosingId ) != owners.end() )
        {
            continue;
        }
        const Symbol&     s    = ing.symbols[h.enclosingId];
        const std::string path = relPath( s.fileId );
        std::string head = " l=\"" + std::to_string( s.line ) + "\" p=\"";
        head += escapeXml( path, esc );
        head += "\" n=\"";
        head += escapeXml( s.name, esc );
        head += "\"";
        const bool above = served.find( head ) != std::string_view::npos;
        owners.push_back( h.enclosingId );
        ownerAbove.push_back( above ? 1 : 0 );
        if( above || out.bodies >= kForLiteralBodies )
        {
            continue;
        }
        std::optional<std::string> body = renderBodyElement( ing, h.enclosingId, /*capBytes=*/0, /*compress=*/false, redact, rootArg, nullptr );
        if( body && body->size() <= kForLiteralBodyCapBytes )
        {
            bodiesXml += *body;
            ++out.bodies;
            continue;
        }
        // an excerpt around every shown site this owner holds (merged windows), clipped to the owner's own lines
        const std::string text = docparse::detail::readWholeFile( diskPath( ing, s.fileId ) ).value_or( std::string() );
        if( text.empty() )
        {
            continue;
        }
        const std::uint32_t first = lit_detail::lineOfByte( text, s.sigStartByte );
        const std::uint32_t last  = lit_detail::lineOfByte( text, s.endByte > 0 ? s.endByte - 1 : 0 );
        std::uint32_t       a = UINT32_MAX, b = 0;
        for( const GrepHit& o : hits )
        {
            if( o.enclosingId == h.enclosingId )
            {
                a = std::min( a, o.line > kForLiteralExcerptLines ? o.line - kForLiteralExcerptLines : 1u );
                b = std::max( b, o.line + kForLiteralExcerptLines );
            }
        }
        a = std::max( a, first );
        b = std::min( b, last );
        if( a > b )
        {
            continue;
        }
        std::string x = "<x p=\"";
        x += escapeXml( path, esc );
        x += "\" n=\"";
        x += escapeXml( s.name, esc );
        x += "\" l=\"" + std::to_string( a ) + "-" + std::to_string( b ) + "\" of=\"" + std::to_string( first ) + "-" + std::to_string( last ) + "\"";
        x += nextAttrXml( nextFlag( "--expand=", path + ":" + std::to_string( s.line ) + ":" + s.name ) );
        x += "><![CDATA[";
        appendCdataSafe( lit_detail::linesOf( text, a, b ), x );
        x += "]]></x>";
        bodiesXml += x;
        ++out.bodies;
    }

    // ── the rows ──────────────────────────────────────────────────────────────────────────────────────────────────────────
    std::string rowsXml;
    std::size_t contextRows = 0;
    for( std::size_t i = 0; i < hits.size(); ++i )
    {
        const GrepHit& h = hits[i];
        rowsXml += "<ls p=\"";
        rowsXml += escapeXml( relPath( h.fileId ), esc );
        rowsXml += "\" l=\"" + std::to_string( h.line ) + "\" k=\"";
        rowsXml += rows[i].k == 's' ? "string" : ( rows[i].k == 'c' ? "code" : "?" );
        rowsXml += "\"";
        if( h.enclosingId != kNoNode )
        {
            rowsXml += " in=\"";
            rowsXml += escapeXml( h.enclosing, esc );
            rowsXml += "\" ink=\"";
            rowsXml += symTag( ing.symbols[h.enclosingId].kind );
            rowsXml += "\"";
            const auto it = std::find( owners.begin(), owners.end(), h.enclosingId );
            if( it != owners.end() && ownerAbove[std::size_t( it - owners.begin() )] )
            {
                rowsXml += " body=\"above\"";
            }
        }
        std::string line = h.text;
        const std::size_t lead = line.find_first_not_of( " \t" );
        line = lead == std::string::npos ? std::string() : line.substr( lead );
        truncateUtf8WithEllipsis( line, kForLiteralLineBytes );
        const bool callableOwner = h.enclosingId != kNoNode && ReceiverEvidence::callableKind( ing.symbols[h.enclosingId].kind );
        if( rows[i].k == 's' && !callableOwner && contextRows < kForLiteralContextRows )
        {
            // a string site outside any function (a table entry, an error-code map): the lines after it hold what it builds
            const std::string text = docparse::detail::readWholeFile( diskPath( ing, h.fileId ) ).value_or( std::string() );
            const std::uint32_t lastLine = text.empty() ? h.line : lit_detail::lineOfByte( text, text.size() - 1 );
            const std::uint32_t b = std::min( h.line + kForLiteralContextLines, lastLine );
            if( !text.empty() && b > h.line )
            {
                line = lit_detail::linesOf( text, h.line, b );
                truncateUtf8WithEllipsis( line, kForLiteralLineBytes * ( kForLiteralContextLines + 1 ) );
                rowsXml += " lines=\"" + std::to_string( h.line ) + "-" + std::to_string( b ) + "\"";
                ++contextRows;
            }
        }
        rowsXml += "><![CDATA[";
        appendCdataSafe( line, rowsXml );
        rowsXml += "]]></ls>";
    }

    std::string& x = out.xml;
    x = "<!-- ripwire for lit: the task names a literal (s=, from its quotes or a CONST_SHAPED word; shape=) and this block is a "
        "byte-exact scan of the index for it, after the answer above (unchanged, and these bytes are not in est_tokens: bytes= counts "
        "them). ls rows are its sites in source files: k=string inside a string literal, k=code outside one (an "
        "identifier or key use), k=? a file not classified; in= the innermost definition holding the site, ink= its kind; the CDATA "
        "is the site's line (lines=a-b: a string site outside any function, with the lines after it); sites inside a function come "
        "first. hits= every match; src= source lines holding it (shown= of them as rows, capped=1 if cut); comment= source comment "
        "and docstring sites, test= test or bench paths, doc= docs, shell and config files: counted, never a handler row. b is the whole body of a "
        "row's function, x an excerpt of its lines around the site (l= the excerpt, of= the body's lines, next= the whole body); "
        "body=above: already printed in this answer. floor=1: the scan stopped early, so every count is a floor. next= lists every "
        "site. -->";
    x += "<lit s=\"";
    x += escapeXml( lit.text, esc );
    x += "\" shape=\"";
    x += lit.shape;
    x += "\" hits=\"" + std::to_string( collected.raw.size() ) + "\" src=\"" + std::to_string( out.sites ) + "\" shown=\""
         + std::to_string( shown ) + "\"";
    if( shown < out.sites )
    {
        x += " capped=\"1\"";
    }
    x += " comment=\"" + std::to_string( commentHits ) + "\" test=\"" + std::to_string( testHits ) + "\" doc=\"" + std::to_string( docHits ) + "\"";
    if( !collected.cleanScan() )
    {
        x += " floor=\"1\"";
    }
    x += nextAttrXml( nextFlag( "--grep=", lit.text ) );
    const std::string inner = rowsXml + bodiesXml + "</lit>";
    // bytes= is the element's own size from its open tag on (the legend comment excluded): a fixed point over its own digits
    const std::size_t openAt = x.find( "<lit " );
    const std::size_t fixed  = ( x.size() - openAt ) + std::string_view( " bytes=\"\">" ).size() + inner.size();
    std::size_t       bytes  = fixed;
    while( fixed + std::to_string( bytes ).size() != bytes )   // the digits count themselves: converges in <= 2 steps
    {
        bytes = fixed + std::to_string( bytes ).size();
    }
    x += " bytes=\"" + std::to_string( bytes ) + "\">";
    x += inner;
    ENSURES( x.size() - openAt == bytes, "bytes= is the element's own size" );
    return out;
}
}   // namespace rw
