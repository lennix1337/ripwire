### Added — C++ pointers to member functions and C class tables with macro-cast receivers are joined to the calls through their fields (`--callers`, `--callees`, `--for`)

A C++ pointer to a member function stored in a struct field (`{ "browse", …, &NinjaMain::ToolBrowse }` in a table of a struct
this file defines, a field assignment, an argument) is now a value reference to that class's method — its definition with a
body, else its declaration, never a same-named method of another class — and a call through a member pointer held in a field,
`( ninja.*options.tool->func )( … )`, is a call through that field (typed by the root's declared struct, as `x->f( … )` is). In
C, a designated initializer nested in an untyped braced member (`.super = { .writeField = f }` in a `ProcessClass` literal)
keys the field path from the enclosing literal's type (`ProcessClass.super` is a `RowClass`), and a call whose receiver is a
function-like macro whose whole body is one pointer cast (`#define As_Row( x ) ( ( const RowClass* )( … ) )`,
`As_Row( this )->writeField( … )`) is typed by the cast's type; any other call receiver (`f( x )->g( … )`, a macro of another
shape) joins by the field name only (`via="slot-name"`, `~` on `--for`). Not joined (floors): `&a::Twin::Go` when the repo
defines two classes of that name (a symbol carries no namespace), a namespace function `&ns::f`, a member pointer in a plain
variable, `->*` and an inline member-pointer field declarator `R ( C::*f )( … )` (both parse as an ERROR node: use a typedef, as
ninja's `ToolFunc` does), table entries inside an `#ifdef` within the initializer list (an ERROR node), a virtual method's
overrides (the pointer names the base class's method only), and `std::function` members (a call result or lambda is no value).
Nothing here is a proven call: count= and reaches= never move.
