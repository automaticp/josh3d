#pragma once


namespace josh {


/*
Tag type denoting objects that are "current" in a context where only 1 object
can be current at a time. Current camera, current skybox, current directional light, etc.
*/
struct Current {};


} // namespace josh
