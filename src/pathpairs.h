#pragma once
// pathpairs.h — TRACE-EP: which from=/to= DEFINITION PAIRS of a --path answer reach each other, printed, never one picked silently.
//
// THE DEFECT. --path=A,B with several definitions of A or B ran ONE multi-source BFS (graph.h shortestPathAny) and printed the
// first chain it met. Nothing said whether another pair also reached, so the answer could be about the wrong namesake with no
// sign of it: fzf `--path=Loop,extendedMatch` printed Terminal.Loop's 4 proven hops while Matcher.Loop (the one asked about) also
// reaches in 5; flake8 `--path=main,run_checks` printed Application.run_checks while FileChecker.run_checks is reached only
// through a hedged crossing. No reachability or hop-count key tells such pairs apart (the v1 rule "reachable first, then fewest
// hops" printed Terminal.Loop), so this file does not PICK: it lists every reaching pair up to kPathPairChains chains, ordered,
// and discloses the rest (alt= + next=).
//
// THE WALK. One reverse walk per to= definition group (a definition and the declarations tied to it, defattrib.h defGroupsOf)
// over the same out-edges the forward BFS follows (an in-edge is used only when the out-CSR holds it), cost = (hedged hops,
// hops) compared lexicographically — so every from= definition's best chain to that group is read off one walk, never one search
// per pair. Bound: at most kPathPairMaxDstGroups walks (more to= groups ⇒ today's answer, unchanged) + at most
// kPathPairBridgeSearches bridge searches (pathgaps.h pathBridge) for to= groups no chain reaches.
// ORDER: the shared reach rank (graph.h reachRankBefore — --connect's terminal pick uses the same one): fewest hedged hops
// (an all-proven chain always first), then fewest hops, then the tower order (the definitions' resolved order). Bridges come
// after every chain: a bridge is not a path.
// TODAY'S CHAIN IS KEPT: the pair shortestPathAny found prints the very chain it printed before, and is never pushed to alt=,
// so no hop a reader was given before can disappear.
// EXACTLY ONE pair reaching (no bridge) ⇒ the caller prints today's answer byte for byte; NONE reach ⇒ the caller's unreached
// answer (gaps, bridge, near_sink) is unchanged — this file is never consulted there.

#include "defattrib.h"   // defGroupsOf, pathHopSplitArm, pathHopOwners, emitPathHopRows — the one --path row emitter
#include "graph.h"       // edgeIndex, edgeNameOnly/edgeIsLink/edgeAnchorMiss/edgeViaImpl, reachRankBefore
#include "model.h"
#include "nextverb.h"    // nextFlag / nextAttrXml — the alt= continuation
#include "pathgaps.h"    // PathBridge, pathBridgeXml — a to= group no chain reaches may still have a hedged bridge
#include "sarif.h"       // rootRelativeUri — p= as the hop rows spell it
#include "serialize.h"   // escapeXml
#include "infra/emit.h"  // emitTo / emitRaw

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <queue>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace rw
{

inline constexpr std::size_t kPathPairChains         = 3;    // knob (knob-census ADDENDUM): chains printed; the rest are alt= rows
inline constexpr std::size_t kPathPairMaxDstGroups   = 16;   // knob: reverse walks per answer; more to= groups keep today's answer
inline constexpr std::size_t kPathPairBridgeSearches = 4;    // knob: bridge searches for to= groups no chain reaches
inline constexpr std::size_t kPathPairAltRows        = 12;   // knob: <alt> rows printed; alt= counts them all, alt_capped="1" says rows were cut
inline constexpr std::size_t kPathPairJoinMin        = 3;    // a shared tail of fewer rows is printed whole (no joins=): joining must save two rows

// A hop the answer hedges: by name alone (FE-B via="name"), a markdown link, a missing anchor, a split arm (prov="split") or the
// one-implementor edge (via="impl": an outside implementor may exist). Exactly the row marks emitPathHopRows prints.
inline bool pathHopHedged( const Graph& g, NodeId from, NodeId to ) noexcept
{
    return edgeNameOnly( g, from, to ) || edgeIsLink( g, from, to ) || edgeAnchorMiss( g, from, to ) || edgeSplitArm( g, from, to )
        || edgeViaImpl( g, from, to );
}
inline std::uint32_t pathChainHedged( const Graph& g, std::span<const NodeId> chain ) noexcept
{
    std::uint32_t h = 0;
    for( std::size_t i = 1; i < chain.size(); ++i )
    {
        h += pathHopHedged( g, chain[ i - 1 ], chain[i] ) ? 1u : 0u;
    }
    return h;
}

struct PathPair
{
    std::vector<NodeId> chain;            // from= definition … to= definition (a bridge: bridge.hops)
    PathBridge          bridge;           // found() only on a bridge pair
    std::uint32_t       hedged   = 0;     // hedged hops (a bridge: crossings + via="name" edges)
    std::size_t         srcGroup = 0, dstGroup = 0;
    std::size_t         joins    = 0;     // 1-based index of the earlier printed chain this one's tail repeats (0 = none)
    std::size_t         joinRow  = 0;     // the last row printed when joins > 0 (the row where it meets that chain)
    bool                today    = false; // the pair shortestPathAny found: printed as it was, never moved to alt=

    bool        isBridge() const noexcept { return bridge.found(); }
    std::size_t hops() const noexcept { return chain.empty() ? 0 : chain.size() - 1; }
};

struct PathPairs
{
    std::vector<PathPair> pairs;   // ordered; empty ⇒ inactive (the caller prints today's answer)
    bool active() const noexcept { return pairs.size() >= 2; }
};

// One reverse walk from the members of one to= group: per node, the lexicographic (hedged, hops) cost to reach the group and the
// next node toward it. Stops once every from= definition is settled. `dist` / `next` are reused across walks (caller-owned).
inline void pathPairReverseWalk( const Graph& g, std::span<const NodeId> dstMembers, std::span<const NodeId> srcNodes,
                                 std::vector<std::uint64_t>& dist, std::vector<NodeId>& next )
{
    constexpr std::uint64_t kInf = std::numeric_limits<std::uint64_t>::max();
    const std::size_t       N    = g.wOutDeg.size();
    EXPECTS( g.inEdges.rows() == N, "the caller checked the in-edge CSR covers every node" );
    dist.assign( N, kInf );
    next.assign( N, kNoNode );
    std::vector<char> isSrc( N, 0 ), done( N, 0 );
    std::size_t       srcLeft = 0;
    for( const NodeId s : srcNodes )
    {
        if( s < N && !isSrc[s] )
        {
            isSrc[s] = 1;
            ++srcLeft;
        }
    }
    using Item = std::pair<std::uint64_t, NodeId>;
    std::priority_queue<Item, std::vector<Item>, std::greater<Item>> pq;
    for( const NodeId d : dstMembers )
    {
        if( d < N && dist[d] != 0 )
        {
            dist[d] = 0;
            pq.emplace( 0, d );
        }
    }
    const auto* ro = g.inEdges.rowOffsets();
    const auto* ci = g.inEdges.colIndices();
    while( !pq.empty() && srcLeft > 0 )
    {
        const auto [ du, u ] = pq.top();
        pq.pop();
        if( done[u] || du != dist[u] )
        {
            continue;
        }
        done[u] = 1;
        srcLeft -= isSrc[u] ? 1u : 0u;
        for( std::uint32_t k = ro[u]; k < ro[ u + 1 ]; ++k )
        {
            const NodeId w = ci[k];   // w calls u
            if( w >= N || done[w] || edgeIndex( g, w, u ) == SIZE_MAX )
            {
                continue;   // only an edge the forward search follows is a hop here
            }
            const std::uint64_t dw = du + ( pathHopHedged( g, w, u ) ? ( std::uint64_t( 1 ) << 32 ) : 0 ) + 1;
            if( dw < dist[w] )
            {
                dist[w] = dw;
                next[w] = u;
                pq.emplace( dw, w );
            }
        }
    }
}

// The reaching pairs of a reached --path answer, or none (inactive) when the answer keeps today's bytes. `today` is
// shortestPathAny's chain (non-empty: the caller asks only on a reached answer). `bridgeFor( dstMembers )` runs the caller's
// bridge search from every from= definition to those to= members (the caller owns the value and backward-hop indexes).
template<class BridgeFor>
inline PathPairs pathReachingPairs( const IngestResult& ing, const Graph& g, std::span<const NodeId> srcDefs, std::span<const NodeId> dstDefs,
                                    std::span<const NodeId> today, BridgeFor&& bridgeFor )
{
    PathPairs out;
    const std::size_t N = g.wOutDeg.size();
    if( today.empty() || g.inEdges.rows() != N )
    {
        return out;
    }
    const DefGroups srcG = defGroupsOf( ing, srcDefs );
    const DefGroups dstG = defGroupsOf( ing, dstDefs );
    if( ( srcG.size() < 2 && dstG.size() < 2 ) || dstG.size() > kPathPairMaxDstGroups )
    {
        return out;   // one pair at most (nothing to list), or past the walk bound: today's answer
    }
    const auto groupOf = []( const DefGroups& gr, std::span<const NodeId> defs, NodeId n ) -> std::size_t
    {
        const auto it = std::find( defs.begin(), defs.end(), n );
        return it == defs.end() ? SIZE_MAX : gr.ofMatch[ std::size_t( it - defs.begin() ) ];
    };
    const std::size_t todaySrc = groupOf( srcG, srcDefs, today.front() );
    const std::size_t todayDst = groupOf( dstG, dstDefs, today.back() );

    std::vector<PathPair>      chains;
    std::vector<std::uint64_t> dist;
    std::vector<NodeId>        next;
    std::vector<std::size_t>   unreachedDst;
    for( std::size_t t = 0; t < dstG.size(); ++t )
    {
        std::vector<NodeId> members;
        for( std::size_t i = 0; i < dstDefs.size(); ++i )
        {
            if( dstG.ofMatch[i] == t )
            {
                members.push_back( dstDefs[i] );
            }
        }
        pathPairReverseWalk( g, members, srcDefs, dist, next );
        bool any = false;
        for( std::size_t s = 0; s < srcG.size(); ++s )
        {
            NodeId best = kNoNode;   // the group's member with the least cost; the first met on a tie (tower order)
            for( std::size_t i = 0; i < srcDefs.size(); ++i )
            {
                const NodeId n = srcDefs[i];
                if( srcG.ofMatch[i] == s && n < N && dist[n] != std::numeric_limits<std::uint64_t>::max() && ( best == kNoNode || dist[n] < dist[best] ) )
                {
                    best = n;
                }
            }
            if( best == kNoNode )
            {
                continue;
            }
            any = true;
            PathPair p;
            p.srcGroup = s;
            p.dstGroup = t;
            if( s == todaySrc && t == todayDst )
            {
                p.chain.assign( today.begin(), today.end() );   // today's chain, exactly as printed before
                p.today = true;
            }
            else
            {
                for( NodeId at = best; at != kNoNode && p.chain.size() <= N; at = next[at] )
                {
                    p.chain.push_back( at );
                }
            }
            p.hedged = pathChainHedged( g, p.chain );
            chains.push_back( std::move( p ) );
        }
        if( !any )
        {
            unreachedDst.push_back( t );
        }
    }
    const auto rankOf = []( const PathPair& p ) { return ReachRank{ 1, p.hedged, std::uint32_t( p.hops() ) }; };
    std::stable_sort( chains.begin(), chains.end(), [ & ]( const PathPair& a, const PathPair& b ) { return reachRankBefore( rankOf( a ), rankOf( b ) ); } );

    std::vector<PathPair> bridges;
    for( std::size_t i = 0; i < unreachedDst.size() && i < kPathPairBridgeSearches; ++i )
    {
        std::vector<NodeId> members;
        for( std::size_t k = 0; k < dstDefs.size(); ++k )
        {
            if( dstG.ofMatch[k] == unreachedDst[i] )
            {
                members.push_back( dstDefs[k] );
            }
        }
        PathPair p;
        p.bridge = bridgeFor( std::span<const NodeId>( members ) );
        if( !p.bridge.found() )
        {
            continue;
        }
        p.chain    = p.bridge.hops;
        p.dstGroup = unreachedDst[i];
        p.srcGroup = groupOf( srcG, srcDefs, p.chain.front() );
        std::uint32_t nameEdges = 0;
        for( std::size_t k = 1; k < p.bridge.hops.size(); ++k )
        {
            nameEdges += p.bridge.via[k] == BridgeVia::Edge && pathHopHedged( g, p.bridge.hops[ k - 1 ], p.bridge.hops[k] ) ? 1u : 0u;
        }
        p.hedged = std::uint32_t( p.bridge.crossings ) + nameEdges;
        bridges.push_back( std::move( p ) );
    }
    std::stable_sort( bridges.begin(), bridges.end(), [ & ]( const PathPair& a, const PathPair& b ) { return reachRankBefore( rankOf( a ), rankOf( b ) ); } );

    out.pairs = std::move( chains );
    for( PathPair& b : bridges )
    {
        out.pairs.push_back( std::move( b ) );
    }
    if( out.pairs.size() < 2 )
    {
        out.pairs.clear();   // exactly one pair reaches: today's answer, byte for byte
        return out;
    }
    // today's chain stays printed: past the K-th slot it takes the K-th (the others keep their order)
    const auto todayIt = std::find_if( out.pairs.begin(), out.pairs.end(), []( const PathPair& p ) { return p.today; } );
    if( todayIt != out.pairs.end() && std::size_t( todayIt - out.pairs.begin() ) >= kPathPairChains )
    {
        std::rotate( out.pairs.begin() + std::ptrdiff_t( kPathPairChains - 1 ), todayIt, todayIt + 1 );
    }
    // shared tails: a printed chain whose remaining rows are an earlier printed chain's stops at the row where it meets it
    const std::size_t shown = std::min( out.pairs.size(), kPathPairChains );
    for( std::size_t i = 1; i < shown; ++i )
    {
        PathPair& p = out.pairs[i];
        if( p.isBridge() )
        {
            continue;
        }
        for( std::size_t r = 0; r + kPathPairJoinMin <= p.chain.size() && p.joins == 0; ++r )
        {
            for( std::size_t j = 0; j < i && p.joins == 0; ++j )
            {
                const PathPair& q = out.pairs[j];
                const std::size_t tail = p.chain.size() - r;
                if( q.isBridge() || q.joins != 0 || q.chain.size() < tail )
                {
                    continue;
                }
                if( std::equal( p.chain.begin() + std::ptrdiff_t( r ), p.chain.end(), q.chain.end() - std::ptrdiff_t( tail ) ) )
                {
                    p.joins   = j + 1;
                    p.joinRow = r;
                }
            }
        }
    }
    ENSURES( out.pairs.size() >= 2 );
    return out;
}

// The legend clause, present only on an answer that lists pairs (no `--` inside: it rides in an XML comment).
inline constexpr std::string_view kPathPairsLegend =
    "pairs_reaching=N: N from=/to= definition pairs reach each other (directed call chains, then at most one hedged bridge per to= "
    "definition no chain reaches); each <pair from_p= to_p= hops= hedged=> is one pair's chain, fewest hedged hops first (0 = all "
    "proven), then fewest hops, then the definitions' order; hedged= counts its via=/prov=split/anchor=missing rows (a bridge: its "
    "crossings plus via=name edges), so hedged>0 is NOT a proven chain and its to_p= is not bound; the root's from_p=/to_p=/hops= "
    "are the first pair's. joins=J: from its last row the chain is pair J's (those rows are not repeated). bridge=1: a hedged "
    "bridge, NOT a path. alt=N: N more reaching pairs, each an <alt from_p= to_p= hops= hedged=> row; next= runs the first; "
    "alt_capped=1: only the first 12 rows print, qualify from= or to= as file:name to list the rest. ";
inline std::string_view pathPairsLegend( bool on ) noexcept
{
    return on ? kPathPairsLegend : std::string_view();
}

// One definition's selector for a continuation: `p::Scope::name` when it has a scope (the canonical-id grammar --path takes),
// else `p:name` — the p= its rows print. A same-file namesake with no scope stays ambiguous (the selector names the file).
inline std::string pathPairSelector( const IngestResult& ing, NodeId n, bool singleRoot, std::string_view rootPrefix )
{
    const Symbol&          s  = ing.symbols[n];
    const std::string_view rp = singleRoot ? sarif::rootRelativeUri( ing.files[ s.fileId ], rootPrefix ) : std::string_view( ing.files[ s.fileId ] );
    return s.scope.empty() ? std::string( rp ) + ":" + s.name : std::string( rp ) + "::" + s.scope + "::" + s.name;
}

struct PathPairsXml
{
    std::string rootAttrs;   // pairs_reaching= [alt= next=]
};

inline PathPairsXml pathPairsRootAttrs( const IngestResult& ing, const PathPairs& pp, bool singleRoot, std::string_view rootPrefix )
{
    PathPairsXml out;
    if( !pp.active() )
    {
        return out;
    }
    out.rootAttrs = " pairs_reaching=\"" + std::to_string( pp.pairs.size() ) + "\"";
    if( pp.pairs.size() > kPathPairChains )
    {
        const PathPair& a = pp.pairs[ kPathPairChains ];
        out.rootAttrs += " alt=\"" + std::to_string( pp.pairs.size() - kPathPairChains ) + "\""
                       + ( pp.pairs.size() - kPathPairChains > kPathPairAltRows ? " alt_capped=\"1\"" : "" )
                       + nextAttrXml( nextFlag( "--path=", pathPairSelector( ing, a.chain.front(), singleRoot, rootPrefix ) + ","
                                                           + pathPairSelector( ing, a.chain.back(), singleRoot, rootPrefix ) ) );
    }
    return out;
}

// The <pair> chains and <alt> rows, written where the single path's hop rows go (the CLI's stdout, MCP's memstream).
inline void emitPathPairs( std::FILE* out, const IngestResult& ing, const Graph& g, const PathPairs& pp, bool singleRoot, std::string_view rootPrefix )
{
    std::vector<char> esc;
    const auto loc = [ & ]( NodeId n ) -> std::string
    {
        const Symbol&          s  = ing.symbols[n];
        const std::string_view rp = singleRoot ? sarif::rootRelativeUri( ing.files[ s.fileId ], rootPrefix ) : std::string_view( ing.files[ s.fileId ] );
        return std::string( escapeXml( rp, esc ) ) + ":" + std::to_string( s.line );
    };
    const auto head = [ & ]( const PathPair& p ) -> std::string
    {
        return " from_p=\"" + loc( p.chain.front() ) + "\" to_p=\"" + loc( p.chain.back() ) + "\" hops=\"" + std::to_string( p.hops() )
             + "\" hedged=\"" + std::to_string( p.hedged ) + "\"" + ( p.isBridge() ? " bridge=\"1\"" : "" );
    };
    for( std::size_t i = 0; i < pp.pairs.size(); ++i )
    {
        const PathPair& p = pp.pairs[i];
        if( i >= kPathPairChains + kPathPairAltRows )
        {
            break;   // alt_capped="1" on the root says so; a file:name endpoint narrows the set
        }
        if( i >= kPathPairChains )
        {
            emitTo( out, "<alt{}/>", head( p ) );
            continue;
        }
        emitTo( out, "<pair{}{}>", head( p ), p.joins > 0 ? " joins=\"" + std::to_string( p.joins ) + "\"" : std::string() );
        if( p.isBridge() )
        {
            emitTo( out, "{}", pathBridgeXml( ing, g, p.bridge, singleRoot, rootPrefix ) );
        }
        else
        {
            const std::span<const NodeId>       rows   = std::span<const NodeId>( p.chain ).first( p.joins > 0 ? p.joinRow + 1 : p.chain.size() );
            const std::vector<std::string_view> owners = pathHopOwners( ing, rows );
            emitPathHopRows( out, ing, g, rows, owners, singleRoot, rootPrefix );
        }
        emitRaw( out, "</pair>" );
    }
}

// The legend flags of every printed chain (a via= row in chain 2 needs its clause as much as one in chain 1).
template<class Pred>
inline bool pathPairsAnyHop( const PathPairs& pp, Pred&& pred )
{
    for( std::size_t i = 0; i < pp.pairs.size() && i < kPathPairChains; ++i )
    {
        const PathPair& p = pp.pairs[i];
        if( !p.isBridge() && pred( std::span<const NodeId>( p.chain ).first( p.joins > 0 ? p.joinRow + 1 : p.chain.size() ) ) )
        {
            return true;
        }
    }
    return false;
}

}   // namespace rw
