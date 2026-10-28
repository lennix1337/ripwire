#pragma once
// defslookup.h — THE one definition lookup every verb shares (lane D2 DEFS-LOOKUP, PLAN_DEFS_LOOKUP 2026-10-27).
//
// "Which definitions does this name bind?" had three partial answers: the resolver's count (from_defs=7 on --path), the shared
// refusal's file list (selectorrefuse.h, failure only) and MCP find_symbol's def table. lookupDefs answers it once: the shared
// resolver (resolveAllByNameQualified — every spelling a SYM verb accepts), folded into definition groups by defattrib.h's ONE
// grouping rule (a C-family prototype joins the definition it declares; a TS overload joins its implementation), each group
// with the spelling that picks it ALONE (sel=, proven by re-resolving it), its class (the --path test-twin rule) and its
// in-graph caller count (a floor). Consumers: the --defs verb (verbs_navigate.h runDefs) and --path's inline alternatives
// (defAltsXml), which print the other definitions of an endpoint whose pick is not provably unique.
//
// WHAT IT DOES NOT MEAN (checklist 3): a definition the parser does not model (an assignment `x.f = …`, define_method,
// setattr) is no row and is not counted — defs= counts parsed definitions; a JSON/YAML key is never one. in= counts distinct
// graph callers, a floor (declined and unbound calls are not edges). sel_proven="0" says no spelling this lookup tried picks
// that group alone; the sel= it prints then names the group's file and owner and may bind more than it.

#include "defattrib.h"     // DefGroups, defGroupsOf — the ONE definition-group rule
#include "filter.h"        // isTestSymbol, pathTierOf (via pathDefInTests)
#include "graph.h"         // resolveAllByNameQualified — the shared resolver
#include "mapinventory.h"  // isInventoryCodeSymbol — a data Section (doc heading, JSON/YAML key) is no code definition
#include "memberowner.h"   // MemberOwners, qualifiedMemberName — n= as every row spells it
#include "model.h"
#include "nextverb.h"      // nextFlag / nextAttrXml
#include "pathpairs.h"     // pathDefInTests — the one test-path rule --path's twins use
#include "serialize.h"     // escapeXml / jsonStr / symTag

#include <algorithm>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace rw
{

// The house cap for definition groups (defattrib.h kDefTableCap): a page of rows, never a cut — past it the answer pages (next=).
inline constexpr std::size_t kDefRowsCap = kDefTableCap;
// --defs takes at most this many names in one call (one index load).
inline constexpr std::size_t kDefsNamesMax = 16;

struct DefRow
{
    std::uint32_t group     = 0;       // the DefGroups index
    NodeId        rep       = kNoNode; // the group's bodied member (else its first match)
    std::string   sel;                 // the spelling that picks this group alone (when selProven)
    bool          selProven = false;   // re-resolving sel binds this group's members and nothing else
    bool          test      = false;   // pathDefInTests: a test/bench file or a test symbol
    std::size_t   callers   = 0;       // distinct in-graph callers over the group's members (a floor)
};

struct DefLookup
{
    std::string_view    spec;
    std::vector<NodeId> matches;          // the resolver's answer, in its order
    DefGroups           groups;
    std::vector<DefRow> rows;             // one per group: source before test, then callers (desc), then path
    std::size_t         unprovenDefs = 0; // the resolver's decl→def residue (graph.h H1), as --path counts it
    std::size_t         dataDefs     = 0; // matches that are data Sections (a doc heading, a JSON/YAML key): not rows, counted

    std::size_t defs() const noexcept { return rows.size(); }
};

namespace defslookup_detail
{
// distinct non-prose callers of `n` in the graph's in-edges (markdown links excluded: a doc naming it is no caller)
inline void collectCallers( const IngestResult& ing, const Graph& g, NodeId n, std::vector<NodeId>& into )
{
    if( n >= g.inEdges.rows() )
    {
        return;
    }
    const auto* ro = g.inEdges.rowOffsets();
    const auto* ci = g.inEdges.colIndices();
    for( auto k = ro[n]; k < ro[ n + 1 ]; ++k )
    {
        const NodeId c = NodeId( ci[k] );
        if( c < ing.symbols.size() && pathTierOf( rootRelPath( ing, ing.symbols[c].fileId ) ) != PathTier::Doc )
        {
            into.push_back( c );
        }
    }
}
// does `spell` resolve to members of group `k` and nothing else?
inline bool picksAlone( const IngestResult& ing, std::string_view spell, const std::vector<NodeId>& members )
{
    const std::vector<NodeId> got = resolveAllByNameQualified( ing, spell );
    return !got.empty() && std::all_of( got.begin(), got.end(), [ & ]( NodeId n ) { return std::find( members.begin(), members.end(), n ) != members.end(); } );
}
}   // namespace defslookup_detail

// THE lookup. `g` supplies the caller counts.
inline DefLookup lookupDefs( const IngestResult& ing, const Graph& g, std::string_view spec )
{
    DefLookup out;
    out.spec    = spec;
    out.matches = resolveAllByNameQualified( ing, spec, &out.unprovenDefs );
    const auto data = std::remove_if( out.matches.begin(), out.matches.end(), [ & ]( NodeId n ) { return !isInventoryCodeSymbol( ing.symbols[n] ); } );
    out.dataDefs    = std::size_t( out.matches.end() - data );
    out.matches.erase( data, out.matches.end() );
    if( out.matches.empty() )
    {
        return out;
    }
    out.groups = defGroupsOf( ing, out.matches );
    const MemberOwners owners( ing );
    std::vector<std::vector<NodeId>> members( out.groups.size() );
    for( std::size_t i = 0; i < out.matches.size(); ++i )
    {
        members[ out.groups.ofMatch[i] ].push_back( out.matches[i] );
    }
    out.rows.resize( out.groups.size() );
    std::vector<NodeId> callers;
    for( std::uint32_t k = 0; k < out.groups.size(); ++k )
    {
        DefRow& r = out.rows[k];
        r.group   = k;
        r.rep     = out.groups.rep[k];
        r.test    = pathDefInTests( ing, r.rep );
        callers.clear();
        for( const NodeId m : members[k] )
        {
            defslookup_detail::collectCallers( ing, g, m, callers );
        }
        std::sort( callers.begin(), callers.end() );
        callers.erase( std::unique( callers.begin(), callers.end() ), callers.end() );
        // a member calling itself (recursion) is no outside caller
        r.callers = std::size_t( std::count_if( callers.begin(), callers.end(), [ & ]( NodeId c )
                                                { return std::find( members[k].begin(), members[k].end(), c ) == members[k].end(); } ) );
        // sel=: the shortest spelling the resolver binds to this group alone — the name as given (one group), file:name,
        // file:Owner.name, then the @file:line seed (one definition starting on that line)
        const Symbol&          s     = ing.symbols[ r.rep ];
        const std::string      file( rootRelPath( ing, s.fileId ) );
        const std::string      owned = qualifiedMemberName( owners, s );
        std::vector<std::string> tries;
        if( out.groups.size() == 1 )
        {
            tries.emplace_back( spec );
        }
        tries.push_back( file + ":" + s.name );
        if( owned != s.name )
        {
            tries.push_back( file + ":" + owned );
        }
        tries.push_back( "@" + file + ":" + std::to_string( s.line ) );
        for( const std::string& t : tries )
        {
            if( defslookup_detail::picksAlone( ing, t, members[k] ) )
            {
                r.sel       = t;
                r.selProven = true;
                break;
            }
        }
        if( !r.selProven )
        {
            r.sel = owned == s.name ? file + ":" + s.name : file + ":" + owned;
        }
    }
    std::stable_sort( out.rows.begin(), out.rows.end(), [ & ]( const DefRow& a, const DefRow& b )
    {
        if( a.test != b.test )
        {
            return !a.test;
        }
        if( a.callers != b.callers )
        {
            return a.callers > b.callers;
        }
        const Symbol& sa = ing.symbols[ a.rep ];
        const Symbol& sb = ing.symbols[ b.rep ];
        const std::string_view pa = rootRelPath( ing, sa.fileId ), pb = rootRelPath( ing, sb.fileId );
        return pa != pb ? pa < pb : sa.line < sb.line;
    } );
    return out;
}

// One definition row: <d n= t= p= sel= [sel_proven="0"] lang= class= in= [bodyless="1"]/>. `compact` (the --path alternatives)
// keeps only what the next step needs: sel=, p=, class=.
inline void defRowXml( std::string& out, const IngestResult& ing, const DefLookup& L, const DefRow& r, const MemberOwners& owners, bool compact )
{
    std::vector<char> esc;
    const Symbol&     s = ing.symbols[ r.rep ];
    out += "<d";
    if( !compact )
    {
        out += " n=\"" + std::string( escapeXml( qualifiedMemberName( owners, s ), esc ) ) + "\" t=\"" + symTag( s.kind ) + "\"";
    }
    out += " p=\"" + std::string( escapeXml( rootRelPath( ing, s.fileId ), esc ) ) + ":" + std::to_string( s.line ) + "\"";
    out += " sel=\"" + std::string( escapeXml( r.sel, esc ) ) + "\"";
    if( !r.selProven )
    {
        out += " sel_proven=\"0\"";
    }
    if( !compact )
    {
        out += std::string( " lang=\"" ) + langTag( s.lang ) + "\"";
    }
    out += r.test ? " class=\"test\"" : " class=\"source\"";
    if( !compact )
    {
        out += " in=\"" + std::to_string( r.callers ) + "\"";
        if( L.groups.bodyless[ r.group ] )
        {
            out += " decl_only=\"1\"";
        }
    }
    out += "/>";
}

inline void defRowJson( std::string& out, const IngestResult& ing, const DefLookup& L, const DefRow& r, const MemberOwners& owners )
{
    const Symbol& s = ing.symbols[ r.rep ];
    out += "{\"n\":\"" + jsonStr( qualifiedMemberName( owners, s ) ) + "\",\"t\":\"" + symTag( s.kind ) + "\",\"p\":\""
         + jsonStr( rootRelPath( ing, s.fileId ) ) + ":" + std::to_string( s.line ) + "\",\"sel\":\"" + jsonStr( r.sel ) + "\"";
    if( !r.selProven )
    {
        out += ",\"sel_proven\":0";
    }
    out += std::string( ",\"lang\":\"" ) + langTag( s.lang ) + "\",\"class\":\"" + ( r.test ? "test" : "source" ) + "\",\"in\":"
         + std::to_string( r.callers );
    if( L.groups.bodyless[ r.group ] )
    {
        out += ",\"decl_only\":1";
    }
    out += "}";
}

// Is the pick a verb made among these definitions provably unique? Only when one group is left — the resolver's binding of a
// name with several groups is a choice, not a proof (attrib-multidef).
inline bool defPickProvablyUnique( const DefLookup& L ) noexcept { return L.defs() <= 1; }

// --path's inline alternatives for one endpoint (part 5): every OTHER group up to the cap, compact rows, then
// alts_shown=K defs=N next=--defs=NAME when more exist. Empty when the pick is provably unique (0 bytes added).
// `bound`: the definition the answer bound (kNoNode when it bound none: every group is then an alternative).
inline std::string defAltsXml( const IngestResult& ing, const DefLookup& L, NodeId bound, std::string_view side )
{
    if( defPickProvablyUnique( L ) )
    {
        return {};
    }
    std::uint32_t boundGroup = UINT32_MAX;
    for( std::size_t i = 0; i < L.matches.size(); ++i )
    {
        if( L.matches[i] == bound )
        {
            boundGroup = L.groups.ofMatch[i];
        }
    }
    const MemberOwners owners( ing );
    std::string        rows;
    std::size_t        shown = 0, total = 0;
    for( const DefRow& r : L.rows )
    {
        if( r.group == boundGroup )
        {
            continue;
        }
        ++total;
        if( shown < kDefRowsCap )
        {
            defRowXml( rows, ing, L, r, owners, /*compact=*/true );
            ++shown;
        }
    }
    std::vector<char> esc;
    std::string out = "<alts side=\"" + std::string( side ) + "\" of=\"" + std::string( escapeXml( L.spec, esc ) ) + "\"";
    if( shown < total )
    {
        out += " alts_shown=\"" + std::to_string( shown ) + "\" defs=\"" + std::to_string( L.defs() ) + "\""
             + nextAttrXml( nextFlag( "--defs=", L.spec ) );
    }
    return out + ">" + rows + "</alts>";
}

// Both --path transports (verbs_navigate.h runPath, mcpverbs.h pathText): the two endpoints' <alts>, from= first. A glob endpoint
// stands for every fitting definition by design (from_glob=), so it lists none.
struct PathAlts
{
    std::string xml;
    bool        any() const noexcept { return !xml.empty(); }
};
inline PathAlts pathAltsOf( const IngestResult& ing, const Graph& g, std::string_view srcN, std::string_view dstN, bool srcGlob, bool dstGlob,
                            NodeId srcBound, NodeId dstBound )
{
    PathAlts out;
    if( !srcGlob )
    {
        out.xml += defAltsXml( ing, lookupDefs( ing, g, srcN ), srcBound, "from" );
    }
    if( !dstGlob )
    {
        out.xml += defAltsXml( ing, lookupDefs( ing, g, dstN ), dstBound, "to" );
    }
    return out;
}

// The legend clause for <alts> — present only when an answer carries one.
inline const char* defAltsLegend( bool on ) noexcept
{
    return on ? "<alts side= of=> lists the OTHER definitions the endpoint's name binds (the pick is not provably unique): sel= pastes "
                "as that endpoint to bind one alone, p= where it is, class=source|test; alts_shown=K defs=N: K of the other rows shown, "
                "next= pages the rest. "
              : "";
}

}   // namespace rw
