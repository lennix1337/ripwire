#pragma once
// handlerrule.h — ON-NAME-HANDLERS-V2: a posted message CLASS names its handler methods (the Python layer of the name-key hop
// family, backhops.h NameKeyHops).
//
// THE GAP. A message framework dispatches by a name the message's class carries: textual's `Message.__init_subclass__` writes
// `cls.handler_name = f"on_{name}"` (name = camel_to_snake of the owner and class names), and the pump looks the handler up
// with `cls.__dict__.get( method_name )` where method_name is a PARAMETER. Nothing at the lookup is provable (every on_* def
// fits it), so lane string-keyed declined it. The class is known at the POST SITE: `self.post_message( Button.Pressed( self ) )`
// names Button.Pressed, whose handler name is on_button_pressed.
//
// THE CONTRACT. HandlerRules reads the RULE from the repo's own source, never from a framework list:
//   rule       a Python `__init_subclass__` of class B with a line `cls.A = f"P{v}"` (literal identifier prefix P, one hole) that
//              calls a converter whose body is the `[a-z][A-Z]` regex plus `.lower()` (camel_to_snake: an underscore between a
//              lower and an upper letter, then all lower), on `cls.__qualname__` cut to its last two parts (`[-2:]`: owner
//              class + class) or on `cls.__name__` alone; a `{namespace}_` branch makes a `namespace="x"` class keyword the
//              owner part. Any other shape: no rule, no hop (a floor). A class body that writes A is overwritten by the
//              rule (it runs after the body), so the rule decides.
//   readers    the functions outside test paths that read A; their classes hold the POSTERS (every method name they define).
//   post site  a call to a poster whose FIRST argument is a construction `X( … )`, `self.X( … )`, `Owner.X( … )` or
//              `module.X( … )` of a subclass of B (written bases, valuerefindex.h ClassFamilies). A variable argument,
//              a factory (`X.from_event( … )`) or an unresolved class: no hop.
//   targets    the methods (a definition inside a class) named by the rule's name for that class: a hedge — which of them
//              runs depends on the receiving object's class, which the index does not decide.
// The hop is never a call edge: consumers print it via="name-key" with its width (cands=), never in reaches=/hops=.

#include "darkflags.h"          // readWhole — the rule and the post site are read from the file text
#include "filter.h"             // isTestPath — a test's reader does not make posters
#include "model.h"
#include "resolve.h"            // trimWs
#include "infra/namesplit.h"   // isIdentChar
#include "valuerefindex.h"      // ClassFamilies — owner class and written-base family

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace rw
{

struct HandlerRule
{
    NodeId        base   = kNoNode;   // the class whose __init_subclass__ writes the attribute
    NodeId        writer = kNoNode;   // that __init_subclass__
    std::uint32_t fileId = 0;         // where the rule's write is (p=)
    std::uint32_t line   = 0;
    std::string   attr;               // the class attribute written (handler_name)
    std::string   prefix;             // the template's literal prefix (on_)
    bool          qualified = false;  // __qualname__[-2:]: owner class + class; else the class name alone
    bool          nsKeyword = false;  // a namespace= class keyword replaces the owner part
};

struct HandlerReader
{
    NodeId        fn   = kNoNode;   // the function reading the attribute
    std::uint32_t line = 0;         // the line of its first read
};

struct HandlerPostSite
{
    std::uint32_t            ref = 0;     // the poster call reference
    std::vector<std::string> names;       // the handler names its message class(es) carry, sorted, unique
    std::vector<NodeId>      classes;     // the message class definitions the construction may mean
};

// camel_to_snake as the `[a-z][A-Z]` rule states it: an underscore between each non-overlapping lower-upper pair, then lower.
inline std::string snakeOfCamel( std::string_view s )
{
    std::string out;
    out.reserve( s.size() + 4 );
    for( std::size_t i = 0; i < s.size(); ++i )
    {
        const unsigned char c = static_cast<unsigned char>( s[i] );
        out.push_back( char( std::tolower( c ) ) );
        if( std::islower( c ) && i + 1 < s.size() && std::isupper( static_cast<unsigned char>( s[ i + 1 ] ) ) )
        {
            out.push_back( '_' );
            out.push_back( char( std::tolower( static_cast<unsigned char>( s[ i + 1 ] ) ) ) );
            ++i;
        }
    }
    return out;
}

// Two reference-ordered site lists merged in reference order (NameKeyHops: template lookups beside post sites).
inline std::vector<std::uint32_t> mergedSiteLists( std::vector<std::uint32_t> sites, const std::vector<std::uint32_t>& more )
{
    const std::size_t mid = sites.size();
    sites.insert( sites.end(), more.begin(), more.end() );
    std::ranges::inplace_merge( sites, sites.begin() + std::ptrdiff_t( mid ) );
    return sites;
}

class HandlerRules
{
public:
    explicit HandlerRules( const IngestResult& ing ) : m_ing( ing ), m_classes( ing ) {}

    // Built on first use; a tree with no Python __init_subclass__ write pays one reference scan.
    bool any() const
    {
        build();
        return !m_sites.empty();
    }
    const std::vector<HandlerRule>&     rules() const { build(); return m_rules; }
    const std::vector<HandlerPostSite>& sites() const { build(); return m_sites; }
    const ClassFamilies&                classes() const noexcept { return m_classes; }
    // The post site at reference `ref`, or null.
    const HandlerPostSite* siteAt( std::uint32_t ref ) const
    {
        build();
        const auto it = m_siteOfRef.find( ref );
        return it == m_siteOfRef.end() ? nullptr : &m_sites[ it->second ];
    }
    // The post sites whose message names handler `d` (a method), in reference order.
    std::vector<std::uint32_t> sitesNaming( NodeId d ) const
    {
        std::vector<std::uint32_t> out;
        if( !any() || !isHandlerShaped( d ) )
        {
            return out;
        }
        for( const HandlerPostSite& s : m_sites )
        {
            if( m_ing.references[ s.ref ].fromSymbol != d && std::ranges::binary_search( s.names, std::string_view( m_ing.symbols[d].name ) ) )
            {
                out.push_back( s.ref );
            }
        }
        return out;
    }
    // The post sites made in `u`, in reference order.
    std::vector<std::uint32_t> sitesIn( NodeId u ) const
    {
        std::vector<std::uint32_t> out;
        if( any() )
        {
            for( const HandlerPostSite& s : m_sites )
            {
                if( m_ing.references[ s.ref ].fromSymbol == u )
                {
                    out.push_back( s.ref );
                }
            }
        }
        return out;
    }
    // The methods named by post site `ref`'s handler names, in symbol order.
    std::vector<NodeId> targets( std::uint32_t ref ) const
    {
        std::vector<NodeId> out;
        if( const HandlerPostSite* s = siteAt( ref ) )
        {
            for( const std::string& n : s->names )
            {
                const auto it = m_methodsByName.find( std::string_view( n ) );
                if( it != m_methodsByName.end() )
                {
                    out.insert( out.end(), it->second.begin(), it->second.end() );
                }
            }
            std::ranges::sort( out );
            out.erase( std::ranges::unique( out ).begin(), out.end() );
        }
        return out;
    }
    // The rule's name for message class `c` (empty when no rule covers it).
    std::string handlerNameOf( NodeId c ) const
    {
        build();
        for( const HandlerRule& r : m_rules )
        {
            if( c != r.base && m_classes.descendsFrom( c, r.base ) )
            {
                return nameUnder( r, c );
            }
        }
        return {};
    }
    // The functions outside test paths that read the rule attribute, in symbol order.
    const std::vector<HandlerReader>& readers() const { build(); return m_readers; }
    // The file text (empty when unreadable).
    const std::string& text( std::uint32_t fileId ) const
    {
        static const std::string kEmpty;
        if( fileId >= m_ing.files.size() )
        {
            return kEmpty;
        }
        if( m_text.size() != m_ing.files.size() )
        {
            m_text.resize( m_ing.files.size() );
            m_textRead.assign( m_ing.files.size(), 0 );
        }
        if( m_textRead[ fileId ] == 0 )
        {
            m_textRead[ fileId ] = 1;
            m_text[ fileId ]     = darkflags::readWhole( diskPath( m_ing, fileId ) ).value_or( std::string() );
        }
        return m_text[ fileId ];
    }

private:
    // A Python function definition inside a class (collectMessagesAndMethods marks them).
    bool isHandlerShaped( NodeId d ) const
    {
        return d < m_inClass.size() && m_inClass[d] != 0;
    }
    std::string_view span( const Symbol& s ) const
    {
        const std::string& t = text( s.fileId );
        return s.sigStartByte < s.endByte && s.endByte <= t.size() ? std::string_view( t ).substr( s.sigStartByte, s.endByte - s.sigStartByte ) : std::string_view();
    }
    // `cls.A = f"P{v}"` (either quote) → prefix P; nullopt for any other line.
    static std::optional<std::string> templatePrefix( std::string_view line, std::string_view attr )
    {
        line = trimWs( line );
        const std::string head = "cls." + std::string( attr );
        if( !line.starts_with( head ) )
        {
            return std::nullopt;
        }
        line = trimWs( line.substr( head.size() ) );
        if( !line.starts_with( '=' ) || line.starts_with( "==" ) )
        {
            return std::nullopt;
        }
        line = trimWs( line.substr( 1 ) );
        if( line.size() < 6 || line[0] != 'f' || ( line[1] != '"' && line[1] != '\'' ) || line.back() != line[1] )
        {
            return std::nullopt;
        }
        const std::string_view body = line.substr( 2, line.size() - 3 );
        const std::size_t      open = body.find( '{' );
        if( open == 0 || open == std::string_view::npos || body.back() != '}' || body.find( '{', open + 1 ) != std::string_view::npos
            || !std::ranges::all_of( body.substr( 0, open ), namesplit::isIdentChar ) || !std::ranges::all_of( body.substr( open + 1, body.size() - open - 2 ), namesplit::isIdentChar ) )
        {
            return std::nullopt;
        }
        return std::string( body.substr( 0, open ) );
    }
    // The writer calls a camel_to_snake converter: a callee defined in Python whose body holds the `[a-z][A-Z]` rule and `.lower()`.
    bool callsSnakeConverter( NodeId writer ) const
    {
        for( const Reference& r : m_ing.references )
        {
            if( r.fromSymbol != writer || r.role != RefRole::Call )
            {
                continue;
            }
            for( const Symbol& f : m_ing.symbols )
            {
                if( f.name == r.calleeName && f.lang == Lang::Python && f.kind == SymKind::Function )
                {
                    const std::string_view body = span( f );
                    if( body.find( "[a-z][A-Z]" ) != std::string_view::npos && body.find( ".lower()" ) != std::string_view::npos )
                    {
                        return true;
                    }
                }
            }
        }
        return false;
    }
    // One `cls.A = f"P{v}"` line of __init_subclass__ `writer` (at byte `at` of its file) as a rule, or nullopt.
    std::optional<HandlerRule> ruleAt( NodeId writer, std::string_view line, std::uint32_t lineNo ) const
    {
        const std::string_view t = trimWs( line );
        std::size_t            e = 4;
        while( e < t.size() && namesplit::isIdentChar( t[e] ) )
        {
            ++e;
        }
        if( !t.starts_with( "cls." ) || e == 4 )
        {
            return std::nullopt;
        }
        HandlerRule rule;
        rule.attr = std::string( t.substr( 4, e - 4 ) );
        const std::optional<std::string> prefix = templatePrefix( t, rule.attr );
        rule.writer = writer;
        rule.base   = m_classes.ownerOf( writer );
        if( !prefix || rule.base == kNoNode || !callsSnakeConverter( writer ) )
        {
            return std::nullopt;
        }
        const std::string_view body = span( m_ing.symbols[ writer ] );
        rule.qualified = body.find( "__qualname__" ) != std::string_view::npos;
        if( rule.qualified ? body.find( "[-2:]" ) == std::string_view::npos : body.find( "cls.__name__" ) == std::string_view::npos )
        {
            return std::nullopt;   // a qualname cut other than the last two parts, or no class name at all: not a rule we read
        }
        rule.nsKeyword = body.find( "{namespace}_" ) != std::string_view::npos;
        rule.fileId    = m_ing.symbols[ writer ].fileId;
        rule.line      = lineNo;
        rule.prefix    = *prefix;
        return rule;
    }
    // `namespace="x"` in class c's header, else empty.
    std::string namespaceKeyword( NodeId c ) const
    {
        const Symbol&          s    = m_ing.symbols[c];
        const std::string_view all  = span( s );
        const std::string_view head = all.substr( 0, std::min( all.find( ':' ), all.size() ) );
        const std::size_t      at   = head.find( "namespace" );
        if( at == std::string_view::npos )
        {
            return {};
        }
        std::string_view v = trimWs( head.substr( at + 9 ) );
        if( !v.starts_with( '=' ) )
        {
            return {};
        }
        v = trimWs( v.substr( 1 ) );
        if( v.size() < 2 || ( v[0] != '"' && v[0] != '\'' ) )
        {
            return {};
        }
        const std::size_t close = v.find( v[0], 1 );
        return close == std::string_view::npos ? std::string() : std::string( v.substr( 1, close - 1 ) );
    }
    std::string nameUnder( const HandlerRule& r, NodeId c ) const
    {
        const std::string ns = r.nsKeyword ? namespaceKeyword( c ) : std::string();
        if( !ns.empty() )
        {
            return r.prefix + ns + "_" + snakeOfCamel( m_ing.symbols[c].name );
        }
        const NodeId owner = r.qualified ? m_classes.ownerOf( c ) : kNoNode;
        return r.prefix + ( owner != kNoNode ? snakeOfCamel( m_ing.symbols[ owner ].name ) + "_" : std::string() ) + snakeOfCamel( m_ing.symbols[c].name );
    }
    // The construction a post site's first argument spells: `a.b.X(` after `callee(` → {a, b, X}; empty when it is not one.
    std::vector<std::string_view> constructionAt( const Reference& r ) const
    {
        const std::string_view t( text( r.fileId ) );
        std::size_t at = r.startByte < t.size() ? t.find( r.calleeName, r.startByte ) : std::string_view::npos;
        if( at == std::string_view::npos || at > r.startByte + 256 )
        {
            return {};
        }
        std::size_t i    = at + r.calleeName.size();
        const auto  skip = [ & ] { while( i < t.size() && std::isspace( static_cast<unsigned char>( t[i] ) ) ) { ++i; } };
        skip();
        if( i >= t.size() || t[i] != '(' )
        {
            return {};
        }
        ++i;
        skip();
        std::vector<std::string_view> segs;
        for( ;; )
        {
            const std::size_t b = i;
            while( i < t.size() && namesplit::isIdentChar( t[i] ) )
            {
                ++i;
            }
            if( i == b || std::isdigit( static_cast<unsigned char>( t[b] ) ) )
            {
                return {};
            }
            segs.push_back( t.substr( b, i - b ) );
            if( i < t.size() && t[i] == '.' )
            {
                ++i;
                continue;
            }
            break;
        }
        skip();
        return i < t.size() && t[i] == '(' ? segs : std::vector<std::string_view>();
    }
    // The message classes construction `segs` in reference r may mean: same name; the owner the qualifier spells.
    std::vector<NodeId> classesFor( const Reference& r, std::span<const std::string_view> segs ) const
    {
        std::vector<NodeId> out;
        const auto it = m_messagesByName.find( segs.back() );
        if( it == m_messagesByName.end() )
        {
            return out;
        }
        const NodeId           caller = r.fromSymbol < m_ing.symbols.size() ? m_classes.ownerOf( r.fromSymbol ) : kNoNode;
        const std::string_view q      = segs.size() >= 2 ? segs[ segs.size() - 2 ] : std::string_view();
        const bool             viaSelf = segs.size() == 2 && ( q == "self" || q == "cls" );
        const bool             viaOwner = !viaSelf && !q.empty() && std::isupper( static_cast<unsigned char>( q.front() ) );
        std::ranges::copy_if( it->second, std::back_inserter( out ), [ & ]( NodeId c )
        {
            const NodeId owner = m_classes.ownerOf( c );
            if( viaSelf )
            {
                return owner != kNoNode && caller != kNoNode && m_classes.sameFamily( caller, owner );
            }
            if( viaOwner )
            {
                return owner != kNoNode && m_ing.symbols[ owner ].name == q;
            }
            return owner == kNoNode;   // a bare or module-qualified name: a top-level class
        } );
        return out;
    }
    // Every Python __init_subclass__'s lines, read from the file (the index keeps no attribute writes on a lean verb).
    void collectRules() const
    {
        for( const Symbol& w : m_ing.symbols )
        {
            if( w.lang != Lang::Python || w.name != "__init_subclass__" || ( w.kind != SymKind::Function && w.kind != SymKind::Method ) )
            {
                continue;
            }
            const std::string_view body = span( w );
            std::uint32_t          lineNo = w.line;
            for( std::size_t at = 0; at < body.size(); )
            {
                const std::size_t nl = std::min( body.find( '\n', at ), body.size() );
                if( std::optional<HandlerRule> rule = ruleAt( w.id, body.substr( at, nl - at ), lineNo ) )
                {
                    m_rules.push_back( std::move( *rule ) );
                }
                at = nl + 1;
                ++lineNo;
            }
        }
    }
    void collectMessagesAndMethods() const
    {
        m_inClass.assign( m_ing.symbols.size(), 0 );
        for( const Symbol& s : m_ing.symbols )
        {
            const bool fn = s.kind == SymKind::Function || s.kind == SymKind::Method;
            if( s.lang == Lang::Python && fn && m_classes.ownerOf( s.id ) != kNoNode )
            {
                m_inClass[ s.id ] = 1;
                m_methodsByName[ std::string_view( s.name ) ].push_back( s.id );
            }
            else if( s.lang == Lang::Python && s.kind == SymKind::Class && !handlerNameOf( s.id ).empty() )
            {
                m_messagesByName[ std::string_view( s.name ) ].push_back( s.id );
            }
        }
    }
    // The innermost Python function definition of `fileId` holding byte `at`, else kNoNode (definition order: an inner function
    // follows the one it is nested in, so the last that holds `at` is the innermost).
    NodeId functionAt( std::uint32_t fileId, std::uint32_t at ) const
    {
        const auto holds = [ & ]( NodeId f ) { return m_ing.symbols[f].sigStartByte <= at && at < m_ing.symbols[f].endByte; };
        const auto hit   = std::ranges::find_if( m_fnsOfFile[ fileId ] | std::views::reverse, holds );
        return hit == ( m_fnsOfFile[ fileId ] | std::views::reverse ).end() ? kNoNode : *hit;
    }
    // Readers of a rule attribute (`.A` not assigned) in Python files outside test paths, read from the text, and every method
    // name their classes define (the posters).
    HashMap<std::string_view, char> collectPosters() const
    {
        m_fnsOfFile.assign( m_ing.files.size(), {} );
        for( const Symbol& s : m_ing.symbols )
        {
            if( s.lang == Lang::Python && ( s.kind == SymKind::Function || s.kind == SymKind::Method ) )
            {
                m_fnsOfFile[ s.fileId ].push_back( s.id );
            }
        }
        std::vector<NodeId> readerClasses;
        for( std::uint32_t f = 0; f < m_ing.files.size(); ++f )
        {
            if( m_fnsOfFile[f].empty() || isTestPath( m_ing.files[f] ) )
            {
                continue;
            }
            const std::string_view t( text( f ) );
            for( const HandlerRule& rule : m_rules )
            {
                const std::string needle = "." + rule.attr;
                for( std::size_t at = t.find( needle ); at != std::string_view::npos; at = t.find( needle, at + 1 ) )
                {
                    const std::size_t end  = at + needle.size();
                    std::size_t       next = end;
                    while( next < t.size() && t[ next ] == ' ' )
                    {
                        ++next;
                    }
                    const bool store = next < t.size() && t[ next ] == '=' && ( next + 1 >= t.size() || t[ next + 1 ] != '=' );
                    if( ( end < t.size() && namesplit::isIdentChar( t[ end ] ) ) || store )
                    {
                        continue;
                    }
                    const NodeId fn = functionAt( f, std::uint32_t( at ) );
                    if( fn == kNoNode || fn == rule.writer || std::ranges::contains( m_readers, fn, &HandlerReader::fn ) )
                    {
                        continue;
                    }
                    m_readers.push_back( HandlerReader{ fn, std::uint32_t( 1 + std::ranges::count( t.substr( 0, at ), '\n' ) ) } );
                    if( const NodeId owner = m_classes.ownerOf( fn ); owner != kNoNode )
                    {
                        readerClasses.push_back( owner );
                    }
                }
            }
        }
        std::ranges::sort( m_readers, {}, &HandlerReader::fn );
        std::ranges::sort( readerClasses );
        HashMap<std::string_view, char> posters;
        for( const auto& [ name, defs ] : m_methodsByName )
        {
            if( std::ranges::any_of( defs, [ & ]( NodeId d ) { return std::ranges::binary_search( readerClasses, m_classes.ownerOf( d ) ); } ) )
            {
                posters[ name ] = 1;
            }
        }
        return posters;
    }
    void build() const
    {
        if( m_built )
        {
            return;
        }
        m_built = true;
        collectRules();
        if( m_rules.empty() )
        {
            return;
        }
        collectMessagesAndMethods();
        const HashMap<std::string_view, char> posters = collectPosters();
        for( std::uint32_t i = 0; i < m_ing.references.size(); ++i )
        {
            const Reference& r = m_ing.references[i];
            if( r.role != RefRole::Call || r.lang != Lang::Python || r.fromSymbol >= m_ing.symbols.size() || !posters.contains( std::string_view( r.calleeName ) ) )
            {
                continue;
            }
            const std::vector<std::string_view> segs = constructionAt( r );
            HandlerPostSite site { i, {}, segs.empty() ? std::vector<NodeId>() : classesFor( r, segs ) };
            for( const NodeId c : site.classes )
            {
                site.names.push_back( handlerNameOf( c ) );
            }
            std::ranges::sort( site.names );
            site.names.erase( std::ranges::unique( site.names ).begin(), site.names.end() );
            if( !site.names.empty() )
            {
                m_siteOfRef[ i ] = std::uint32_t( m_sites.size() );
                m_sites.push_back( std::move( site ) );
            }
        }
    }

    const IngestResult&                                   m_ing;
    ClassFamilies                                         m_classes;
    mutable bool                                          m_built = false;
    mutable std::vector<HandlerRule>                      m_rules;
    mutable std::vector<HandlerPostSite>                  m_sites;      // reference order
    mutable HashMap<std::uint32_t, std::uint32_t>         m_siteOfRef;  // post-site reference -> its m_sites index
    mutable std::vector<char>                             m_inClass;    // per symbol: a Python function definition inside a class
    mutable std::vector<HandlerReader>                    m_readers;
    mutable std::vector<std::vector<NodeId>>              m_fnsOfFile;   // per file: its Python function definitions
    mutable HashMap<std::string_view, std::vector<NodeId>> m_messagesByName;
    mutable HashMap<std::string_view, std::vector<NodeId>> m_methodsByName;
    mutable std::vector<std::string>                      m_text;
    mutable std::vector<char>                             m_textRead;
};

}   // namespace rw
