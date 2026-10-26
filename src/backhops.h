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

    // The class definition owning definition `d` (ownerOf below, built on first use); kNoNode when none. QUEUE-HOP reads it.
    NodeId classOf( NodeId d ) const
    {
        buildOnce();
        return d < m_ing.symbols.size() ? ownerOf( d ) : kNoNode;
    }
    // Every in-tree class-like definition named `name` (a base class written by name binds to each of them).
    void classesNamed( std::string_view name, std::vector<NodeId>& out ) const
    {
        buildOnce();
        for( auto it = std::ranges::lower_bound( m_classIds, name, sortutil::svLess, &ClassName::name ); it != m_classIds.end() && it->name == name; ++it )
        {
            out.push_back( it->id );
        }
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
//   queue      (QUEUE-HOP) a hand-off, not a call: the producer puts onto a queue field of its own object (`self.q.put( v )`) that
//              the sink reads (`self.q.get()`) in the same class or a base/subclass of it (QueueHops below); width = the producing
//              functions. It has no <vr> row of its own, so the --impact ring LISTS it at d=1 (`atOne`).
enum class BackHopVia : std::uint8_t { Slot, SlotName, Declined, Container, Queue };

struct HopKindInfo
{
    std::string_view name;
    bool             fieldCall;   // a call through a struct field (slot, slot-name)
    bool             ring;        // the --impact ring follows it
    bool             atOne;       // the ring lists its d=1 hops as rows (no --callers <vr> row speaks for them)
};
inline constexpr std::array kHopKinds = { HopKindInfo{ "slot", true, true, false }, HopKindInfo{ "slot-name", true, true, false },
                                          HopKindInfo{ "declined", false, false, false }, HopKindInfo{ "container", false, true, false },
                                          HopKindInfo{ "queue", false, true, true } };   // indexed by BackHopVia
inline constexpr std::size_t kBackHopViaCount = 5;
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

// QUEUE-HOP: a hand-off through a queue field of one object (Python). A PUT site is a call `self.X.put( v )` / `self.X.put_nowait( v )`
// with exactly one positional argument, a GET site `self.X.get()` / `self.X.get_nowait()` with none (a keyed `d.get( k )` or a two-
// argument `cache.put( k, v )` is no queue operation), both on the receiver keyword `self` (receiverOf's FieldOfThis) and both with
// a counted argument list. A put site and a get site PAIR when they name the same attribute X and their classes are one family:
// the same class, or one is an in-tree base of the other by name (the Python inheritance refs, walked to the root; a sibling
// class's get never reads this object's queue). The READERS of a get site are the function holding it and, one level up, its
// proven callers in that same family (`message = await self._get_message()`: the loop that dispatches the message reads it).
// What it does NOT mean: no call edge; a reader is not proven to receive any given item. A queue the class hands out (a worker's
// parameter, a local alias `q = self.q`) has readers this rule does not see: the readers listed are a floor, every hop a hedge.
class QueueHops
{
public:
    QueueHops( const IngestResult& ing, const Graph& g, const DeclinedCrossRule& rule ) : m_ing( ing ), m_g( g ), m_rule( rule ) {}

    // The sink's QUEUE hops: one per producing function of every queue the sink reads (the sink itself excluded: a reader that
    // re-puts onto its own queue is no way in); width = those producers.
    void backHops( NodeId sink, std::vector<BackHop>& out ) const
    {
        build();
        std::vector<BackHop> rows;
        for( const Pair& p : m_pairs )
        {
            if( p.reader == sink && m_sites[ p.put ].fn != sink )
            {
                rows.push_back( BackHop{ m_sites[ p.put ].fn, BackHopVia::Queue, 0, m_sites[ p.put ].ref } );
            }
        }
        keepOneHopPerCaller( rows );
        for( BackHop& h : rows )
        {
            h.width = std::uint32_t( rows.size() );
        }
        out.insert( out.end(), rows.begin(), rows.end() );
    }

    // The readers of put reference `ref` (distinct, ascending — m_pairs is sorted by site, then reader; the put's own function
    // excluded): one binary search for the site (m_sites is in reference order), one for its pairs.
    std::vector<NodeId> readersOf( std::uint32_t ref ) const
    {
        build();
        std::vector<NodeId> fns;
        const auto site = std::ranges::lower_bound( m_sites, ref, {}, &Site::ref );
        if( site == m_sites.end() || site->ref != ref || !site->put )
        {
            return fns;
        }
        const std::uint32_t idx = std::uint32_t( site - m_sites.begin() );
        for( auto it = std::ranges::lower_bound( m_pairs, idx, {}, &Pair::put ); it != m_pairs.end() && it->put == idx; ++it )
        {
            if( it->reader != site->fn )
            {
                fns.push_back( it->reader );
            }
        }
        return fns;
    }

    // The put references made in u, ascending (one sorted (function, reference) table, so a walk never rescans the sites per node).
    std::vector<std::uint32_t> putsIn( NodeId u ) const
    {
        build();
        std::vector<std::uint32_t> refs;
        for( auto it = std::ranges::lower_bound( m_putsByFn, std::pair<NodeId, std::uint32_t>( u, 0u ) ); it != m_putsByFn.end() && it->first == u; ++it )
        {
            refs.push_back( it->second );
        }
        return refs;
    }

private:
    struct Site
    {
        std::string_view field;
        NodeId           fn  = kNoNode;
        NodeId           cls = kNoNode;
        std::uint32_t    ref = 0;
        bool             put = false;
    };
    struct Pair
    {
        std::uint32_t put    = 0;   // index into m_sites
        NodeId        reader = kNoNode;
    };

    static bool queueOp( const Reference& r, bool& put ) noexcept
    {
        if( r.lang != Lang::Python || r.role != RefRole::Call || r.isInherit || r.recv != RecvKind::FieldOfThis || r.fieldName.empty() || !r.argCountKnown )
        {
            return false;
        }
        const std::string_view m = r.calleeName;
        put                      = m == "put" || m == "put_nowait";
        return ( put && r.argCount == 1 ) || ( ( m == "get" || m == "get_nowait" ) && r.argCount == 0 );
    }

    // Whether class b is a or one of a's in-tree ancestors (by base name, every same-named class; visited once each).
    bool derivesFrom( NodeId a, NodeId b ) const
    {
        std::vector<NodeId> todo{ a };
        std::vector<NodeId> seen;
        while( !todo.empty() )
        {
            const NodeId c = todo.back();
            todo.pop_back();
            if( c == b )
            {
                return true;
            }
            if( std::ranges::find( seen, c ) != seen.end() )
            {
                continue;   // an inheritance cycle by name (two same-named classes) ends here
            }
            seen.push_back( c );
            const auto lo = std::ranges::lower_bound( m_bases, std::pair<NodeId, NodeId>( c, 0 ) );
            for( auto it = lo; it != m_bases.end() && it->first == c; ++it )
            {
                todo.push_back( it->second );
            }
        }
        return false;
    }
    bool oneFamily( NodeId a, NodeId b ) const { return a != kNoNode && b != kNoNode && ( derivesFrom( a, b ) || derivesFrom( b, a ) ); }

    void build() const
    {
        if( m_built )
        {
            return;
        }
        m_built = true;
        for( std::uint32_t i = 0; i < m_ing.references.size(); ++i )
        {
            const Reference& r   = m_ing.references[ i ];
            bool             put = false;
            if( r.isInherit && r.lang == Lang::Python && r.fromSymbol < m_ing.symbols.size() )
            {
                std::vector<NodeId> bases;
                m_rule.classesNamed( r.calleeName, bases );
                for( const NodeId b : bases )
                {
                    m_bases.emplace_back( r.fromSymbol, b );
                }
            }
            else if( r.fromSymbol < m_ing.symbols.size() && queueOp( r, put ) )
            {
                m_sites.push_back( Site{ r.fieldName, r.fromSymbol, m_rule.classOf( r.fromSymbol ), i, put } );
                if( put )
                {
                    m_putsByFn.emplace_back( r.fromSymbol, i );
                }
            }
        }
        if( !m_sites.empty() )
        {
            std::ranges::sort( m_bases );
            std::ranges::sort( m_putsByFn );
            pairSites();   // a tree with no queue operation pays one scan
        }
    }

    // Each get site's readers (its function + one level of same-family proven callers) paired with every same-attribute,
    // same-family put site; sorted and distinct.
    void pairSites() const
    {
        const std::size_t N  = m_g.inEdges.rows();
        const auto*       ro = m_g.inEdges.rowOffsets();
        const auto*       ci = m_g.inEdges.colIndices();
        for( const Site& get : m_sites )
        {
            if( get.put || get.cls == kNoNode )
            {
                continue;
            }
            std::vector<NodeId> readers{ get.fn };
            for( std::uint32_t k = get.fn < N ? ro[ get.fn ] : 0; get.fn < N && k < ro[ get.fn + 1 ]; ++k )
            {
                const NodeId c = ci[ k ];
                if( c < m_ing.symbols.size() && c != get.fn && oneFamily( m_rule.classOf( c ), get.cls ) )
                {
                    readers.push_back( c );   // one level up: a same-family caller of the reader reads the item it returns
                }
            }
            for( std::uint32_t p = 0; p < m_sites.size(); ++p )
            {
                if( m_sites[ p ].put && m_sites[ p ].field == get.field && oneFamily( m_sites[ p ].cls, get.cls ) )
                {
                    for( const NodeId rd : readers )
                    {
                        m_pairs.push_back( Pair{ p, rd } );
                    }
                }
            }
        }
        std::ranges::sort( m_pairs, [ ]( const Pair& a, const Pair& b ) { return a.put != b.put ? a.put < b.put : a.reader < b.reader; } );
        m_pairs.erase( std::ranges::unique( m_pairs, [ ]( const Pair& a, const Pair& b ) { return a.put == b.put && a.reader == b.reader; } ).begin(), m_pairs.end() );
    }

    const IngestResult&                           m_ing;
    const Graph&                                  m_g;
    const DeclinedCrossRule&                      m_rule;
    mutable bool                                  m_built = false;
    mutable std::vector<Site>                     m_sites;   // every put / get site, in reference order
    mutable std::vector<std::pair<NodeId, NodeId>> m_bases;  // (class, in-tree base), sorted
    mutable std::vector<Pair>                     m_pairs;   // (put site, reader), sorted, distinct
    mutable std::vector<std::pair<NodeId, std::uint32_t>> m_putsByFn;   // (function, put reference), sorted
};

class BackwardHops
{
public:
    BackwardHops( const IngestResult& ing, const Graph& g, const ValueRefIndex& values ) : m_ing( ing ), m_g( g ), m_values( values ), m_rule( ing, g ), m_queue( ing, g, m_rule ) {}

    const DeclinedCrossRule& declinedRule() const noexcept { return m_rule; }
    const QueueHops&         queue() const noexcept { return m_queue; }
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
        m_queue.backHops( sink, out );
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
    QueueHops                                    m_queue;   // after m_rule: it holds a reference to it
    mutable HashMap<NodeId, std::vector<BackHop>> m_memo;
};

// The forward width of a backward hop's call: the functions that field (slot) or container may hold, the readers of a queue
// put; 0 for a declined hop.
inline std::uint32_t backHopStoredWidth( const IngestResult& ing, const BackwardHops& hops, const BackHop& h )
{
    if( hopKind( h.via ).fieldCall )
    {
        return std::uint32_t( slotSiteOf( ing, hops.values(), h.ref ).targets.size() );
    }
    if( h.via == BackHopVia::Queue )
    {
        return std::uint32_t( hops.queue().readersOf( h.ref ).size() );
    }
    return h.via == BackHopVia::Container ? std::uint32_t( containerHolds( hops.values(), h.ref ).size() ) : 0u;
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
            confirm( u, site.targets, site.ref, BackHopVia::Slot, out );
        }
        for( const std::uint32_t t : throughsIn( u ) )
        {
            confirm( u, containerHolds( m_back.values(), t ), t, BackHopVia::Container, out );
        }
        for( const std::uint32_t put : m_back.queue().putsIn( u ) )
        {
            confirm( u, m_back.queue().readersOf( put ), put, BackHopVia::Queue, out );   // QUEUE-HOP: after every older kind
        }
        return m_memo.try_emplace( u, std::move( out ) ).first->second;
    }

private:
    // Each of `fns` whose backward hops name u by `kind` (any field kind for a field call) becomes a forward row.
    void confirm( NodeId u, const std::vector<NodeId>& fns, std::uint32_t call, BackHopVia kind, std::vector<ForwardHop>& out ) const
    {
        const bool fieldCall = hopKind( kind ).fieldCall;
        for( const NodeId f : fns )
        {
            const std::span<const BackHop> hops = m_back.backwardHops( f );
            const auto it = std::ranges::find_if( hops, [ & ]( const BackHop& h )
            {
                return h.from == u && ( fieldCall ? hopKind( h.via ).fieldCall : h.via == kind );
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

// The ring's walk state (one answer): which depth each function joined at, whether some chain to it stayed narrow, and whether
// such a chain is also field-only. Each step is one method so impactSlotRing reads as the contract above.
class ImpactRingWalk
{
public:
    ImpactRingWalk( const IngestResult& ing, const BackwardHops& hops, std::span<const NodeId> seeds, std::span<const NodeId> reach )
        : m_hops( hops ), m_S( ing.symbols.size() ), m_known( m_S, 0 ), m_counted( m_S, 0 ), m_level( m_S, 0 ), m_narrow( m_S, 0 ), m_fieldNarrow( m_S, 0 )
    {
        for( const std::span<const NodeId> set : { seeds, reach } )
        {
            for( const NodeId n : set ) { if( n < m_S ) { m_known[ n ] = 1; } }
        }
    }

    // d=1: each answered definition's ring hops (all of them counted, whatever their width); a kind with no <vr> row of its own
    // (atOne: queue) is kept as a d=1 row, listed first by depthTwo
    void depthOne( std::span<const NodeId> seeds )
    {
        for( const NodeId s : seeds )
        {
            for( const BackHop& h : m_hops.backwardHops( s ) )
            {
                if( hopKind( h.via ).ring && h.from < m_S )
                {
                    if( hopKind( h.via ).atOne && m_known[ h.from ] == 0 && m_level[ h.from ] == 0 )
                    {
                        m_atOne.push_back( ImpactRingRow{ h.from, s, h.via, h.ref, 1, 0 } );
                    }
                    join( h.from, 1, m_d1 );
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
        out.rows.insert( out.rows.end(), m_atOne.begin(), m_atOne.end() );
        for( const NodeId f : m_d1 )
        {
            for( const BackHop& h : m_hops.backwardHops( f ) )
            {
                if( hopKind( h.via ).ring && fresh( f, h, 1 ) )
                {
                    count( h.from );
                    out.rows.push_back( ImpactRingRow{ h.from, f, h.via, h.ref, 2, 0 } );
                    join( h.from, 2, m_d2 );
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
                    join( h.from, 3, m_d3 );
                    m_narrow[ h.from ] = 1;
                    count( h.from );
                    out.rows.push_back( ImpactRingRow{ h.from, f, h.via, h.ref, 3, 0 } );
                }
            }
        }
    }
    // the tail: the proven callers of every member whose chain stayed narrow (one walk over all of them)
    void tail()
    {
        std::vector<NodeId> composable;
        for( const std::vector<NodeId>* members : { &m_d1, &m_d2, &m_d3 } )
        {
            std::ranges::copy_if( *members, std::back_inserter( composable ), [ & ]( NodeId n ) { return m_narrow[ n ] != 0; } );
        }
        for( const NodeId n : composable.empty() ? std::vector<NodeId>() : transitiveCallers( m_hops.graph(), composable ) )
        {
            count( n );
        }
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
    void join( NodeId n, char depth, std::vector<NodeId>& at )
    {
        if( m_level[ n ] == 0 )
        {
            m_level[ n ] = depth;
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
    std::vector<char>   m_known, m_counted, m_level, m_narrow, m_fieldNarrow;
    std::vector<NodeId> m_d1, m_d2, m_d3;
    std::vector<ImpactRingRow> m_atOne;   // the d=1 rows of an atOne kind (queue)
};

inline ImpactRing impactSlotRing( const IngestResult& ing, const BackwardHops& hops, std::span<const NodeId> seeds, std::span<const NodeId> reach )
{
    static_assert( kImpactRingSlotDepth == 3, "the walk below takes d=1, d=2 and exactly one more field hop" );
    ImpactRing     out;
    ImpactRingWalk walk( ing, hops, seeds, reach );
    walk.depthOne( seeds );
    walk.depthTwo( out );
    walk.depthThree( out );
    walk.tail();
    out.hedged = walk.hedged();
    for( std::size_t i = 0; i < out.rows.size() && i < kImpactRingRowCap; ++i )
    {
        ImpactRingRow& row = out.rows[ i ];
        row.stored         = backHopStoredWidth( ing, hops, BackHop{ row.fn, row.via, 0, row.ref } );
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

// The deepest row listed: 3, 2, or 1 (only queue d=1 rows).
inline std::uint32_t impactRingDepth( const ImpactRing& ring )
{
    return ring.rows.empty() ? 1u : std::ranges::max( ring.rows, {}, &ImpactRingRow::depth ).depth;
}
// A ring row's d= attribute: printed for d=1 (a queue hop) and d=3; a d=2 row stays bare (byte-identity of the older rows).
inline bool impactRingRowPrintsDepth( const ImpactRingRow& row ) noexcept { return row.depth != 2; }

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
             + ( impactRingRowPrintsDepth( row ) ? " d=\"" + std::to_string( row.depth ) + "\"" : std::string() ) + " " + std::string( impactRingStoredKey( row ) ) + "=\"" + std::to_string( row.stored ) + "\"/>";
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
             + "\",\"calls\":\"" + jsonStr( ing.symbols[ row.calls ].name ) + "\"" + ( impactRingRowPrintsDepth( row ) ? ",\"d\":" + std::to_string( row.depth ) : std::string() )
             + ",\"" + std::string( impactRingStoredKey( row ) ) + "\":" + std::to_string( row.stored ) + "}";
    }
    return out + "]}";
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
// QUEUE-HOP, present-only: rides beside the rows legend when a row is a queue hop (the --path bridge reads it too).
inline constexpr std::string_view kQueueHopLegend =
    "rh via=queue d=1 (bridge s via=queue): a HAND-OFF, not a call: called_by (the previous row) puts onto a queue field of its own object (self.X.put/put_nowait, one argument) and "
    "calls= (this row) takes from it (self.X.get/get_nowait) in the same class or a base or subclass of it, or is a proven caller in that family of "
    "such a function (one level); cands=K the readers of that put, a floor (a queue handed out of the class has readers not seen). ";
// Whether any ring row is a queue hop: the queue clause rides.
inline bool impactRingHasQueue( const ImpactRing& ring ) noexcept
{
    return std::ranges::contains( ring.rows, BackHopVia::Queue, &ImpactRingRow::via );
}
// `on`: reaches_hedged= rides; `rows`: the <ring> element rides too (XML/JSON; the columnar form carries the count alone); `queue`:
// a listed row is a queue hop.
inline std::string impactRingLegend( bool on, bool rows, bool queue = false )
{
    std::string out;
    if( on )
    {
        out.append( kImpactRingLegend );
        out.append( rows ? kImpactRingRowsLegend : std::string_view() );
        out.append( rows && queue ? kQueueHopLegend : std::string_view() );
    }
    return out;
}

}   // namespace rw
