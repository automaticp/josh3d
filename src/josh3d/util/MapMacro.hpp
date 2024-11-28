#pragma once


/*
Taken from: https://stackoverflow.com/questions/6707148/foreach-macro-on-macros-arguments
*/


#define _JOSH3D_EVAL0(...) __VA_ARGS__
#define _JOSH3D_EVAL1(...) _JOSH3D_EVAL0(_JOSH3D_EVAL0(_JOSH3D_EVAL0(__VA_ARGS__)))
#define _JOSH3D_EVAL2(...) _JOSH3D_EVAL1(_JOSH3D_EVAL1(_JOSH3D_EVAL1(__VA_ARGS__)))
#define _JOSH3D_EVAL3(...) _JOSH3D_EVAL2(_JOSH3D_EVAL2(_JOSH3D_EVAL2(__VA_ARGS__)))
#define _JOSH3D_EVAL4(...) _JOSH3D_EVAL3(_JOSH3D_EVAL3(_JOSH3D_EVAL3(__VA_ARGS__)))
#define _JOSH3D_EVAL(...)  _JOSH3D_EVAL4(_JOSH3D_EVAL4(_JOSH3D_EVAL4(__VA_ARGS__)))

#define _JOSH3D_MAP_END(...)
#define _JOSH3D_MAP_OUT
#define _JOSH3D_MAP_COMMA ,

#define _JOSH3D_MAP_GET_END2() 0, _JOSH3D_MAP_END
#define _JOSH3D_MAP_GET_END1(...) _JOSH3D_MAP_GET_END2
#define _JOSH3D_MAP_GET_END(...) _JOSH3D_MAP_GET_END1
#define _JOSH3D_MAP_NEXT0(test, next, ...) next _JOSH3D_MAP_OUT
#define _JOSH3D_MAP_NEXT1(test, next) _JOSH3D_MAP_NEXT0(test, next, 0)
#define _JOSH3D_MAP_NEXT(test, next)  _JOSH3D_MAP_NEXT1(_JOSH3D_MAP_GET_END test, next)

#define _JOSH3D_MAP0(f, x, peek, ...) f(x) _JOSH3D_MAP_NEXT(peek, _JOSH3D_MAP1)(f, peek, __VA_ARGS__)
#define _JOSH3D_MAP1(f, x, peek, ...) f(x) _JOSH3D_MAP_NEXT(peek, _JOSH3D_MAP0)(f, peek, __VA_ARGS__)

#define _JOSH3D_MAP_LIST_NEXT1(test, next) _JOSH3D_MAP_NEXT0(test, _JOSH3D_MAP_COMMA next, 0)
#define _JOSH3D_MAP_LIST_NEXT(test, next)  _JOSH3D_MAP_LIST_NEXT1(_JOSH3D_MAP_GET_END test, next)

#define _JOSH3D_MAP_LIST0(f, x, peek, ...) f(x) _JOSH3D_MAP_LIST_NEXT(peek, _JOSH3D_MAP_LIST1)(f, peek, __VA_ARGS__)
#define _JOSH3D_MAP_LIST1(f, x, peek, ...) f(x) _JOSH3D_MAP_LIST_NEXT(peek, _JOSH3D_MAP_LIST0)(f, peek, __VA_ARGS__)


// Applies the function macro `f` to each of the remaining parameters.
#define JOSH3D_MAP(f, ...) _JOSH3D_EVAL(_JOSH3D_MAP1(f, __VA_ARGS__, ()()(), ()()(), ()()(), 0))


// Applies the function macro `f` to each of the remaining parameters and
// inserts commas between the results.
#define JOSH3D_MAP_LIST(f, ...) _JOSH3D_EVAL(_JOSH3D_MAP_LIST1(f, __VA_ARGS__, ()()(), ()()(), ()()(), 0))


