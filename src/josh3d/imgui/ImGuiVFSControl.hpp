#pragma once
#include "Common.hpp"
#include "Filesystem.hpp"


namespace josh {

class VirtualFilesystem;

struct ImGuiVFSControl
{
    VirtualFilesystem& vfs_;

    void display();

    String _new_root;
    String _test_vpath;
    Path   _last_resolved_entry;
    String _exception_str;

    void _roots_listbox_widget();
    void _add_new_root_widget();
    void _debug_resolve_widget();
};


} // namespace josh
