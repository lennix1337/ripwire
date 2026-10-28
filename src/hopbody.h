#pragma once
// hopbody.h — THE BODY OF A MIDDLE HOP (lane path-hop-body, train 41): two answers that name a function and stop short of what it does.
//
// WHY. A reached --path answer prints each hop as a <s t= n= p=> row and nothing else. When the path is short, the question is often
// what happens INSIDE the middle hop: hono's `Hono.fetch -> Hono.#dispatch -> compose` names #dispatch, and the whole request flow
// (path extraction, router match, Context, the one-handler fast path, compose, error handling) lives in its body, which the answer
// never printed (readout-40 G: 0 of 7 items; the same body through --expand: 6 of 7).
//
// WHAT. (1) PATH-HOP-BODY: a reached --path whose shown chain has at most kPathHopBodyMaxHops hops prints, after its hop rows, the body
// of each INTERMEDIATE hop (never an endpoint, never one whose text the answer already carries: fold_detail::overlapsServed) inside <bodies of="hops" shown= total= capped=>,
// each <b> the --expand body element (no callee rows, no file context). A chain past the gate prints no body and the root carries
// next= (the first middle hop's --expand) unless another continuation already owns next=.
// THE BOUND IS A SPEW GUARD, NEVER A COMPLETENESS CUT (owner rule): each body is whole up to kHopBodyCapBytes; a longer one is cut at a
// line end by packBodies' own truncation, which says so on the element (truncated="1" lines="1-K/T" next= the call serving the rest).

#include "model.h"
#include "nextverb.h"   // nextFlag
#include "sarif.h"      // rootPrefixOf / rootRelativeUri — the --expand selector's path
#include "serialize.h"  // packBodies, chargeSection, escapeXml

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace rw
{

// The per-body spew guard (bytes of source text): whole below it, cut at a line end and disclosed above it. Measured over the in-sample
// --path rows (reports/path-hop-body.md): the bodies these answers need are 0.1-3.4 KB.
inline constexpr std::size_t kHopBodyCapBytes = 4000;
// The hop-count gate: a chain of at most this many hops prints its middle hop's body (kPathHopBodyMaxHops - 1 = one body). Measured
// (reports/path-hop-body.md, the curve over the in-sample --path rows): 2 hops buys hono-12 its 6 items; 3 hops adds flake8-14
// (+1.6 KB, already complete), 4 hops fzf-14 and tmux-14 (+13 KB, both already complete) and nothing else; past the gate next= names the body.
inline constexpr std::size_t kPathHopBodyMaxHops = 2;

namespace fold_detail
{
// does a's text lie inside b's: the same file and b's span holds a's (two equal spans: the later id is the inner one). A body
// inside another the answer serves is already there — the fold serves each text once (FIX1 N2).
inline bool spanInside( const IngestResult& ing, NodeId a, NodeId b ) noexcept
{
    const Symbol& x = ing.symbols[a];
    const Symbol& y = ing.symbols[b];
    return a != b && x.fileId == y.fileId && y.sigStartByte <= x.sigStartByte && x.endByte <= y.endByte
        && ( y.sigStartByte < x.sigStartByte || x.endByte < y.endByte || b < a );
}
// does a's text overlap a body in `ids` the answer already serves: a is one of them or lies inside one, or (orHolds) holds one
inline bool overlapsServed( const IngestResult& ing, NodeId a, std::span<const NodeId> ids, bool orHolds ) noexcept
{
    for( const NodeId b : ids )
    {
        if( a == b || spanInside( ing, a, b ) || ( orHolds && spanInside( ing, b, a ) ) )
        {
            return true;
        }
    }
    return false;
}
}   // namespace fold_detail

// The A/B switch (the RIPWIRE_NO_BODY_FOLD precedent): the hop bodies ride unless RIPWIRE_NO_HOP_BODY is set.
inline bool hopBodiesOn()
{
    return std::getenv( "RIPWIRE_NO_HOP_BODY" ) == nullptr;
}

// One body through packBodies (the --expand body shape, no callee rows: the empty CSR has no out-edge for any id), unwrapped from its
// <bodies> envelope; the legend comments packBodies writes ahead of or inside the envelope stay in front of the <b>. capBytes 0 =
// whole; else a body over it is cut at a line end with truncated="1" lines= next= (packBodies' first-body truncation). nullopt when
// the buffer failed or the span rendered nothing.
inline std::optional<std::string> renderBodyElement( const IngestResult& ing, NodeId id, std::size_t capBytes, bool compress, RedactCounts* redact,
                                                     std::string_view rootArg, const std::vector<float>* rank )
{
    static const std::vector<std::uint32_t> kNoOff;
    static const std::vector<NodeId>        kNoTargets;
    static const std::vector<std::uint8_t>  kNoNameOnly;
    ChargedSection sec = chargeSection( [ & ]( std::FILE* f )
        { packBodies( f, ing, { id }, capBytes, kNoOff, kNoTargets, CalleeEdgeBytes( kNoNameOnly ), compress, redact,
                      /*ranges=*/nullptr, /*notes=*/nullptr, /*outEmitted=*/nullptr, /*truncateOversizedFirst=*/capBytes > 0,
                      /*withFileContext=*/false, rootArg, rank, {} ); },
        kBytesPerTokenBody );
    if( !sec.isRendered )
    {
        return std::nullopt;
    }
    std::string&      x     = sec.xml;
    const std::size_t open  = x.find( "<bodies shown=" );
    const std::size_t close = open == std::string::npos ? std::string::npos : x.find( '>', open );
    constexpr std::string_view kClose = "</bodies>";
    if( close == std::string::npos || !x.ends_with( kClose ) || x.find( "<b t=\"", close ) == std::string::npos )
    {
        return std::nullopt;   // an unreadable span renders an empty envelope: nothing to serve
    }
    return x.substr( 0, open ) + x.substr( close + 1, x.size() - kClose.size() - close - 1 );
}

// The --expand selector of one definition, as the answer spells p= (root-relative on a single root), unescaped.
inline std::string expandSelectorOf( const IngestResult& ing, NodeId id, std::string_view rootArg )
{
    const Symbol&     s          = ing.symbols[id];
    const std::string rootPrefix = rootArg.empty() ? std::string() : sarif::rootPrefixOf( rootArg );
    const std::string path       = rootArg.empty() ? std::string( ing.files[s.fileId] ) : std::string( sarif::rootRelativeUri( ing.files[s.fileId], rootPrefix ) );
    return nextFlag( "--expand=", path + ":" + std::to_string( s.line ) + ":" + s.name );
}

// THE PATH'S MIDDLE HOPS: the intermediate rows of `chain` (never an endpoint), in chain order, deduped, minus `served` (a body the
// enclosing answer already carries). Empty when the chain is past the hop gate or has no middle.
inline std::vector<NodeId> pathMiddleHops( const IngestResult& ing, std::span<const NodeId> chain, std::span<const NodeId> served )
{
    std::vector<NodeId> out;
    if( chain.size() < 3 || chain.size() - 1 > kPathHopBodyMaxHops )
    {
        return out;
    }
    for( std::size_t i = 1; i + 1 < chain.size(); ++i )
    {
        const NodeId n = chain[i];
        if( n < ing.symbols.size() && ing.symbols[n].kind != SymKind::ModuleScope && ing.symbols[n].endByte > ing.symbols[n].sigStartByte
            && std::ranges::find( out, n ) == out.end() && !fold_detail::overlapsServed( ing, n, served, /*orHolds=*/true ) )
        {
            out.push_back( n );
        }
    }
    return out;
}

// next= for a reached chain past the hop gate: the first middle hop's --expand (unescaped), "" when the chain has no middle or is
// within the gate (its bodies print).
inline std::string pathMiddleHopNext( const IngestResult& ing, std::span<const NodeId> chain, std::string_view rootArg )
{
    if( chain.size() < 3 || chain.size() - 1 <= kPathHopBodyMaxHops )
    {
        return {};
    }
    return expandSelectorOf( ing, chain[1], rootArg );
}

// THE PATH'S <bodies of="hops"> ELEMENT: every middle hop's body, each whole up to kHopBodyCapBytes; "" when there is none.
inline std::string pathHopBodiesXml( const IngestResult& ing, std::span<const NodeId> hops, std::string_view rootArg )
{
    std::string children;
    std::size_t shown = 0;
    bool        cut   = false;
    for( const NodeId n : hops )
    {
        if( const std::optional<std::string> b = renderBodyElement( ing, n, kHopBodyCapBytes, /*compress=*/false, /*redact=*/nullptr, rootArg, nullptr ) )
        {
            children += *b;
            ++shown;
            cut = cut || b->find( " truncated=\"1\"" ) != std::string::npos;
        }
    }
    if( shown == 0 )
    {
        return {};
    }
    return "<bodies of=\"hops\" shown=\"" + std::to_string( shown ) + "\" total=\"" + std::to_string( hops.size() ) + "\" capped=\""
         + ( cut || shown < hops.size() ? "1" : "0" ) + "\">" + children + "</bodies>";
}

// The full dialect's clause for <bodies of="hops">, present-only (no "--": it rides inside the path's XML comment).
static_assert( kHopBodyCapBytes == 4000 && kPathHopBodyMaxHops == 2, "the path and fold clauses spell both bounds: re-word them with the knob" );
inline std::string_view pathHopBodiesLegend( bool on ) noexcept
{
    return on ? "bodies of=hops: the body of the middle hop of the shown chain (never an endpoint), when it has 2 hops; "
                "b t= l= p= n= as expand prints it (no callee rows), whole up to 4000 bytes (a longer one is cut, truncated=1). "
              : "";
}
inline std::string_view pathHopNextLegend( bool on ) noexcept
{
    return on ? "next=: the chain has more than 2 hops, so no middle hop body prints; the first middle hop's expand. " : "";
}

}   // namespace rw
