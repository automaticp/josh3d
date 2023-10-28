#pragma once
#include "Filesystem.hpp"
#include <string>


namespace josh {


class VirtualFilesystem;


class ImGuiVFSControl {
private:
    VirtualFilesystem& vfs_;

    std::string new_root_;

    std::string test_vpath_;
    Path last_resolved_entry_;

    std::string exception_str_;

public:
    ImGuiVFSControl(VirtualFilesystem& vfs) : vfs_{ vfs } {}

    void display();

private:
    void roots_listbox_widget();
    void add_new_root_widget();
    void debug_resolve_widget();

};


} // namespace josh
