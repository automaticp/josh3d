#pragma once


namespace josh {


/*
Tag type denoting objects that were culled from rendering.

TODO: Culled is a bad tag, we want an inverse of this, like "visible".
Otherwise, due to how entt storage is handled, iterating all
not-culled is suboptimal.
*/
struct Culled {};


} // namespace josh
