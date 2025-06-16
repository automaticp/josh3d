#pragma once


namespace josh {


/*
Tag type denoting objects that were not culled and are visible to the active camera.

This should not be used for user-controlled visibility. This is reset on every cull pass.
*/
struct Visible {};


} // namespace josh
