#pragma once
#include "Common.hpp"
#include "Filesystem.hpp"
#include "UIContextFwd.hpp"


namespace josh {

/*
NOTE: Currently the vfs is exposed through the global vfs()
and is not available in the UIContext.
*/
struct ImGuiVFSControl
{
    void display(UIContext& ui);

    String _new_root;
    String _test_vpath;
    Path   _last_resolved_entry;
    String _exception_str;
};


} // namespace josh
