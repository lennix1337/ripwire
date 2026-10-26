#pragma once
// backhops.h — HOP1: ONE backward hop provider for a sink, read by three answers (lane hop1).
//
// THE GAP. `--callers=X` already names, one hop at a time, who MAY call X without a graph edge: a <vr via="slot"> row's
// called_by= (a call through a struct field X was stored in, IX-1) and a <dc> row (a declined call whose stored candidate
// list holds X). The answers that join hops — --path's hedged bridge, --path's root next= and --impact's blast radius —
// ignored those rows: tmux `--path=cmdq_fire_command,window_copy_cmd_cancel` printed no chain and `--impact` said
// reaches="0" beside its own slot row, while three `--callers` calls walk the exact chain.
//
// THE CONTRACT. BackwardHops::backwardHops( sink ) returns the sink's backward hops of two kinds, each KNOWN, never guessed:
//   slot / slot-name  the functions the value-ref slot join names as calling through a struct field the sink is stored in
//                     (valuerefindex.h slotCallers, the --callers rows' own rule: same-type tier alone when it has a hit,
//                     else the field-name tier; one per calling function); width = how many functions that row names;
//   declined          the callers of declined calls whose stored candidate list holds the sink (graph.h declinedSitesNaming's
//                     list membership) — only a call with a stored list, not a self-call, and not one that reads as the
//                     builtin method its name spells (DeclinedCrossRule); width = the list's candidate count.
// A consumer applies its own width bound (kBackHopWidth: a wider set is a hedge it does not follow, never a hop).
//
// What it does NOT mean: a hop is NOT a call edge and NOT proven. Every consumer prints it as a hedge (proven="0", via=
// named, the width beside it) and never counts it in reachable= / hops= / reaches=.

#include "graphlegend.h"        // countFieldOrEmpty — reaches_hedged= absent at zero
#include "graph.h"              // Graph — the declined records and their stored candidate lists
#include "model.h"
#include "nextverb.h"           // nextAttrXml / nextFlag — the ring's continuation
#include "resolve.h"            // includerDir — the ring order's same-directory key
#include "receiverevidence.h"   // ReceiverEvidence::chainOf / classLike — the receiver chain and the class kinds, one test each
#include "serialize.h"          // escapeXml / jsonStr
#include "valuerefindex.h"      // ValueRefIndex, slotCallers — the slot join and its tier rule
#include "valuerefs.h"          // VrRender / vrPath — p= spelled as every other row
#include "infra/Diagnostics.h"  // EXPECTS
#include "infra/sortutil.h"     // svLess
#include "infra/enumcount.h"    // enumCountIsExact — kHopKinds has one row per BackHopVia value

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace rw
{

// The widest backward set a consumer follows as a hop: the same 16 the bridge allows a declined fan (pathgaps.h
// kBridgeFanOut reads this). A wider slot or declined set is a hedge a consumer does not follow.
inline constexpr std::size_t kBackHopWidth = 16;

// A member call spelled like a builtin type's method (`ids.join( "," )`, `keys.push( k )`, `d.get( k )`, Ruby `x.to_s`) — the
// builtin-method gate's per-language tables (graph.h BuiltinMethodGate::tableFor) — is far more often the builtin than an in-tree
// candidate the resolver declined between: the acceptance hand-check rejected every such declined site as a way in or a crossing.
inline bool builtinSpelledCall( const Reference& call ) noexcept
{
    const std::span<const std::string_view> table = BuiltinMethodGate::tableFor( call.lang );
    return call.qualifier.empty() && std::binary_search( table.begin(), table.end(), std::string_view( call.calleeName ), sortutil::svLess );
}

// #11 (HOP1 Stage 0, hono `this.router.match( method, path )`): builtinSpelledCall reads the NAME alone, so a declined call on a
// receiver the source TYPES as an in-tree class was dropped as "probably String.prototype.match". DeclinedCrossRule keeps the
// name rule and lifts it for exactly one shape, all of whose facts the index holds:
//   (1) the record is a Ladder decline (graph.h DeclinedSiteKind::Ladder): the resolver's own builtin-method gate ran on this call
//       and ADMITTED in-tree candidates — it did not decline it as the builtin (that is DeclinedSiteKind::Builtin);
//   (2) its stored candidate list is complete: kept, two or more candidates, at most kBackHopWidth;
//   (3) the receiver is ONE field `f` of an object whose class the index knows: the enclosing class's own instance — `this.f`
//       (JS/TS) or `self.f` (Python, Ruby), the language's keyword only (checklist 37: a local named `self` in JS is an ordinary
//       name) — or a parameter or local of the calling function whose class the source states (`app: Hono<E>`, a
//       LocalBindKind::RecvType record, unqualified, one spelling), standing for EVERY in-tree class definition of that name;
//   (4) EVERY such class states f's class (a LocalBindKind::MemberType record, `router!: Router<…>`, `this.f = new Foo()`; one
//       spelling per class, two decide nothing) as the name of an in-tree class-like definition.
// What it does NOT mean: the call is not proven to reach any candidate; it is only no longer read as the builtin. A `.match` on a
// receiver with no stated class (`path.match( re )`), on a field typed `string`, on a field one same-named class types outside
// the tree or not at all, or on a field only a base class states (bases are not walked) stays skipped.
class DeclinedCrossRule
{
public:
    DeclinedCrossRule( const IngestResult& ing, const Graph& g ) : m_ing( ing ), m_g( g ) {}

    // Whether declined record `site` reads as the builtin method its name spells: no crossing or way in goes through it.
    bool builtinSpelled( std::uint32_t site ) const
    {
        EXPECTS( site < m_g.declinedSiteRef.size(), "a declined record index from the graph's own record list" );
        const Reference& r = m_ing.references[ m_g.declinedSiteRef[ site ] ];
        return builtinSpelledCall( r ) && !receiverTypedInTree( site, r );
    }

private:
    struct ClassName
    {
        std::string_view name;
        NodeId           id = kNoNode;
    };
    struct ClassSpan
    {
        std::uint32_t fileId = 0;
        std::uint32_t begin  = 0;
        std::uint32_t end    = 0;
        NodeId        id     = kNoNode;
    };

    // The class definition that owns definition `d`: `d` itself when class-like, else the innermost class-like definition of its
    // file whose span holds it (ReceiverEvidence's containment rule); kNoNode when none (a free function, a C++ out-of-line member).
    NodeId ownerOf( NodeId d ) const
    {
        const Symbol& s = m_ing.symbols[ d ];
        if( ReceiverEvidence::classLike( s.kind ) )
        {
            return d;
        }
        const auto       lo   = std::ranges::lower_bound( m_classes, s.fileId, {}, &ClassSpan::fileId );
        const ClassSpan* best = nullptr;
        for( auto it = lo; it != m_classes.end() && it->fileId == s.fileId; ++it )
        {
            if( it->id != d && it->begin <= s.sigStartByte && s.sigStartByte < it->end && ( best == nullptr || it->end - it->begin < best->end - best->begin ) )
            {
                best = &*it;
            }
        }
        return best != nullptr ? best->id : kNoNode;
    }

    // The receiver keyword of the four languages builtinSpelledCall's tables cover; any other language, or another spelling, is
    // an ordinary name (checklist 37).
    static bool selfKeyword( Lang lang, std::string_view root ) noexcept
    {
        static constexpr std::pair<Lang, std::string_view> kSelf[] = {
            { Lang::JavaScript, "this" }, { Lang::TypeScript, "this" }, { Lang::Python, "self" }, { Lang::Ruby, "self" } };
        return std::ranges::find( kSelf, std::pair( lang, root ) ) != std::end( kSelf );
    }
    // One stated class per key (graph.h jsImportKey's "id#name", here a definition id): a second, different spelling leaves "" (decides nothing), as ReceiverEvidence's tombstones do.
    static void noteOnce( HashMap<std::string, std::string_view>& m, std::string key, std::string_view type )
    {
        const auto [ it, fresh ] = m.try_emplace( std::move( key ), type );
        if( !fresh && it->second != type )
        {
            it->second = {};
        }
    }
    static std::string_view lookup( const HashMap<std::string, std::string_view>& m, NodeId id, std::string_view name )
    {
        const auto it = m.find( jsImportKey( id, name ) );
        return it == m.end() ? std::string_view() : it->second;
    }

    bool receiverTypedInTree( std::uint32_t site, const Reference& r ) const
    {
        if( m_g.declinedSiteKind[ site ] != DeclinedSiteKind::Ladder )
        {
            return false;   // (1) the gate declined it as the builtin, or another rule declined it
        }
        const std::uint32_t list = m_g.declinedSiteList[ site ];
        if( list == kNoDeclinedList || list + 1 >= m_g.declinedListOff.size() )
        {
            return false;   // (2) no stored list: nothing says the set is complete
        }
        const std::size_t fan = m_g.declinedListOff[ list + 1 ] - m_g.declinedListOff[ list ];
        if( fan < 2 || fan > kBackHopWidth )
        {
            return false;   // (2) a single or too-wide set
        }
        const ReceiverEvidence::Chain ch     = ReceiverEvidence::chainOf( r );
        const NodeId                  caller = m_g.declinedSiteCaller[ site ];
        if( !ch.valid || ch.root.empty() || ch.path.empty() || ch.path.find( '.' ) != std::string_view::npos || caller >= m_ing.symbols.size() )
        {
            return false;   // (3) not one field of a named receiver
        }
        buildOnce();
        std::vector<NodeId> owners;   // (3) the class definitions the receiver object may be an instance of
        if( selfKeyword( r.lang, ch.root ) )
        {
            if( const NodeId own = ownerOf( caller ); own != kNoNode )
            {
                owners.push_back( own );
            }
        }
        else if( const std::string_view cls = lookup( m_recvTypes, caller, ch.root ); !cls.empty() )
        {
            for( auto it = std::ranges::lower_bound( m_classIds, cls, sortutil::svLess, &ClassName::name ); it != m_classIds.end() && it->name == cls; ++it )
            {
                owners.push_back( it->id );
            }
        }
        return !owners.empty() && std::ranges::all_of( owners, [ & ]( NodeId owner )
        {
            // (4) this class states one in-tree class for the field; stating none (a primitive, a base's field), two spellings, or
            // a class the tree does not define leaves the builtin reading standing
            const std::string_view type = lookup( m_fields, owner, ch.path );
            return !type.empty() && std::ranges::binary_search( m_classNames, type, sortutil::svLess );
        } );
    }

    // Built on the first receiver shape that needs it: a tree with no such call pays nothing.
    void buildOnce() const
    {
        if( m_built )
        {
            return;
        }
        m_built = true;
        for( NodeId id = 0; id < m_ing.symbols.size(); ++id )
        {
            if( const Symbol& s = m_ing.symbols[ id ]; ReceiverEvidence::classLike( s.kind ) && isDefinitionNotDeclaration( s ) )
            {
                m_classNames.push_back( s.name );
                m_classIds.push_back( ClassName{ s.name, id } );
                m_classes.push_back( ClassSpan{ s.fileId, s.sigStartByte, s.endByte, id } );
            }
        }
        std::ranges::stable_sort( m_classes, {}, &ClassSpan::fileId );
        std::ranges::stable_sort( m_classIds, sortutil::svLess, &ClassName::name );
        std::ranges::sort( m_classNames, sortutil::svLess );
        m_classNames.erase( std::ranges::unique( m_classNames ).begin(), m_classNames.end() );
        for( const Binding& b : m_ing.bindings )
        {
            if( ( b.kind != LocalBindKind::MemberType && b.kind != LocalBindKind::RecvType ) || b.var.empty() || b.typeName.empty() || !b.importedName.empty()
                || b.fromSymbol >= m_ing.symbols.size() )
            {
                continue;   // a qualified spelling (`ext.Router`) names no class of this tree for certain: left out
            }
            if( b.kind == LocalBindKind::RecvType )
            {
                noteOnce( m_recvTypes, jsImportKey( b.fromSymbol, b.var ), b.typeName );   // keyed by the function that states it
            }
            else if( const NodeId owner = ownerOf( b.fromSymbol ); owner != kNoNode )
            {
                noteOnce( m_fields, jsImportKey( owner, b.var ), b.typeName );
            }
        }
    }

    const IngestResult&                    m_ing;
    const Graph&                           m_g;
    mutable bool                           m_built = false;
    mutable HashMap<std::string, std::string_view> m_fields;   // "classId#field" → the one stated class ("" two spellings)
    mutable std::vector<std::string_view>  m_classNames;   // every in-tree class-like definition name, sorted, distinct
    mutable std::vector<ClassSpan>         m_classes;      // every class-like definition's span, by file (ownerOf)
    mutable std::vector<ClassName>         m_classIds;     // every class-like definition by name (a stated class's definitions)
    mutable HashMap<std::string, std::string_view> m_recvTypes;   // "functionId#name" → its parameter's or local's stated class
};

// ── the provider ─────────────────────────────────────────────────────────────────────────────────────────────────────
// THE HOP KINDS, as data (composed hops). Each kind is one row of kHopKinds: its printed name, whether it is a call through a
// struct FIELD (a chain of field calls only may go one hop deeper: the bridge's kBridgeSlotCrossings, the ring's
// kImpactRingSlotDepth), and whether the --impact ring follows it (a declined hop is spoken for by declined_calls= / <dc>).
// A new indirection shape (QUEUE-HOP's put/get pairing) is an appended enum value, a row here and one producer in
// backwardHops / forwardHops; every walk then reads it through the same span with no walk-side change.
//   container  (composed hops) a call through a container element (`handlers[i]()`) or a parameter's default
//              (`cb()` where `cb = fn`) whose value-ref join names the sink: the --callers <vr called_by=> row (valuerefindex.h calledBy,
//              throughsFor); width = the calling functions that row names. slotHops skipped it: it answers first there.
enum class BackHopVia : std::uint8_t { Slot, SlotName, Declined, Container };

struct HopKindInfo
{
    std::string_view name;
    bool             fieldCall;   // a call through a struct field (slot, slot-name)
    bool             ring;        // the --impact ring follows it
};
inline constexpr std::array kHopKinds = { HopKindInfo{ "slot", true, true }, HopKindInfo{ "slot-name", true, true }, HopKindInfo{ "declined", false, false },
                                          HopKindInfo{ "container", false, true } };   // indexed by BackHopVia
inline constexpr std::size_t kBackHopViaCount = 4;
static_assert( enumCountIsExact<BackHopVia, kBackHopViaCount>(), "kBackHopViaCount must equal the BackHopVia count — raise it with the append" );
static_assert( kHopKinds.size() == kBackHopViaCount, "one kHopKinds row per BackHopVia value (the extent is deduced from the rows)" );
inline constexpr const HopKindInfo& hopKind( BackHopVia v ) noexcept { return kHopKinds[ std::size_t( v ) ]; }

struct BackHop
{
    NodeId        from  = kNoNode;   // the function that may call the sink without a graph edge
    BackHopVia    via   = BackHopVia::Slot;
    std::uint32_t width = 0;         // slot / container: the calling functions that row names; declined: the stored candidates
    std::uint32_t ref   = 0;         // the call's reference: the slot call through the field, the container call, or the declined call
};

// The forward twin of a BackHop (composed hops, rv-hop1 correction 2): from a function to a function one of its calls MAY reach.
struct ForwardHop
{
    NodeId        to     = kNoNode;
    BackHopVia    via    = BackHopVia::Slot;
    std::uint32_t width  = 0;   // the backward width the provider names for that hop (the calling functions of the join)
    std::uint32_t stored = 0;   // the FORWARD width: the distinct functions the call may reach (the field's / container's holders), never capped
    std::uint32_t ref    = 0;   // the call
};

// One row per calling function: the stronger kind (slot before slot-name), then the narrowest width, then the first call.
inline void keepOneHopPerCaller( std::vector<BackHop>& v )
{
    std::ranges::sort( v, [ ]( const BackHop& a, const BackHop& b )
    {
        if( a.from != b.from ) { return a.from < b.from; }
        if( a.via != b.via ) { return a.via < b.via; }
        if( a.width != b.width ) { return a.width < b.width; }
        return a.ref < b.ref;
    } );
    v.erase( std::ranges::unique( v, [ ]( const BackHop& a, const BackHop& b ) { return a.from == b.from; } ).begin(), v.end() );
}

// The distinct functions a container call may reach: every value reference its join lands (valuesThrough), resolved.
inline std::vector<NodeId> containerHolds( const ValueRefIndex& values, std::uint32_t through )
{
    std::vector<NodeId> fns;
    for( const std::uint32_t v : values.valuesThrough( through ) )
    {
        const std::vector<NodeId> t = values.targetsOf( v );
        fns.insert( fns.end(), t.begin(), t.end() );
    }
    std::ranges::sort( fns );
    fns.erase( std::ranges::unique( fns ).begin(), fns.end() );
    return fns;
}

// The hops one value-ref join yields for `sink`: `per( r, rows )` appends the rows of each value reference r to the sink; a row
// whose caller is the sink itself or no symbol is dropped (a call made inside the sink is no way in), then one row per caller.
template<class Per>
inline void valueRefHops( const IngestResult& ing, const ValueRefIndex& values, NodeId sink, std::vector<BackHop>& out, Per&& per )
{
    std::vector<BackHop> rows;
    for( const std::uint32_t r : values.valueRefsTo( std::span<const NodeId>( &sink, 1 ) ) )
    {
        per( r, rows );
    }
    std::erase_if( rows, [ & ]( const BackHop& h ) { return h.from == sink || h.from >= ing.symbols.size(); } );
    keepOneHopPerCaller( rows );
    out.insert( out.end(), rows.begin(), rows.end() );
}

// The sink's CONTAINER hops (composed hops): each function whose call through a container element (or a parameter's default) the
// sink's value-ref join names (the --callers <vr called_by=> row: calledBy / throughsFor); width = the callers that row names.
inline void containerBackHops( const IngestResult& ing, const ValueRefIndex& values, NodeId sink, std::vector<BackHop>& out )
{
    valueRefHops( ing, values, sink, out, [ & ]( std::uint32_t r, std::vector<BackHop>& rows )
    {
        if( ValueRefIndex::scopeOf( ing.references[ r ] ) == 'a' )
        {
            return;   // an ARGUMENT binds per call site: the callee's other callers pass other functions (rv-composed-hops F1); the bridge's value crossing names the passer
        }
        const std::uint32_t width = std::uint32_t( values.calledBy( r ).size() );
        for( const std::uint32_t t : values.throughsFor( r ) )
        {
            rows.push_back( BackHop{ ing.references[ t ].fromSymbol, BackHopVia::Container, width, t } );
        }
    } );
}

class BackwardHops
{
public:
    BackwardHops( const IngestResult& ing, const Graph& g, const ValueRefIndex& values ) : m_ing( ing ), m_g( g ), m_values( values ), m_rule( ing, g ) {}

    const DeclinedCrossRule& declinedRule() const noexcept { return m_rule; }
    const Graph&             graph() const noexcept { return m_g; }
    const ValueRefIndex&     values() const noexcept { return m_values; }

    // The sink's backward hops: slot rows first (the stronger tier first per calling function), then declined, then container,
    // each kind by caller id then reference — one row per (from, kind), the narrowest width kept. Memoised per sink.
    std::span<const BackHop> backwardHops( NodeId sink ) const
    {
        if( sink >= m_ing.symbols.size() )
        {
            return {};
        }
        if( const auto it = m_memo.find( sink ); it != m_memo.end() )
        {
            return it->second;
        }
        std::vector<BackHop> out;
        slotHops( sink, out );
        declinedHops( sink, out );
        containerBackHops( m_ing, m_values, sink, out );
        return m_memo.try_emplace( sink, std::move( out ) ).first->second;
    }

private:
    void slotHops( NodeId sink, std::vector<BackHop>& out ) const
    {
        valueRefHops( m_ing, m_values, sink, out, [ & ]( std::uint32_t r, std::vector<BackHop>& rows )
        {
            if( !m_values.calledBy( r ).empty() )
            {
                return;   // a container join answers that row first (valueRefCallerRows' own rule): container hops, no slot callers
            }
            bool                             typed    = false;
            std::uint32_t                    nameAlso = 0;
            const std::vector<std::uint32_t> calls    = slotCallers( m_ing, m_values.slots().callsFor( r ), typed, nameAlso );
            for( const std::uint32_t c : calls )
            {
                rows.push_back( BackHop{ m_ing.references[ c ].fromSymbol, typed ? BackHopVia::Slot : BackHopVia::SlotName, std::uint32_t( calls.size() ), c } );
            }
        } );
    }

    void declinedHops( NodeId sink, std::vector<BackHop>& out ) const
    {
        std::vector<BackHop> dc;
        for( const std::uint32_t site : declinedSitesNaming( m_g, std::span<const NodeId>( &sink, 1 ) ) )
        {
            const std::uint32_t list = m_g.declinedSiteList[ site ];
            const NodeId        from = m_g.declinedSiteCaller[ site ];
            if( list == kNoDeclinedList || list + 1 >= m_g.declinedListOff.size() || from == sink || from >= m_ing.symbols.size() || m_rule.builtinSpelled( site ) )
            {
                continue;   // no stored list, a declined self-call, or a call that reads as the builtin it spells
            }
            dc.push_back( BackHop{ from, BackHopVia::Declined, m_g.declinedListOff[ list + 1 ] - m_g.declinedListOff[ list ], m_g.declinedSiteRef[ site ] } );
        }
        keepOneHopPerCaller( dc );
        out.insert( out.end(), dc.begin(), dc.end() );
    }

    const IngestResult&                          m_ing;
    const Graph&                                 m_g;
    const ValueRefIndex&                         m_values;
    DeclinedCrossRule                            m_rule;
    mutable HashMap<NodeId, std::vector<BackHop>> m_memo;
};

// The forward width of a backward hop's call: the functions that field (slot) or container may hold; 0 for a declined hop.
inline std::uint32_t backHopStoredWidth( const IngestResult& ing, const ValueRefIndex& values, const BackHop& h )
{
    if( hopKind( h.via ).fieldCall )
    {
        return std::uint32_t( slotSiteOf( ing, values, h.ref ).targets.size() );
    }
    return h.via == BackHopVia::Container ? std::uint32_t( containerHolds( values, h.ref ).size() ) : 0u;
}

// Direction::Forward (composed hops, rv-hop1 correction 2): the functions `u`'s calls through a struct field or a container
// MAY reach, each confirmed from the other side (that function's backward hops name u by the same kind) so the two
// directions never disagree. Slot rows come from slotSiteRows (the --callees <vs> rows: same-type tier alone when it has any, a
// site the container join answers left to it), container rows from each call through a container (or a parameter's default) in u
// (valuesThrough). stored = how many functions that call may reach. Slot rows first, then container; memoised per u. No
// declined rows: a declined call's candidates are the graph's stored list (pathgaps.h bridgeDeclinedCrossings reads it).
class ForwardHops
{
public:
    ForwardHops( const IngestResult& ing, const BackwardHops& back ) : m_ing( ing ), m_back( back ) {}

    std::span<const ForwardHop> forwardHops( NodeId u ) const
    {
        if( u >= m_ing.symbols.size() )
        {
            return {};
        }
        if( const auto it = m_memo.find( u ); it != m_memo.end() )
        {
            return it->second;
        }
        std::vector<ForwardHop> out;
        for( const SlotSiteRow& site : slotSiteRows( m_ing, m_back.values(), std::span<const NodeId>( &u, 1 ) ) )
        {
            confirm( u, site.targets, site.ref, true, out );
        }
        for( const std::uint32_t t : throughsIn( u ) )
        {
            confirm( u, containerHolds( m_back.values(), t ), t, false, out );
        }
        return m_memo.try_emplace( u, std::move( out ) ).first->second;
    }

private:
    // Each of `fns` whose backward hops name u by a field kind (fieldCall) or the container kind becomes a forward row.
    void confirm( NodeId u, const std::vector<NodeId>& fns, std::uint32_t call, bool fieldCall, std::vector<ForwardHop>& out ) const
    {
        for( const NodeId f : fns )
        {
            const std::span<const BackHop> hops = m_back.backwardHops( f );
            const auto it = std::ranges::find_if( hops, [ & ]( const BackHop& h )
            {
                return h.from == u && ( fieldCall ? hopKind( h.via ).fieldCall : h.via == BackHopVia::Container );
            } );
            if( it != hops.end() )
            {
                out.push_back( ForwardHop{ f, it->via, it->width, std::uint32_t( fns.size() ), call } );
            }
        }
    }

    // The calls through a container element (or a parameter's default) made in u (role Through), in reference order: one sorted
    // (caller, reference) table built on first use, so a walk never rescans the index per node.
    std::span<const std::uint32_t> throughsIn( NodeId u ) const
    {
        if( !m_built )
        {
            m_built = true;
            for( std::uint32_t i = 0; i < m_ing.references.size(); ++i )
            {
                if( m_ing.references[ i ].role == RefRole::Through && m_ing.references[ i ].fromSymbol < m_ing.symbols.size() )
                {
                    m_by.emplace_back( m_ing.references[ i ].fromSymbol, i );
                }
            }
            std::ranges::sort( m_by );
            m_refs.reserve( m_by.size() );
            std::ranges::transform( m_by, std::back_inserter( m_refs ), [ ]( const std::pair<NodeId, std::uint32_t>& e ) { return e.second; } );
        }
        const auto lo = std::ranges::lower_bound( m_by, std::pair<NodeId, std::uint32_t>( u, 0u ) );
        const auto hi = std::ranges::lower_bound( m_by, std::pair<NodeId, std::uint32_t>( u + 1, 0u ) );
        return std::span<const std::uint32_t>( m_refs ).subspan( std::size_t( lo - m_by.begin() ), std::size_t( hi - lo ) );
    }

    const IngestResult&                                   m_ing;
    const BackwardHops&                                   m_back;
    mutable HashMap<NodeId, std::vector<ForwardHop>>      m_memo;
    mutable std::vector<std::pair<NodeId, std::uint32_t>> m_by;     // (caller, Through reference), sorted
    mutable std::vector<std::uint32_t>                    m_refs;   // parallel: the reference alone (throughsIn's span)
    mutable bool                                          m_built = false;
};

// Whether any of `sinks` has a backward hop a consumer follows (width within kBackHopWidth): --path's root next= swap.
inline bool anyFollowedBackHop( const BackwardHops& hops, std::span<const NodeId> sinks )
{
    return std::ranges::any_of( sinks, [ & ]( NodeId s )
    {
        return std::ranges::any_of( hops.backwardHops( s ), [ ]( const BackHop& h ) { return h.width <= kBackHopWidth; } );
    } );
}

// ── consumer 2: the --impact hedged ring ─────────────────────────────────────────────────────────────────────────────
// THE GAP. --impact's reaches= walks resolved edges only, so a function stored in a struct field answered reaches="0" beside
// its own <vr via="slot" called_by=> row (the IX-1 census: 226 such answers on tmux + htop). HOP1 added one or two slot hops;
// the functions that reach THOSE through ordinary proven calls (tmux: cmdq_fire_command's own callers) stayed out, a third slot
// hop (tmux send-keys: cancel ← window_copy_command ← cmd_send_keys_exec ← cmdq_fire_command) stopped at two, and a function
// in a table called by index (`handlers[i]()`) had no ring at all.
//
// THE CONTRACT (HOP1 + composed hops). The ring follows the hop kinds whose kHopKinds row says `ring` (slot, slot-name,
// container; never declined: declined_calls= / <dc> speak for those):
//   d=1  each answered definition's ring hops (the <vr> rows' called_by=);
//   d=2  each d=1 function's ring hops;
//   d=3  each d=2 function's FIELD hops, only on a chain whose every hop is a field call (slot / slot-name) within
//        kBackHopWidth calling functions — the bridge's own allowance (pathgaps.h kBridgeSlotCrossings);
//   tail every function that reaches a ring member through PROVEN call edges (graph.h transitiveCallers), from members whose
//        every hop on the chain is within kBackHopWidth (a wider hop is counted itself but composed with nothing).
// reaches_hedged=N counts the distinct functions of d=1..3 and the tail that reaches= does not already hold and that are not an
// answered definition. Every one of them is HEDGED: a chain with one hedged hop is hedged, whatever its other edges are.
// The d=2 and d=3 rows (<rh>) are listed after the <vrs> rows in the ring order (ImpactRingOrder), kImpactRingRowCap shown,
// next= the callers of the first unshown row's `calls`; each prints its call's forward width (slot_fns= for a field, cands= for
// a container). The tail has no rows: `--impact=<ring member>` lists it, and a member with more than kBackHopWidth counted
// functions behind it prints one <hub> row naming that call (the hub fold, ImpactHubRow). Absent (no attribute, no rows)
// when the ring is empty: byte-identical.
// What it does NOT mean: a ring or tail function MAY change behaviour through a hedged call; it is never a proven caller and
// never in reaches= / by_depth= / radius_*.
inline constexpr std::size_t   kImpactRingRowCap    = 16;
inline constexpr std::uint32_t kImpactRingSlotDepth = 3;   // a field-only chain's depth; pathgaps.h pins it to kBridgeSlotCrossings

struct ImpactRingRow
{
    NodeId        fn     = kNoNode;   // the d=2 / d=3 function: it may call `calls` through a hedged hop
    NodeId        calls  = kNoNode;   // the d=1 / d=2 function it may call
    BackHopVia    via    = BackHopVia::Slot;
    std::uint32_t ref    = 0;         // its call (called_at=)
    std::uint32_t depth  = 2;
    std::uint32_t stored = 0;         // the call's forward width (every row: the ordering reads it)
    std::uint32_t width  = 0;         // the hop's backward width (BackHop.width): the ordering's crossing width
};

// HUB FOLD (hub-fold lane). A ring member (d=1..3) whose chain stayed narrow and whose own proven tail (the counted functions
// that reach it through proven calls) holds more than kBackHopWidth functions prints ONE <hub> row: behind= that tail's size,
// tested= how many of it a test reaches, sample= its first three by the ring order, next= --impact on the member (which lists
// the tail as proven rows). The tail itself is never listed here. The fold changes rows only: reaches_hedged= keeps every
// function; reaches_hedged_hubs=K says K of them sit behind some hub (hubs may share callers; K counts each once).
// What it does NOT mean: behind= is not a proven count toward of= (every one is still hedged), and a member at or under the
// bound is not folded (its tail stays unlisted, as before the fold).
struct ImpactHubRow
{
    NodeId              fn     = kNoNode;
    BackHopVia          via    = BackHopVia::Slot;   // the hop that joined the member to the ring
    std::uint32_t       depth  = 1;
    std::size_t         behind = 0;
    std::size_t         tested = 0;
    std::vector<NodeId> sample;                      // at most kImpactHubSample, in ring order
};
inline constexpr std::size_t kImpactHubSample = 3;

struct ImpactRing
{
    std::size_t                hedged = 0;    // reaches_hedged=
    std::vector<ImpactRingRow> rows;          // ring order (impactRingLess): certainty tier, width narrow first, then the node lens
    std::vector<ImpactHubRow>  hubs;          // widest first
    std::size_t                hubbed    = 0; // reaches_hedged_hubs=: distinct functions behind some hub
    std::size_t                rowsShown = 0; // <rh> rows printed (kImpactRingRowCap, or less under an explicit --limit)
    std::size_t                hubsShown = 0; // <hub> rows printed
    std::size_t                cut       = 0; // ring_cut=: hedged rows the --limit page cut (proven <s> rows keep the page)
    std::string                cutNext;       // the replay that shows them (the root next= while cut > 0)
};

// The ring order's node lens (rule (b) after the certainty tier and the width): tested (isTestedByReach, the radius_tested=
// lens) > an entry point (no proven caller: the top of its chain) > same directory as one of of='s definitions > rank r=.
// A null testReach / empty rank skips that key (every row ties on it).
struct ImpactRingLens
{
    const std::vector<char>* testReach = nullptr;
    std::span<const float>   rank;
};

// The ring's walk state (one answer): which depth each function joined at, whether some chain to it stayed narrow, and whether
// such a chain is also field-only. Each step is one method so impactSlotRing reads as the contract above.
class ImpactRingWalk
{
public:
    ImpactRingWalk( const IngestResult& ing, const BackwardHops& hops, std::span<const NodeId> seeds, std::span<const NodeId> reach )
        : m_hops( hops ), m_S( ing.symbols.size() ), m_known( m_S, 0 ), m_counted( m_S, 0 ), m_level( m_S, 0 ), m_narrow( m_S, 0 ), m_fieldNarrow( m_S, 0 ),
          m_via( m_S, BackHopVia::Slot )
    {
        for( const std::span<const NodeId> set : { seeds, reach } )
        {
            for( const NodeId n : set ) { if( n < m_S ) { m_known[ n ] = 1; } }
        }
    }

    // d=1: each answered definition's ring hops (all of them counted, whatever their width)
    void depthOne( std::span<const NodeId> seeds )
    {
        for( const NodeId s : seeds )
        {
            for( const BackHop& h : m_hops.backwardHops( s ) )
            {
                if( hopKind( h.via ).ring && h.from < m_S )
                {
                    join( h.from, 1, m_d1, h.via );
                    carry( h.from, h, true, true );
                }
            }
        }
        for( const NodeId f : m_d1 )
        {
            count( f );
        }
    }
    // d=2: each d=1 function's ring hops; a proven member, an answered definition or a d=1 function already speaks for itself
    void depthTwo( ImpactRing& out )
    {
        for( const NodeId f : m_d1 )
        {
            for( const BackHop& h : m_hops.backwardHops( f ) )
            {
                if( hopKind( h.via ).ring && fresh( f, h, 1 ) )
                {
                    count( h.from );
                    out.rows.push_back( ImpactRingRow{ h.from, f, h.via, h.ref, 2, 0, h.width } );
                    join( h.from, 2, m_d2, h.via );
                    carry( h.from, h, m_narrow[ f ] != 0, m_fieldNarrow[ f ] != 0 );
                }
            }
        }
    }
    // d=3: only off a field-only, narrow chain, only by a narrow field hop, never back onto a member the ring already holds
    void depthThree( ImpactRing& out )
    {
        for( const NodeId f : m_d2 )
        {
            for( const BackHop& h : m_hops.backwardHops( f ) )
            {
                if( m_fieldNarrow[ f ] != 0 && hopKind( h.via ).fieldCall && h.width <= kBackHopWidth && fresh( f, h, 0 ) )
                {
                    join( h.from, 3, m_d3, h.via );
                    m_narrow[ h.from ] = 1;
                    count( h.from );
                    out.rows.push_back( ImpactRingRow{ h.from, f, h.via, h.ref, 3, 0, h.width } );
                }
            }
        }
    }
    // the tail: the proven callers of every member whose chain stayed narrow (one walk over all of them)
    void tail()
    {
        for( const std::vector<NodeId>* members : { &m_d1, &m_d2, &m_d3 } )
        {
            std::ranges::copy_if( *members, std::back_inserter( m_composable ), [ & ]( NodeId n ) { return m_narrow[ n ] != 0; } );
        }
        const std::size_t before = m_hedged;
        for( const NodeId n : m_composable.empty() ? std::vector<NodeId>() : transitiveCallers( m_hops.graph(), m_composable ) )
        {
            count( n );
        }
        m_tailCounted = m_hedged - before;
    }
    // The hub fold: one proven-caller walk per composable member (attributeReachAnswer groups a reach by DEFINITION of of=,
    // not by ring member, so it cannot say which member a tail function sits behind). The walk is the tail's own
    // (expandCallers) with its visited marks reset per member over the nodes it touched, never re-allocated. Skipped
    // whole when the tail itself is within the bound: no member's share of it can exceed it then.
    // `tailOf(member, out)` receives each hub's counted tail before the row is kept (the caller orders the sample).
    template<class TailFn>
    void hubs( ImpactRing& out, TailFn&& tailOf ) const
    {
        std::size_t hubbed = 0;
        if( m_tailCounted + m_composable.size() <= kBackHopWidth )
        {
            return;
        }
        const Graph&               g = m_hops.graph();
        std::vector<char>          seen( g.wOutDeg.size(), 0 ), behindAny( m_S, 0 );
        std::vector<std::uint32_t> depth( g.wOutDeg.size(), 0 );
        std::vector<NodeId>        q, tail;
        for( const NodeId n : m_composable )
        {
            if( n >= seen.size() )
            {
                continue;
            }
            q.assign( 1, n );
            seen[ n ] = 1;
            expandCallers( g, q, 0, SIZE_MAX, seen, depth );
            tail.clear();
            for( std::size_t i = 1; i < q.size(); ++i )
            {
                if( q[ i ] < m_S && m_counted[ q[ i ] ] != 0 )
                {
                    tail.push_back( q[ i ] );
                }
            }
            for( const NodeId x : q ) { seen[ x ] = 0; }
            if( tail.size() > kBackHopWidth )
            {
                ImpactHubRow hub{ n, m_via[ n ], std::uint32_t( m_level[ n ] ), tail.size(), 0, {} };
                for( const NodeId x : tail )
                {
                    hubbed += behindAny[ x ] == 0 ? 1 : 0;
                    behindAny[ x ] = 1;
                }
                tailOf( hub, tail );
                out.hubs.push_back( std::move( hub ) );
            }
        }
        out.hubbed = hubbed;
    }
    std::size_t hedged() const noexcept { return m_hedged; }

private:
    void count( NodeId n )
    {
        if( n < m_S && m_known[ n ] == 0 && m_counted[ n ] == 0 )
        {
            m_counted[ n ] = 1;
            ++m_hedged;
        }
    }
    void join( NodeId n, char depth, std::vector<NodeId>& at, BackHopVia via )
    {
        if( m_level[ n ] == 0 )
        {
            m_level[ n ] = depth;
            m_via[ n ]   = via;
            at.push_back( n );
        }
    }
    void carry( NodeId to, const BackHop& h, bool narrowIn, bool fieldIn )
    {
        const bool w         = h.width <= kBackHopWidth;
        m_narrow[ to ]       = char( m_narrow[ to ] != 0 || ( narrowIn && w ) );
        m_fieldNarrow[ to ]  = char( m_fieldNarrow[ to ] != 0 || ( fieldIn && w && hopKind( h.via ).fieldCall ) );
    }
    // h.from is a new ring function for member f: not f, not known, and (maxLevel 1) not at d=1 / (maxLevel 0) at no depth yet
    bool fresh( NodeId f, const BackHop& h, char maxLevel ) const
    {
        return h.from != f && h.from < m_S && m_known[ h.from ] == 0 && ( maxLevel == 0 ? m_level[ h.from ] == 0 : m_level[ h.from ] != 1 );
    }

    const BackwardHops& m_hops;
    std::size_t         m_S;
    std::size_t         m_hedged = 0;
    std::size_t         m_tailCounted = 0;            // the tail's own share of m_hedged
    std::vector<char>   m_known, m_counted, m_level, m_narrow, m_fieldNarrow;
    std::vector<BackHopVia> m_via;                    // the hop that joined each member (a hub row's via=)
    std::vector<NodeId> m_d1, m_d2, m_d3, m_composable;
};

// The ring order (rule (b)): a node's lens key, then the row key that puts it after the certainty tier and the width.
class ImpactRingOrder
{
public:
    ImpactRingOrder( const IngestResult& ing, const Graph& g, const ImpactRingLens& lens, std::span<const NodeId> seeds )
        : m_ing( ing ), m_g( g ), m_lens( lens ), m_testReach( lens.testReach != nullptr ? *lens.testReach : kNoTestReach() )
    {
        for( const NodeId s : seeds )
        {
            if( s < ing.symbols.size() ) { m_seedDirs.push_back( dirOf( s ) ); }
        }
    }
    bool nodeLess( NodeId a, NodeId b ) const
    {
        const auto ka = key( a ), kb = key( b );
        return ka != kb ? ka < kb : a < b;
    }
    // certainty tier, then the backward and forward widths (narrow first), then the node lens
    bool rowLess( const ImpactRingRow& a, const ImpactRingRow& b ) const
    {
        const auto ta = std::tie( a.depth, a.width, a.stored ), tb = std::tie( b.depth, b.width, b.stored );
        return ta != tb ? ta < tb : ( a.fn != b.fn ? nodeLess( a.fn, b.fn ) : a.ref < b.ref );
    }
    bool tested( NodeId n ) const noexcept { return isTestedByReach( m_ing, m_testReach, n ); }

private:
    std::string_view dirOf( NodeId n ) const { return includerDir( m_ing.files[ m_ing.symbols[ n ].fileId ] ); }
    // (untested, not an entry, another directory, -rank): smaller first
    std::tuple<bool, bool, bool, float> key( NodeId n ) const
    {
        const auto*       ro    = m_g.inEdges.rowOffsets();
        const bool        entry = n < m_g.inEdges.rows() && ro[ n ] == ro[ n + 1 ];
        const bool        same  = std::ranges::find( m_seedDirs, dirOf( n ) ) != m_seedDirs.end();
        const float       r     = n < m_lens.rank.size() ? m_lens.rank[ n ] : 0.f;
        return { !tested( n ), !entry, !same, -r };
    }

    const IngestResult&           m_ing;
    const Graph&                  m_g;
    static const std::vector<char>& kNoTestReach()
    {
        static const std::vector<char> none;
        return none;
    }
    const ImpactRingLens&         m_lens;
    const std::vector<char>&      m_testReach;   // empty: no row is tested
    std::vector<std::string_view> m_seedDirs;
};

inline ImpactRing impactSlotRing( const IngestResult& ing, const BackwardHops& hops, std::span<const NodeId> seeds, std::span<const NodeId> reach,
                                  const ImpactRingLens& lens = {} )
{
    static_assert( kImpactRingSlotDepth == 3, "the walk below takes d=1, d=2 and exactly one more field hop" );
    ImpactRing     out;
    ImpactRingWalk walk( ing, hops, seeds, reach );
    walk.depthOne( seeds );
    walk.depthTwo( out );
    walk.depthThree( out );
    walk.tail();
    out.hedged = walk.hedged();
    for( ImpactRingRow& row : out.rows )
    {
        row.stored = backHopStoredWidth( ing, hops.values(), BackHop{ row.fn, row.via, 0, row.ref } );
    }
    const ImpactRingOrder order( ing, hops.graph(), lens, seeds );
    std::ranges::stable_sort( out.rows, [ & ]( const ImpactRingRow& a, const ImpactRingRow& b ) { return order.rowLess( a, b ); } );
    walk.hubs( out, [ & ]( ImpactHubRow& hub, std::vector<NodeId>& tail )
    {
        hub.tested = std::size_t( std::ranges::count_if( tail, [ & ]( NodeId n ) { return order.tested( n ); } ) );
        const std::size_t k = std::min( tail.size(), kImpactHubSample );
        std::partial_sort( tail.begin(), tail.begin() + std::ptrdiff_t( k ), tail.end(), [ & ]( NodeId a, NodeId b ) { return order.nodeLess( a, b ); } );
        hub.sample.assign( tail.begin(), tail.begin() + std::ptrdiff_t( k ) );
    } );
    std::ranges::stable_sort( out.hubs, [ ]( const ImpactHubRow& a, const ImpactHubRow& b )
                              { return a.depth != b.depth ? a.depth < b.depth : a.behind > b.behind; } );
    out.rowsShown = std::min( out.rows.size(), kImpactRingRowCap );
    out.hubsShown = out.hubs.size();
    return out;
}

// The page window (rule (c)): under an explicit --limit, the proven <s> rows keep the page and the hedged rows take what is
// left, hubs first (they carry most of the count), then <rh>; ring_cut= counts the rows cut and the root next= replays the
// answer at the limit that shows them all (every proven row plus every ring row the default shows). No --limit: unchanged.
inline void sizeImpactRing( ImpactRing& ring, int pageLimit, std::size_t provenShown, std::size_t provenTotal, std::string_view sym )
{
    const std::size_t rowsDefault = std::min( ring.rows.size(), kImpactRingRowCap );
    ring.rowsShown = rowsDefault;
    ring.hubsShown = ring.hubs.size();
    if( pageLimit <= 0 )
    {
        return;
    }
    std::size_t budget = std::size_t( pageLimit ) > provenShown ? std::size_t( pageLimit ) - provenShown : 0;
    ring.hubsShown     = std::min( ring.hubsShown, budget );
    budget -= ring.hubsShown;
    ring.rowsShown = std::min( ring.rowsShown, budget );
    ring.cut       = rowsDefault + ring.hubs.size() - ring.rowsShown - ring.hubsShown;
    ring.cutNext   = ring.cut == 0 ? std::string() : nextFlag( "--impact=", sym ) + " --limit=" + std::to_string( provenTotal + rowsDefault + ring.hubs.size() );
}

inline std::string impactRingAttrXml( const ImpactRing& ring )
{
    return countFieldOrEmpty( "reaches_hedged", ring.hedged, /*json=*/false ) + countFieldOrEmpty( "reaches_hedged_hubs", ring.hubbed, /*json=*/false )
         + countFieldOrEmpty( "ring_cut", ring.cut, /*json=*/false );
}
inline std::string impactRingKeyJson( const ImpactRing& ring )
{
    return countFieldOrEmpty( "reaches_hedged", ring.hedged, /*json=*/true ) + countFieldOrEmpty( "reaches_hedged_hubs", ring.hubbed, /*json=*/true )
         + countFieldOrEmpty( "ring_cut", ring.cut, /*json=*/true );
}

// A hub's selector: file:name, so a namesake in another file is never the one --impact lists (checklist 16).
inline std::string impactHubSelector( const IngestResult& ing, const ImpactHubRow& hub, const VrRender& rr )
{
    const Symbol& s = ing.symbols[ hub.fn ];
    return vrPath( ing, s.fileId, rr ) + ":" + s.name;
}

// The answer's root next=: the ring_cut replay while hedged rows were cut; else the widest hub's --impact when the hubs hold
// more than half of reaches_hedged= (that is where the answer's weight sits); else `fallback` (--safe-delete=, as before).
inline std::string impactRootNext( const IngestResult& ing, const ImpactRing& ring, const VrRender& rr, std::string fallback )
{
    if( ring.cut > 0 )
    {
        return ring.cutNext;
    }
    if( !ring.hubs.empty() && 2 * ring.hubbed > ring.hedged )
    {
        const auto widest = std::ranges::max_element( ring.hubs, [ ]( const ImpactHubRow& a, const ImpactHubRow& b ) { return a.behind < b.behind; } );
        return nextFlag( "--impact=", impactHubSelector( ing, *widest, rr ) );
    }
    return fallback;
}

inline std::string impactRingNext( const IngestResult& ing, const ImpactRing& ring )
{
    return ring.rowsShown < ring.rows.size() ? nextFlag( "--callers=", ing.symbols[ ring.rows[ ring.rowsShown ].calls ].name ) : std::string();
}

inline std::uint32_t impactRingDepth( const ImpactRing& ring )
{
    return std::ranges::any_of( ring.rows, [ ]( const ImpactRingRow& r ) { return r.depth == 3; } ) ? 3u : 2u;
}

// A ring row's forward-width attribute name: slot_fns for a field call, cands for a container call.
inline std::string_view impactRingStoredKey( const ImpactRingRow& row ) noexcept
{
    return hopKind( row.via ).fieldCall ? std::string_view( "slot_fns" ) : std::string_view( "cands" );
}

inline std::string impactHubSampleText( const IngestResult& ing, const ImpactHubRow& hub, std::string_view sep, bool json )
{
    std::string out;
    for( const NodeId n : hub.sample )
    {
        out += ( out.empty() ? std::string() : std::string( sep ) ) + ( json ? "\"" + jsonStr( ing.symbols[ n ].name ) + "\"" : ing.symbols[ n ].name );
    }
    return out;
}

// <ring d="2|3" total= shown=[ next=]><rh called_by= called_at= via= calls=[ d="3"] slot_fns=|cands=/>…</ring> (absent with no
// shown row), then <hub n= via= d= behind= tested= sample= next=/> per shown hub; "" when neither rides.
inline std::string impactRingXml( const IngestResult& ing, const ImpactRing& ring, const VrRender& rr )
{
    std::vector<char> esc;
    const auto        ex  = [ & ]( std::string_view s ) { return std::string( escapeXml( s, esc ) ); };
    std::string       out;
    if( ring.rowsShown > 0 )
    {
        out = "<ring d=\"" + std::to_string( impactRingDepth( ring ) ) + "\" total=\"" + std::to_string( ring.rows.size() ) + "\" shown=\"" + std::to_string( ring.rowsShown ) + "\""
            + nextAttrXml( impactRingNext( ing, ring ) ) + ">";
        for( std::size_t i = 0; i < ring.rowsShown; ++i )
        {
            const ImpactRingRow& row = ring.rows[ i ];
            const Reference&     c   = ing.references[ row.ref ];
            out += "<rh called_by=\"" + ex( ing.symbols[ row.fn ].name ) + "\" called_at=\"" + ex( vrPath( ing, c.fileId, rr ) ) + ":" + std::to_string( c.line )
                 + "\" via=\"" + std::string( hopKind( row.via ).name ) + "\" calls=\"" + ex( ing.symbols[ row.calls ].name ) + "\""
                 + ( row.depth == 3 ? std::string( " d=\"3\"" ) : std::string() ) + " " + std::string( impactRingStoredKey( row ) ) + "=\"" + std::to_string( row.stored ) + "\"/>";
        }
        out += "</ring>";
    }
    for( std::size_t i = 0; i < ring.hubsShown; ++i )
    {
        const ImpactHubRow& hub = ring.hubs[ i ];
        out += "<hub n=\"" + ex( ing.symbols[ hub.fn ].name ) + "\" via=\"" + std::string( hopKind( hub.via ).name ) + "\" d=\"" + std::to_string( hub.depth )
             + "\" behind=\"" + std::to_string( hub.behind ) + "\" tested=\"" + std::to_string( hub.tested ) + "\" sample=\"" + ex( impactHubSampleText( ing, hub, ",", false ) ) + "\""
             + nextAttrXml( nextFlag( "--impact=", impactHubSelector( ing, hub, rr ) ) ) + "/>";
    }
    return out;
}
// The --json twin: `,"ring":{"d":2|3,"total":N,"shown":M[,"next":…],"rows":[{"called_by":…,"called_at":…,"via":…,"calls":…[,"d":3],"slot_fns"|"cands":K}]}`
// then `,"hubs":[{"n":…,"via":…,"d":D,"behind":B,"tested":T,"sample":[…],"next":…}]`, each absent as its XML twin is.
inline std::string impactHubsJson( const IngestResult& ing, const ImpactRing& ring, const VrRender& rr );
inline std::string impactRingJson( const IngestResult& ing, const ImpactRing& ring, const VrRender& rr )
{
    std::string out;
    if( ring.rowsShown > 0 )
    {
        out = ",\"ring\":{\"d\":" + std::to_string( impactRingDepth( ring ) ) + ",\"total\":" + std::to_string( ring.rows.size() ) + ",\"shown\":" + std::to_string( ring.rowsShown );
        if( const std::string next = impactRingNext( ing, ring ); !next.empty() )
        {
            out += ",\"next\":\"" + jsonStr( next ) + "\"";
        }
        out += ",\"rows\":[";
        for( std::size_t i = 0; i < ring.rowsShown; ++i )
        {
            const ImpactRingRow& row = ring.rows[ i ];
            const Reference&     c   = ing.references[ row.ref ];
            out += std::string( i == 0 ? "{" : ",{" ) + "\"called_by\":\"" + jsonStr( ing.symbols[ row.fn ].name ) + "\",\"called_at\":\""
                 + jsonStr( vrPath( ing, c.fileId, rr ) + ":" + std::to_string( c.line ) ) + "\",\"via\":\"" + std::string( hopKind( row.via ).name )
                 + "\",\"calls\":\"" + jsonStr( ing.symbols[ row.calls ].name ) + "\"" + ( row.depth == 3 ? std::string( ",\"d\":3" ) : std::string() )
                 + ",\"" + std::string( impactRingStoredKey( row ) ) + "\":" + std::to_string( row.stored ) + "}";
        }
        out += "]}";
    }
    return out + impactHubsJson( ing, ring, rr );
}
inline std::string impactHubsJson( const IngestResult& ing, const ImpactRing& ring, const VrRender& rr )
{
    std::string out;
    for( std::size_t i = 0; i < ring.hubsShown; ++i )
    {
        const ImpactHubRow& hub = ring.hubs[ i ];
        out += std::string( i == 0 ? ",\"hubs\":[{" : ",{" ) + "\"n\":\"" + jsonStr( ing.symbols[ hub.fn ].name ) + "\",\"via\":\"" + std::string( hopKind( hub.via ).name )
             + "\",\"d\":" + std::to_string( hub.depth ) + ",\"behind\":" + std::to_string( hub.behind ) + ",\"tested\":" + std::to_string( hub.tested )
             + ",\"sample\":[" + impactHubSampleText( ing, hub, ",", true ) + "],\"next\":\"" + jsonStr( nextFlag( "--impact=", impactHubSelector( ing, hub, rr ) ) ) + "\"}"
             + ( i + 1 == ring.hubsShown ? "]" : "" );
    }
    return out;
}

inline constexpr std::string_view kImpactRingLegend =
    "reaches_hedged=N (absent when 0): N functions MAY reach of= through a HEDGED hop: a call through a struct FIELD (the IX-1 slot "
    "join) or through a container element or a parameter's default (container), one or two such hops back (three when every hop is a field "
    "call within 16 callers), plus every function that reaches one of those through proven calls (only past hops within 16 callers). "
    "Not counted in reaches= and not an answered definition. The one-hop set has NO width bound (a wide field's callers are all counted). "
    "A hedge: one hedged hop makes the whole chain hedged; never a proven caller, never in reaches=/by_depth=/radius_*. ";
inline constexpr std::string_view kImpactRingRowsLegend =
    "<ring d=2 total= shown=> lists the two-hop ones (d=3 when a third field hop is listed too): <rh called_by= called_at= via=slot|slot-name|container calls=> = called_by may call "
    "calls= through a field or container at called_at=; d=3 the third field hop; slot_fns=K the functions that field holds, cands=K the functions "
    "that container may hold (the forward width, not capped); first 16, next= the callers of the first unshown calls=. ";
inline constexpr std::string_view kImpactHubLegend =
    "<hub n= via= d= behind= tested= sample= next=>: a ring member with more than 16 proven callers behind it, folded: those behind= functions are "
    "in reaches_hedged= but not listed (tested= of them a test reaches, sample= the first 3, next= lists them as proven rows); reaches_hedged_hubs=K: "
    "K of reaches_hedged= sit behind a hub; ring_cut=N: the page limit cut N ring rows (hedged rows go first, proven rows keep the page), next= shows them. ";
// `ring.hedged`: reaches_hedged= rides; `rowsForm`: the answer prints rows (XML/JSON; the columnar form carries the counts alone),
// so the <ring> clause rides with a shown <rh> row; the hub clause rides with reaches_hedged_hubs= / ring_cut= in every form.
inline std::string impactRingLegend( const ImpactRing& ring, bool rowsForm )
{
    std::string out;
    if( ring.hedged > 0 )
    {
        out.append( kImpactRingLegend );
        out.append( rowsForm && ring.rowsShown > 0 ? kImpactRingRowsLegend : std::string_view() );
        out.append( ring.hubbed > 0 || ring.cut > 0 ? kImpactHubLegend : std::string_view() );
    }
    return out;
}

}   // namespace rw
