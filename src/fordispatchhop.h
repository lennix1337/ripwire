#pragma once
// fordispatchhop.h — THE DISPATCH HOP of a name-exact --for answer (lane mf1-nx, wave 4): when the question names a call the
// served function makes through an interface-, trait- or abstract-typed receiver, the implementations that can run there.
//
// WHY. "When `Parser::Load` calls `file_reader_->ReadFile(...)`, which ReadFile runs?" routes name-exact and serves Parser::Load:
// its signature, its callers, its body. The call itself is absent from the body's <calls>: the resolver DECLINED it (the receiver
// is a `FileReader*`, an interface with four implementors; ifacedispatch.h, gate="iface") and stored exactly that candidate list,
// which only `--callees … --legend=full` printed, as file:line. Measured offline before this code (reports/mf1-nx.md Stage 0):
// the two MF1-DEV which-definition-runs rows (mdbook-02 `renderer.render`, ninja-02 `file_reader_->ReadFile`) gained 1 and 3
// required items from the candidate rows plus the sites that construct the candidates' types.
//
// WHAT. <dispatch call= in= p=file:line cands= shown= [test=] capped= next=> after the caller hop and before the bodies:
//   * one <s t= n= p=file:line> row per NON-TEST candidate method (the resolver's own declined list, in its order; a test double
//     is counted in test=, never shown — a test-path definition, or one whose type only test code names), at most kForDispatchHopRows;
//   * <s … ctor="Owner::new"> rows: the callers of each shown candidate owner's constructor (a member of the candidate's scope
//     named as the language spells a constructor — forfold.h isConstructorName), non-test, at most kForDispatchCtorRows in all —
//     where a candidate's type is made, which is where the choice is decided;
//   * the candidate's body when exactly ONE non-test candidate exists (the implementation production code can run there).
// Hedged by construction: the rows are the declined list, never an edge (which one runs is unproven) — nothing enters a reaches=
// set, and a call the resolver bound or declined for another reason (ladder, builtin, std) carries no element.
//
// WHEN. The name-exact route's auto <bodies> bundle, the DEFAULT regime and the XML dialect (the caller hop's posture, verbs_for.h);
// a body candidate that is a function or method made the declined call; and the TASK names that call: a task token spelling a
// member access (`recv.m`, `recv->m`, `Q::m`) whose last segment is the called name, and that name is not the calling function's
// own (a `render` wrapper calling `inner.render` stays as it was). The shape comes from the task text, never from a row id.
// RIPWIRE_NO_DISPATCH_HOP (any value) turns it off — the A/B handle.

#include "callhierarchy.h"   // oneHopCallerRows — the callers verb's rows
#include "declinedsites.h"   // declinedSitesMadeBy, orderDeclinedSites
#include "filter.h"          // isTestSymbol
#include "forcallerhop.h"    // appendCallerHopRow / CallerHopRowCtx — the caller hop's <s> row
#include "forfold.h"         // fold_detail::renderOneBody / isConstructorName
#include "graph.h"
#include "memberowner.h"
#include "model.h"
#include "nextverb.h"
#include "receiverevidence.h"   // ReceiverEvidence::callableKind
#include "sarif.h"           // rootRelativeUri
#include "serialize.h"       // escapeXml

#include <algorithm>
#include <cctype>
#include <optional>
#include <cstddef>
#include <cstdlib>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace rw
{

// Runaway guards, not size targets: every row left out is counted (cands=, test=, capped="1") and next= lists them all.
inline constexpr std::size_t kForDispatchHopRows  = 8;   // candidate rows per call
inline constexpr std::size_t kForDispatchCtorRows = 4;   // constructor-caller rows per call
inline constexpr std::size_t kForDispatchSites    = 2;   // calls per answer (a task naming more is a different question)

inline bool forDispatchHopOn()
{
    return std::getenv( "RIPWIRE_NO_DISPATCH_HOP" ) == nullptr;
}

inline constexpr std::string_view kForDispatchHopLegend =
    "; dispatch call=: the task names a call the served function in= makes at p=file:line through an interface, trait or abstract "
    "receiver, which the resolver declined: s t= n= p=file:line rows are the implementations that can run there (cands= in all, "
    "shown= of them, test= test doubles not shown (test code, or a type only test code names), which one runs is unproven, never an edge), s ctor= rows call that candidate "
    "type's constructor (where it is made), one shown candidate carries its body b, capped=1 if cut, next= lists every definition";
inline constexpr std::string_view kForCompactDispatchHopLegend =
    "; dispatch call= in= p=: a task-named call declined at an interface/trait receiver, s rows the implementations that can run "
    "(unproven which), cands= shown= test= doubles, s ctor= rows make that type, one candidate: its body b, capped=1 if cut, next= every definition";
static_assert( kForDispatchHopLegend.find( ';', 1 ) == std::string_view::npos && kForCompactDispatchHopLegend.find( ';', 1 ) == std::string_view::npos
                   && kForDispatchHopLegend.find( "--" ) == std::string_view::npos && kForCompactDispatchHopLegend.find( "--" ) == std::string_view::npos,
               "one clause each, and no \"--\": it rides inside an XML comment" );

namespace dispatch_detail
{
// The last segment of a task token that spells a member access or a qualified name (`recv.m`, `recv->m`, `Q::m`, `m(…)` after
// one of those); "" for a bare word — a bare word is prose ("the name"), not a named call.
inline std::string_view callSegment( std::string_view tok )
{
    if( const std::size_t paren = tok.find( '(' ); paren != std::string_view::npos )
    {
        tok = tok.substr( 0, paren );
    }
    while( !tok.empty() && !( std::isalnum( static_cast<unsigned char>( tok.back() ) ) || tok.back() == '_' ) )
    {
        tok.remove_suffix( 1 );
    }
    std::size_t cut = std::string_view::npos;
    for( const std::string_view sep : { std::string_view( "->" ), std::string_view( "::" ), std::string_view( "." ) } )
    {
        if( const std::size_t at = tok.rfind( sep ); at != std::string_view::npos && ( cut == std::string_view::npos || at + sep.size() > cut ) )
        {
            cut = at + sep.size();
        }
    }
    if( cut == std::string_view::npos || cut == 0 || cut >= tok.size() )
    {
        return {};
    }
    return tok.substr( cut );
}
inline std::vector<std::string_view> taskCallNames( std::string_view task )
{
    std::vector<std::string_view> out;
    std::size_t                   i = 0;
    while( i < task.size() )
    {
        while( i < task.size() && ( std::isspace( static_cast<unsigned char>( task[i] ) ) || task[i] == '`' ) )
        {
            ++i;
        }
        std::size_t j = i;
        while( j < task.size() && !std::isspace( static_cast<unsigned char>( task[j] ) ) && task[j] != '`' )
        {
            ++j;
        }
        if( const std::string_view seg = callSegment( task.substr( i, j - i ) ); !seg.empty() && std::ranges::find( out, seg ) == out.end() )
        {
            out.push_back( seg );
        }
        i = j;
    }
    return out;
}
inline std::span<const NodeId> siteCandidates( const Graph& g, std::uint32_t site )
{
    const std::uint32_t list = g.declinedSiteList[ site ];
    if( list == kNoDeclinedList || list + 1 >= g.declinedListOff.size() )
    {
        return {};
    }
    return std::span<const NodeId>( g.declinedListCand.data() + g.declinedListOff[ list ], g.declinedListOff[ list + 1 ] - g.declinedListOff[ list ] );
}
}   // namespace dispatch_detail

// The calls the element is about: declined gate="iface" sites with a stored list, made by a callable body candidate, whose called
// name the task names as a call and which is not the caller's own name. One per called name (a declaration and its definition, or
// two calls of one name, are one question), at most kForDispatchSites, in the declined rows' served order.
inline std::vector<std::uint32_t> forDispatchSites( const IngestResult& ing, const Graph& g, std::span<const NodeId> bodyCandidates, std::string_view task )
{
    std::vector<std::uint32_t> out;
    const std::vector<std::string_view> names = dispatch_detail::taskCallNames( task );
    if( names.empty() || bodyCandidates.empty() )
    {
        return out;
    }
    std::vector<NodeId> callers;
    for( const NodeId id : bodyCandidates )
    {
        if( id < ing.symbols.size() && ReceiverEvidence::callableKind( ing.symbols[id].kind ) )
        {
            callers.push_back( id );
        }
    }
    std::vector<std::uint32_t> sites = declinedSitesMadeBy( g, callers );
    orderDeclinedSites( ing, g, sites, /*fileFirst=*/false );
    std::vector<std::string_view> taken;
    for( const std::uint32_t site : sites )
    {
        const Reference& r = ing.references[ g.declinedSiteRef[ site ] ];
        const std::string_view called = r.calleeName;
        if( g.declinedSiteKind[ site ] != DeclinedSiteKind::Iface || dispatch_detail::siteCandidates( g, site ).empty()
            || std::ranges::find( names, called ) == names.end() || ing.symbols[ g.declinedSiteCaller[ site ] ].name == called
            || std::ranges::find( taken, called ) != taken.end() )
        {
            continue;
        }
        taken.push_back( called );
        out.push_back( site );
        if( out.size() == kForDispatchSites )
        {
            break;
        }
    }
    return out;
}

struct DispatchHopRender
{
    std::string xml;              // <dispatch …>…</dispatch> per site; empty when nothing rides
    std::size_t bodyBytes = 0;    // the body elements' bytes inside xml (the body rate); the rest is markup
};

// The constructors of `cand`'s owner: callables of the same scope whose name the language spells a constructor (forfold.h).
inline std::vector<NodeId> dispatchOwnerCtors( const IngestResult& ing, NodeId cand )
{
    std::vector<NodeId> out;
    const Symbol&       c = ing.symbols[ cand ];
    if( c.scope.empty() )
    {
        return out;
    }
    const std::size_t      sep   = c.scope.find_last_of( ".:" );
    const std::string_view owner = sep == std::string::npos ? std::string_view( c.scope ) : std::string_view( c.scope ).substr( sep + 1 );
    for( NodeId id = 0; id < ing.symbols.size(); ++id )
    {
        const Symbol& s = ing.symbols[id];
        if( id != cand && s.lang == c.lang && s.scope == c.scope && ReceiverEvidence::callableKind( s.kind ) && fold_detail::isConstructorName( s.name, owner ) )
        {
            out.push_back( id );
        }
    }
    return out;
}

// Does any non-test code outside the candidate's own type name that type? A reference (call, type use, member type, base clause,
// qualifier) spelling the owner's name, from a file holding none of the owner's members and from a non-test symbol in a non-test
// file. By name only, so a collision keeps a candidate shown (never the other way): a candidate whose type test code names and
// nothing else does (ninja's VirtualFileSystem, defined in src/test.cc and used by *_test.cc only) is a test double, counted in
// test=; a type nothing outside its own files names (a Rust unit struct built in place) keeps its row — no evidence, no demotion.
inline bool ownerNamedByProduction( const IngestResult& ing, NodeId cand )
{
    const Symbol& c = ing.symbols[ cand ];
    if( c.scope.empty() )
    {
        return true;   // a free function: no type to ask about
    }
    const std::size_t      sep   = c.scope.find_last_of( ".:" );
    const std::string_view owner = sep == std::string::npos ? std::string_view( c.scope ) : std::string_view( c.scope ).substr( sep + 1 );
    std::vector<std::uint32_t> ownFiles;
    for( const Symbol& s : ing.symbols )
    {
        if( ( s.scope == c.scope || ( s.name == owner && s.lang == c.lang && ReceiverEvidence::classLike( s.kind ) ) ) && std::ranges::find( ownFiles, s.fileId ) == ownFiles.end() )
        {
            ownFiles.push_back( s.fileId );
        }
    }
    bool testNames = false;
    for( const Reference& r : ing.references )
    {
        if( ( r.calleeName != owner && r.qualifier != owner ) || r.isDocLink || std::ranges::find( ownFiles, r.fileId ) != ownFiles.end() )
        {
            continue;
        }
        if( isTestPath( rootRelPath( ing, r.fileId ) ) || ( r.fromSymbol != kNoNode && isTestSymbol( ing, r.fromSymbol ) ) )
        {
            testNames = true;
            continue;
        }
        return true;
    }
    return !testNames;   // named by nothing outside its own files: no evidence either way, so it stays a candidate
}

inline DispatchHopRender renderForDispatchHop( const IngestResult& ing, const Graph& g, std::span<const std::uint32_t> sites, const BodyFoldInputs& bodyIn )
{
    DispatchHopRender     out;
    std::vector<char>     esc;
    const MemberOwners    owners( ing );
    const CallerHopRowCtx rc{ ing, owners, bodyIn.rootArg, esc };
    for( const std::uint32_t site : sites )
    {
        const Reference&              r     = ing.references[ g.declinedSiteRef[ site ] ];
        const std::span<const NodeId> cands = dispatch_detail::siteCandidates( g, site );
        std::vector<NodeId>           prod;
        std::size_t                   tests = 0;
        for( const NodeId c : cands )
        {
            if( isTestSymbol( ing, c ) || !ownerNamedByProduction( ing, c ) )
            {
                ++tests;
            }
            else
            {
                prod.push_back( c );
            }
        }
        if( prod.empty() )
        {
            continue;   // only test doubles: nothing production code runs to name
        }
        const std::size_t shown = std::min( prod.size(), kForDispatchHopRows );
        // where each shown candidate's type is made: the callers of its constructors, non-test, deduped, capped over the call
        std::vector<std::pair<NodeId, NodeId>> made;   // (caller, ctor)
        std::size_t                            madeTotal = 0;
        for( std::size_t i = 0; i < shown; ++i )
        {
            for( const NodeId ctor : dispatchOwnerCtors( ing, prod[i] ) )
            {
                const std::vector<NodeId> ctorTargets{ ctor };
                for( const NodeId caller : oneHopCallerRows( ing, g, ctorTargets ) )
                {
                    if( isTestSymbol( ing, caller ) || std::ranges::any_of( made, [ & ]( const auto& m ) { return m.first == caller; } ) )
                    {
                        continue;
                    }
                    ++madeTotal;
                    if( made.size() < kForDispatchCtorRows )
                    {
                        made.emplace_back( caller, ctor );
                    }
                }
            }
        }
        const Symbol&          caller = ing.symbols[ g.declinedSiteCaller[ site ] ];
        const std::string_view rp     = bodyIn.rootArg.empty() ? std::string_view( ing.files[ r.fileId ] ) : sarif::rootRelativeUri( ing.files[ r.fileId ], bodyIn.rootArg );
        std::string            x;
        x += "<dispatch call=\"";
        x += escapeXml( r.recvVar.empty() ? std::string( r.calleeName ) : r.recvVar + ( r.viaArrow ? "->" : "." ) + r.calleeName, esc );
        x += "\" in=\"";
        x += escapeXml( qualifiedMemberName( owners, caller ), esc );
        x += "\" p=\"";
        x += escapeXml( rp, esc );
        x += ':' + std::to_string( r.line ) + "\" cands=\"" + std::to_string( cands.size() ) + "\" shown=\"" + std::to_string( shown ) + '"';
        if( tests > 0 )
        {
            x += " test=\"" + std::to_string( tests ) + '"';
        }
        x += " capped=\"";
        x += ( shown < prod.size() || made.size() < madeTotal ) ? "1" : "0";
        x += '"';
        x += nextAttrXml( nextFlag( "--whereis=", r.calleeName ) );
        x += '>';
        for( std::size_t i = 0; i < shown; ++i )
        {
            appendCallerHopRow( x, rc, prod[i], {} );
        }
        for( const auto& [ who, ctor ] : made )
        {
            appendCallerHopRow( x, rc, who, std::string( " ctor=\"" ) + std::string( escapeXml( qualifiedMemberName( owners, ing.symbols[ ctor ] ), esc ) ) + '"' );
        }
        if( prod.size() == 1 )
        {
            if( const std::optional<std::string> body = fold_detail::renderOneBody( bodyIn, prod.front() ) )
            {
                x += *body;
                out.bodyBytes += body->size();
            }
        }
        x += "</dispatch>";
        out.xml += x;
    }
    return out;
}

}   // namespace rw
