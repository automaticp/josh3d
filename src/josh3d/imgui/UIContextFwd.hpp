#pragma once


namespace josh {

struct ImGuiApplicationAssembly;

/*
It is convenient to let all UI components access all other
runtime and UI state, without additional encapsulation.
This significantly helps reduce implementation friction.

Since ApplicationAssembly is *the UI* and contains all UI widgets
and a reference to Runtime, we can use it as our UI context.

To avoid circular includes, the widgets can accept a reference
to this forward declaration, only incuding the assembly in .cpp files.
*/
using UIContext = ImGuiApplicationAssembly;

} // namespace josh
