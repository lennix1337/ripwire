#pragma once
// forgraphverb.h — FOR-ROUTE-DECLINED: a --for task SPELLED as a graph question (`path from A to B`, `callers of X`,
// `common callers of A and B`) gets the graph verb it names appended after its ordinary answer, run on the
// already-built index (CLI src/verbs_for.h appendSubjectGraphVerb, MCP `for` src/mcpverbs.h forTaskText — one wrapper).
//
// WHY. The ranked --for bundle answers these by subtoken overlap and misses the chain between the two names; the
// verb answers them from the call graph. Measured (ORCH reports/for-route-s0.md, train-39 binary, LARGE-DEV): appending
// the verb's DEFAULT-cap answer flips 7 rows a tier (5 to complete), recovers 13 of 15 missing chain items and loses
// none, at +1.2-4.9 KB per fired row; the parser fires on 0 of 258 non-graph subjects.
//
// WHAT. lexical.h parseGraphSubject reads the endpoints from the task's words (no plain-word fallback but `main`);
// chooseGraphSubjectArgs below tries at most kGraphSubjectAttempts candidate combinations in preference order and
// keeps the first whose every endpoint the verb's own resolver finds (the verb then never refuses). The ordinary
// answer is never replaced: the block rides after it, inside <graph_verb endpoints="subject">, whose legend says the
// endpoints were read from prose (a name match, hedged), and the verb's default caps and next= paging bound it.
// Nothing resolves → nothing is appended (byte-identical to the answer without this lane).
#include "graph.h"       // resolveAllByNameQualified / resolveFocus — the callers / connect resolvers
#include "lexical.h"     // parseGraphSubject — the subject-only half
#include "nextverb.h"    // nextFlag / nextAttrXml — verb= is a pasteable invocation
#include "pathgaps.h"    // resolvePathEndpoint — the --path resolver
#include "serialize.h"   // escapeXml

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace rw
{
inline constexpr std::size_t kGraphSubjectAttempts = 4;   // candidate combinations tried (Stage 0: <= 4)
inline constexpr std::size_t kGraphSubjectSlotCands = 3;  // per-slot candidates in a two-endpoint form (3 x 3, best 4)

struct GraphSubjectArgs
{
    GraphSubjectForm         form = GraphSubjectForm::Path;
    std::vector<std::string> endpoints;   // the resolved spellings, slot order
    std::vector<std::string> spans;       // the words each slot was read from
    std::size_t              tried = 0;   // combinations tried before this one resolved (1 = the first)
};

// The first candidate combination whose every endpoint the verb's own resolver finds; nullopt when the task is not
// a graph question or no combination resolves. Two-endpoint forms rank (a,b) by index sum, then a's index (parse.py).
inline std::optional<GraphSubjectArgs> chooseGraphSubjectArgs( const IngestResult& ing, std::string_view task )
{
    const std::optional<GraphSubject> gs = parseGraphSubject( task );
    if( !gs )
    {
        return std::nullopt;
    }
    const auto resolves = [ & ]( const std::string& spec ) -> bool
    {
        std::size_t unproven = 0;
        bool        glob     = false;
        switch( gs->form )
        {
            case GraphSubjectForm::Path: return !resolvePathEndpoint( ing, spec, &unproven, &glob ).empty();
            case GraphSubjectForm::Callers: return !resolveAllByNameQualified( ing, spec, &unproven ).empty();
            case GraphSubjectForm::Common: return resolveFocus( ing, spec, &unproven ) != kNoNode;
        }
        return false;
    };
    std::vector<std::vector<std::string>> combos;
    if( gs->slots.size() == 1 )
    {
        for( std::size_t a = 0; a < gs->slots[0].size() && combos.size() < kGraphSubjectAttempts; ++a )
        {
            combos.push_back( { gs->slots[0][a] } );
        }
    }
    else
    {
        const std::size_t na = std::min( gs->slots[0].size(), kGraphSubjectSlotCands );
        const std::size_t nb = std::min( gs->slots[1].size(), kGraphSubjectSlotCands );
        for( std::size_t sum = 0; sum + 2 <= na + nb && combos.size() < kGraphSubjectAttempts; ++sum )
        {
            for( std::size_t a = 0; a < na && a <= sum && combos.size() < kGraphSubjectAttempts; ++a )
            {
                if( sum - a < nb )
                {
                    combos.push_back( { gs->slots[0][a], gs->slots[1][sum - a] } );
                }
            }
        }
    }
    std::vector<std::pair<std::string, bool>> memo;   // a spelling is resolved once however many combinations hold it
    const auto found = [ & ]( const std::string& spec ) -> bool
    {
        for( const auto& [ s, ok ] : memo )
        {
            if( s == spec ) { return ok; }
        }
        memo.emplace_back( spec, resolves( spec ) );
        return memo.back().second;
    };
    for( std::size_t k = 0; k < combos.size(); ++k )
    {
        bool all = true;
        for( const std::string& e : combos[k] )
        {
            all = all && found( e );
        }
        if( all )
        {
            return GraphSubjectArgs{ gs->form, combos[k], gs->spans, k + 1 };
        }
    }
    return std::nullopt;
}

// the verb's flag and its value, as the CLI spells them
inline std::pair<std::string_view, std::string> graphSubjectVerbFlag( const GraphSubjectArgs& a )
{
    std::string v = a.endpoints.empty() ? std::string() : a.endpoints[0];
    for( std::size_t k = 1; k < a.endpoints.size(); ++k )
    {
        v += "," + a.endpoints[k];
    }
    switch( a.form )
    {
        case GraphSubjectForm::Path: return { "--path=", v };
        case GraphSubjectForm::Callers: return { "--callers=", v };
        case GraphSubjectForm::Common: return { "--connect=", v };
    }
    return { "--path=", v };
}

// The opening of the block: its own legend (present only with the block; no `--` inside the comment), then
// <graph_verb endpoints="subject" verb=… from_span=/to_span= | of_span= [| and_span=] [tried=]>. tried= is present
// only when an earlier combination did not resolve.
inline std::string graphSubjectOpenXml( const GraphSubjectArgs& a )
{
    std::string out = "<!-- ripwire for graph_verb: the task is spelled as a graph question, so the verb it names ran too, on the built "
                      "index with its default caps; endpoints=\"subject\": its endpoints were READ FROM THE TASK WORDS (the *_span= "
                      "attributes, cut at the first for/with/via/in/on/when/using/during/at/near), matched by name, not chosen "
                      "from the graph: check they are the symbols meant; verb= is the same call to paste";
    out += a.tried > 1 ? "; tried= combinations of the words were tried, the first that resolves ran" : "";
    out += "; the verb's own legend follows; the answer above is unchanged, and these bytes are not in est_tokens --><graph_verb endpoints=\"subject\"";
    const auto [ flag, value ] = graphSubjectVerbFlag( a );
    out += nextAttrXml( nextFlag( flag, value ), "verb" );
    static constexpr std::string_view kPathSpans[]   = { "from_span", "to_span" };
    static constexpr std::string_view kCommonSpans[] = { "of_span", "and_span" };
    std::vector<char> esc;
    for( std::size_t k = 0; k < a.spans.size() && k < 2; ++k )
    {
        const std::string_view name = a.form == GraphSubjectForm::Path ? kPathSpans[k] : a.form == GraphSubjectForm::Common ? kCommonSpans[k] : "of_span";
        out += " " + std::string( name ) + "=\"" + std::string( escapeXml( a.spans[k], esc ) ) + "\"";
    }
    out += a.tried > 1 ? " tried=\"" + std::to_string( a.tried ) + "\"" : "";
    out += ">";
    return out;
}

inline constexpr std::string_view kGraphSubjectCloseXml = "</graph_verb>";
}   // namespace rw
