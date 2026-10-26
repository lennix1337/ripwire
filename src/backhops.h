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
//   container  (composed hops) a call through a container element or a called parameter (`handlers[i]()`, `cb()` on a
//              parameter) whose value-ref join names the sink: the --callers <vr called_by=> row (valuerefindex.h calledBy,
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

class BackwardHops
{
public:
    BackwardHops( const IngestResult& ing, const Graph& g, const ValueRefIndex& values ) : m_ing( ing ), m_g( g ), m_values( values ), m_rule( ing, g ) {}

    const DeclinedCrossRule& declinedRule() const noexcept { return m_rule; }
    const Graph&             graph() const noexcept { return m_g; }

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
        std::vector<BackHop> container;
        slotHops( sink, out, container );
        declinedHops( sink, out );
        keepOnePerCaller( container );
        out.insert( out.end(), container.begin(), container.end() );
        return m_memo.try_emplace( sink, std::move( out ) ).first->second;
    }

    // Direction::Forward (composed hops): the functions `u`'s calls through a struct field or a container MAY reach, each
    // confirmed from the other side (that function's backward hops name u by the same kind) so the two directions never
    // disagree. Slot rows come from slotSiteRows (the --callees <vs> rows: same-type tier alone when it has any, a site the
    // container join answers left to it), container rows from each call through a container or called parameter in u
    // (valuesThrough). stored = how many functions that call may reach. Slot rows first, then container; memoised per u.
    // No declined rows: a declined call's candidates are the graph's stored list (bridgeDeclinedCrossings reads it).
    std::span<const ForwardHop> forwardHops( NodeId u ) const
    {
        if( u >= m_ing.symbols.size() )
        {
            return {};
        }
        if( const auto it = m_fwdMemo.find( u ); it != m_fwdMemo.end() )
        {
            return it->second;
        }
        std::vector<ForwardHop> out;
        for( const SlotSiteRow& site : slotSiteRows( m_ing, m_values, std::span<const NodeId>( &u, 1 ) ) )
        {
            for( const NodeId f : site.targets )
            {
                if( const BackHop* h = hopFrom( f, u, true ); h != nullptr )
                {
                    out.push_back( ForwardHop{ f, h->via, h->width, std::uint32_t( site.targets.size() ), site.ref } );
                }
            }
        }
        for( const std::uint32_t t : throughsIn( u ) )
        {
            const std::vector<NodeId> fns = containerHolds( t );
            for( const NodeId f : fns )
            {
                if( const BackHop* h = hopFrom( f, u, false ); h != nullptr )
                {
                    out.push_back( ForwardHop{ f, h->via, h->width, std::uint32_t( fns.size() ), t } );
                }
            }
        }
        return m_fwdMemo.try_emplace( u, std::move( out ) ).first->second;
    }

    // The forward width of a backward hop's call: the functions that field (slot) or container may hold; 0 for a declined hop.
    std::uint32_t storedWidth( const BackHop& h ) const
    {
        if( hopKind( h.via ).fieldCall )
        {
            return std::uint32_t( slotSiteOf( m_ing, m_values, h.ref ).targets.size() );
        }
        return h.via == BackHopVia::Container ? std::uint32_t( containerHolds( h.ref ).size() ) : 0u;
    }

private:
    void slotHops( NodeId sink, std::vector<BackHop>& out, std::vector<BackHop>& container ) const
    {
        std::vector<BackHop> slot;
        for( const std::uint32_t r : m_values.valueRefsTo( std::span<const NodeId>( &sink, 1 ) ) )
        {
            if( const std::vector<NodeId> by = m_values.calledBy( r ); !by.empty() )
            {
                // a container/parameter join answers that row first (valueRefCallerRows' own rule): its calling functions are
                // container hops (composed hops), and the row has no slot callers
                for( const std::uint32_t t : m_values.throughsFor( r ) )
                {
                    const NodeId f = m_ing.references[ t ].fromSymbol;
                    if( f != sink && f < m_ing.symbols.size() )
                    {
                        container.push_back( BackHop{ f, BackHopVia::Container, std::uint32_t( by.size() ), t } );
                    }
                }
                continue;
            }
            bool                             typed    = false;
            std::uint32_t                    nameAlso = 0;
            const std::vector<std::uint32_t> calls    = slotCallers( m_ing, m_values.slots().callsFor( r ), typed, nameAlso );
            for( const std::uint32_t c : calls )
            {
                const NodeId f = m_ing.references[ c ].fromSymbol;
                if( f != sink && f < m_ing.symbols.size() )   // a call made inside the sink itself is no way in (defensive: the ring's `known` and the bridge's layer test hold the seeds too)
                {
                    slot.push_back( BackHop{ f, typed ? BackHopVia::Slot : BackHopVia::SlotName, std::uint32_t( calls.size() ), c } );
                }
            }
        }
        keepOnePerCaller( slot );
        out.insert( out.end(), slot.begin(), slot.end() );
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
        keepOnePerCaller( dc );
        out.insert( out.end(), dc.begin(), dc.end() );
    }

    // f's backward hop from u of the field kinds (slot, slot-name) or of the container kind, or null.
    const BackHop* hopFrom( NodeId f, NodeId u, bool fieldCall ) const
    {
        const std::span<const BackHop> hops = backwardHops( f );
        const auto it = std::ranges::find_if( hops, [ & ]( const BackHop& h )
        {
            return h.from == u && ( fieldCall ? hopKind( h.via ).fieldCall : h.via == BackHopVia::Container );
        } );
        return it != hops.end() ? &*it : nullptr;
    }

    // The calls through a container element or a called parameter made in u (role Through, not a struct-field call), in
    // reference order: one sorted (caller, reference) table built on first use, so a walk never rescans the index per node.
    std::span<const std::uint32_t> throughsIn( NodeId u ) const
    {
        if( !m_throughsBuilt )
        {
            m_throughsBuilt = true;
            const std::size_t S = m_ing.symbols.size();
            for( std::uint32_t i = 0; i < m_ing.references.size(); ++i )
            {
                if( m_ing.references[ i ].role == RefRole::Through && m_ing.references[ i ].fromSymbol < S )
                {
                    m_throughsBy.emplace_back( m_ing.references[ i ].fromSymbol, i );
                }
            }
            std::ranges::sort( m_throughsBy );
            m_throughRefs.reserve( m_throughsBy.size() );
            for( const auto& [ from, ref ] : m_throughsBy )
            {
                m_throughRefs.push_back( ref );
            }
        }
        const auto lo = std::ranges::lower_bound( m_throughsBy, std::pair<NodeId, std::uint32_t>( u, 0u ) );
        const auto hi = std::ranges::lower_bound( m_throughsBy, std::pair<NodeId, std::uint32_t>( u + 1, 0u ) );
        return std::span<const std::uint32_t>( m_throughRefs ).subspan( std::size_t( lo - m_throughsBy.begin() ), std::size_t( hi - lo ) );
    }

    // The distinct functions a container/parameter call may reach: every value reference its join lands (valuesThrough), resolved.
    std::vector<NodeId> containerHolds( std::uint32_t through ) const
    {
        std::vector<NodeId> fns;
        for( const std::uint32_t v : m_values.valuesThrough( through ) )
        {
            const std::vector<NodeId> t = m_values.targetsOf( v );
            fns.insert( fns.end(), t.begin(), t.end() );
        }
        std::ranges::sort( fns );
        fns.erase( std::ranges::unique( fns ).begin(), fns.end() );
        return fns;
    }

    // One row per calling function: the stronger kind (slot before slot-name), then the narrowest width, then the first call.
    static void keepOnePerCaller( std::vector<BackHop>& v )
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

    const IngestResult&                             m_ing;
    const Graph&                                    m_g;
    const ValueRefIndex&                            m_values;
    DeclinedCrossRule                               m_rule;
    mutable HashMap<NodeId, std::vector<BackHop>>    m_memo;
    mutable HashMap<NodeId, std::vector<ForwardHop>> m_fwdMemo;
    mutable std::vector<std::pair<NodeId, std::uint32_t>> m_throughsBy;   // (caller, Through reference), sorted
    mutable std::vector<std::uint32_t>              m_throughRefs;       // parallel: the reference alone (throughsIn's span)
    mutable bool                                    m_throughsBuilt = false;
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
// The d=2 and d=3 rows (<rh>) are listed after the <vrs> rows, kImpactRingRowCap shown, next= the callers of the first unshown
// row's `calls`; each prints its call's forward width (slot_fns= for a field, cands= for a container). The tail has no rows:
// `--impact=<ring member>` lists it. Absent (no attribute, no rows) when the ring is empty: byte-identical.
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
    std::uint32_t stored = 0;         // the call's forward width (filled for the shown rows only)
};

struct ImpactRing
{
    std::size_t                hedged = 0;   // reaches_hedged=
    std::vector<ImpactRingRow> rows;         // d=2 in d=1 order, then d=3 in d=2 order
};

inline ImpactRing impactSlotRing( const IngestResult& ing, const BackwardHops& hops, std::span<const NodeId> seeds, std::span<const NodeId> reach )
{
    ImpactRing           out;
    const std::size_t    S = ing.symbols.size();
    std::vector<char>    known( S, 0 );   // an answered definition or a proven reach member: never counted here
    std::vector<char>    counted( S, 0 );
    std::vector<char>    level( S, 0 );   // 1..3: the ring depth a function joined at (a member never rejoins deeper)
    std::vector<char>    narrow( S, 0 );  // some chain to it keeps every hop within kBackHopWidth: its proven callers compose
    std::vector<char>    fieldNarrow( S, 0 );   // ... and every hop on that chain is a field call: it may take one more field hop
    for( const NodeId n : seeds ) { if( n < S ) { known[ n ] = 1; } }
    for( const NodeId n : reach ) { if( n < S ) { known[ n ] = 1; } }
    const auto count = [ & ]( NodeId n )
    {
        if( n < S && known[ n ] == 0 && counted[ n ] == 0 )
        {
            counted[ n ] = 1;
            ++out.hedged;
        }
    };
    const auto carry = [ & ]( NodeId to, const BackHop& h, bool narrowIn, bool fieldIn )
    {
        const bool w = h.width <= kBackHopWidth;
        narrow[ to ]      = char( narrow[ to ] != 0 || ( narrowIn && w ) );
        fieldNarrow[ to ] = char( fieldNarrow[ to ] != 0 || ( fieldIn && w && hopKind( h.via ).fieldCall ) );
    };
    std::vector<NodeId> d1;
    for( const NodeId s : seeds )
    {
        for( const BackHop& h : hops.backwardHops( s ) )
        {
            if( hopKind( h.via ).ring && h.from < S )
            {
                if( level[ h.from ] == 0 )
                {
                    level[ h.from ] = 1;
                    d1.push_back( h.from );
                }
                carry( h.from, h, true, true );
            }
        }
    }
    for( const NodeId f : d1 )
    {
        count( f );
    }
    std::vector<NodeId> d2;
    for( const NodeId f : d1 )
    {
        for( const BackHop& h : hops.backwardHops( f ) )
        {
            if( !hopKind( h.via ).ring || h.from == f || h.from >= S || known[ h.from ] != 0 || level[ h.from ] == 1 )
            {
                continue;   // a proven member, an answered definition or a d=1 function already speaks for itself
            }
            count( h.from );
            out.rows.push_back( ImpactRingRow{ h.from, f, h.via, h.ref, 2, 0 } );
            if( level[ h.from ] == 0 )
            {
                level[ h.from ] = 2;
                d2.push_back( h.from );
            }
            carry( h.from, h, narrow[ f ] != 0, fieldNarrow[ f ] != 0 );
        }
    }
    static_assert( kImpactRingSlotDepth == 3, "the ring walks d=1 and d=2 above and exactly one more field hop below" );
    std::vector<NodeId> d3;
    for( const NodeId f : d2 )
    {
        for( const BackHop& h : hops.backwardHops( f ) )
        {
            if( fieldNarrow[ f ] == 0 || !hopKind( h.via ).fieldCall || h.width > kBackHopWidth || h.from == f || h.from >= S || known[ h.from ] != 0
                || level[ h.from ] != 0 )
            {
                continue;   // only a field-only, narrow chain goes one deeper, and never back onto a member it already holds
            }
            level[ h.from ]  = 3;
            narrow[ h.from ] = 1;
            d3.push_back( h.from );
            count( h.from );
            out.rows.push_back( ImpactRingRow{ h.from, f, h.via, h.ref, 3, 0 } );
        }
    }
    // the tail: the proven callers of every member whose chain stayed narrow (one walk over all of them)
    std::vector<NodeId> composable;
    for( const std::vector<NodeId>* members : { &d1, &d2, &d3 } )
    {
        std::ranges::copy_if( *members, std::back_inserter( composable ), [ & ]( NodeId n ) { return narrow[ n ] != 0; } );
    }
    if( !composable.empty() )
    {
        for( const NodeId n : transitiveCallers( hops.graph(), composable ) )
        {
            count( n );
        }
    }
    for( std::size_t i = 0; i < out.rows.size() && i < kImpactRingRowCap; ++i )
    {
        ImpactRingRow& row = out.rows[ i ];
        row.stored         = hops.storedWidth( BackHop{ row.fn, row.via, 0, row.ref } );
    }
    return out;
}

inline std::string impactRingAttrXml( const ImpactRing& ring ) { return countFieldOrEmpty( "reaches_hedged", ring.hedged, /*json=*/false ); }
inline std::string impactRingKeyJson( const ImpactRing& ring ) { return countFieldOrEmpty( "reaches_hedged", ring.hedged, /*json=*/true ); }

inline std::string impactRingNext( const IngestResult& ing, const ImpactRing& ring )
{
    const std::size_t shown = std::min( ring.rows.size(), kImpactRingRowCap );
    return shown < ring.rows.size() ? nextFlag( "--callers=", ing.symbols[ ring.rows[ shown ].calls ].name ) : std::string();
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

// <ring d="2|3" total= shown=[ next=]><rh called_by= called_at= via= calls=[ d="3"] slot_fns=|cands=/>…</ring>, or "" when there is no row.
inline std::string impactRingXml( const IngestResult& ing, const ImpactRing& ring, const VrRender& rr )
{
    if( ring.rows.empty() )
    {
        return {};
    }
    std::vector<char> esc;
    const auto        ex    = [ & ]( std::string_view s ) { return std::string( escapeXml( s, esc ) ); };
    const std::size_t shown = std::min( ring.rows.size(), kImpactRingRowCap );
    std::string       out   = "<ring d=\"" + std::to_string( impactRingDepth( ring ) ) + "\" total=\"" + std::to_string( ring.rows.size() ) + "\" shown=\"" + std::to_string( shown ) + "\""
                      + nextAttrXml( impactRingNext( ing, ring ) ) + ">";
    for( std::size_t i = 0; i < shown; ++i )
    {
        const ImpactRingRow& row = ring.rows[ i ];
        const Reference&     c   = ing.references[ row.ref ];
        out += "<rh called_by=\"" + ex( ing.symbols[ row.fn ].name ) + "\" called_at=\"" + ex( vrPath( ing, c.fileId, rr ) ) + ":" + std::to_string( c.line )
             + "\" via=\"" + std::string( hopKind( row.via ).name ) + "\" calls=\"" + ex( ing.symbols[ row.calls ].name ) + "\""
             + ( row.depth == 3 ? std::string( " d=\"3\"" ) : std::string() ) + " " + std::string( impactRingStoredKey( row ) ) + "=\"" + std::to_string( row.stored ) + "\"/>";
    }
    return out + "</ring>";
}
// The --json twin: `,"ring":{"d":2|3,"total":N,"shown":M[,"next":…],"rows":[{"called_by":…,"called_at":…,"via":…,"calls":…[,"d":3],"slot_fns"|"cands":K}]}`, or "".
inline std::string impactRingJson( const IngestResult& ing, const ImpactRing& ring, const VrRender& rr )
{
    if( ring.rows.empty() )
    {
        return {};
    }
    const std::size_t shown = std::min( ring.rows.size(), kImpactRingRowCap );
    std::string       out   = ",\"ring\":{\"d\":" + std::to_string( impactRingDepth( ring ) ) + ",\"total\":" + std::to_string( ring.rows.size() ) + ",\"shown\":" + std::to_string( shown );
    if( const std::string next = impactRingNext( ing, ring ); !next.empty() )
    {
        out += ",\"next\":\"" + jsonStr( next ) + "\"";
    }
    out += ",\"rows\":[";
    for( std::size_t i = 0; i < shown; ++i )
    {
        const ImpactRingRow& row = ring.rows[ i ];
        const Reference&     c   = ing.references[ row.ref ];
        out += std::string( i == 0 ? "{" : ",{" ) + "\"called_by\":\"" + jsonStr( ing.symbols[ row.fn ].name ) + "\",\"called_at\":\""
             + jsonStr( vrPath( ing, c.fileId, rr ) + ":" + std::to_string( c.line ) ) + "\",\"via\":\"" + std::string( hopKind( row.via ).name )
             + "\",\"calls\":\"" + jsonStr( ing.symbols[ row.calls ].name ) + "\"" + ( row.depth == 3 ? std::string( ",\"d\":3" ) : std::string() )
             + ",\"" + std::string( impactRingStoredKey( row ) ) + "\":" + std::to_string( row.stored ) + "}";
    }
    return out + "]}";
}

inline constexpr std::string_view kImpactRingLegend =
    "reaches_hedged=N (absent when 0): N functions MAY reach of= through a HEDGED hop: a call through a struct FIELD (the IX-1 slot "
    "join) or through a container element or called parameter (container), one or two such hops back (three when every hop is a field "
    "call within 16 callers), plus every function that reaches one of those through proven calls (only past hops within 16 callers). "
    "Not counted in reaches= and not an answered definition. The one-hop set has NO width bound (a wide field's callers are all counted). "
    "A hedge: one hedged hop makes the whole chain hedged; never a proven caller, never in reaches=/by_depth=/radius_*. ";
inline constexpr std::string_view kImpactRingRowsLegend =
    "<ring d=2 total= shown=> lists the two-hop ones (d=3 when a third field hop is listed too): <rh called_by= called_at= via=slot|slot-name|container calls=> = called_by may call "
    "calls= through a field or container at called_at=; d=3 the third field hop; slot_fns=K the functions that field holds, cands=K the functions "
    "that container may hold (the forward width, not capped); first 16, next= the callers of the first unshown calls=. ";
// `on`: reaches_hedged= rides; `rows`: the <ring> element rides too (XML/JSON; the columnar form carries the count alone).
inline std::string impactRingLegend( bool on, bool rows )
{
    std::string out;
    if( on )
    {
        out.append( kImpactRingLegend );
        out.append( rows ? kImpactRingRowsLegend : std::string_view() );
    }
    return out;
}

}   // namespace rw
