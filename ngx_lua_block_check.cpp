#include <climits>
#include <cstddef>
#include <cstdint>
#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>

extern "C" {
#define NGX_ERROR -1
#define PHASE_SET 0x0001
#define PHASE_REWRITE 0x0002
#define PHASE_ACCESS 0x0004
#define PHASE_CONTENT 0x0008
#define PHASE_BALANCER 0x0200
#define PHASE_HEADER_FILTER 0x0020
#define PHASE_BODY_FILTER 0x0040
#define LUA_TNUMBER 3
#define LUA_GLOBALSINDEX (-10002)

struct lua_State;
struct ngx_http_request_t;
typedef ptrdiff_t lua_Integer;
typedef int (*lua_resume_t)(lua_State *L, int narg);
typedef int (*lua_pcall_t)(lua_State *L, int nargs, int nresults, int errfunc);
typedef int (*lua_type_t)(lua_State *L, int idx);
typedef void (*lua_settop_t)(lua_State *L, int idx);
typedef void (*lua_setfield_t)(lua_State *L, int idx, const char *k);
typedef void (*lua_getfield_t) (lua_State *L, int index, const char *k);
typedef ngx_http_request_t *(*ngx_http_lua_get_request_t)(lua_State *L);
typedef int (*ngx_http_lua_ffi_get_phase_t)(ngx_http_request_t *r, char **err);
typedef int (*ngx_http_lua_traceback_t)(lua_State *L);
typedef int (*lua_CFunction) (lua_State *L);
typedef lua_CFunction (*lua_tocfunction_t)(lua_State *L, int idx);
typedef lua_Integer (*lua_tointeger_t)(lua_State *L, int idx);
typedef void (*lua_pushinteger_t)(lua_State *L, lua_Integer n);

static lua_resume_t lua_resume_f = NULL;
static lua_pcall_t lua_pcall_f = NULL;
static lua_type_t lua_type_f = NULL;
static lua_settop_t lua_settop_f = NULL;
static lua_setfield_t lua_setfield_f = NULL;
static lua_getfield_t lua_getfield_f = NULL;
static ngx_http_lua_get_request_t ngx_http_lua_get_request_f = NULL;
static ngx_http_lua_ffi_get_phase_t ngx_http_lua_ffi_get_phase_f = NULL;
static ngx_http_lua_traceback_t ngx_http_lua_traceback_f = NULL;
static lua_tocfunction_t lua_tocfunction_f = NULL;
static lua_tointeger_t lua_tointeger_f = NULL;
static lua_pushinteger_t lua_pushinteger_f = NULL;

static const char* VAR_NAME = NULL;

static bool is_enabled() {
    if (!lua_resume_f) {
        lua_resume_f = (lua_resume_t)dlsym(RTLD_NEXT, "lua_resume");
        lua_pcall_f = (lua_pcall_t)dlsym(RTLD_NEXT, "lua_pcall");
        lua_type_f = (lua_type_t)dlsym(RTLD_NEXT, "lua_type");
        lua_settop_f = (lua_settop_t)dlsym(RTLD_NEXT, "lua_settop");
        lua_getfield_f = (lua_getfield_t)dlsym(RTLD_NEXT, "lua_getfield");
        lua_setfield_f = (lua_setfield_t)dlsym(RTLD_NEXT, "lua_setfield");
        ngx_http_lua_get_request_f = (ngx_http_lua_get_request_t)dlsym(RTLD_DEFAULT, "ngx_http_lua_get_request");
        ngx_http_lua_ffi_get_phase_f = (ngx_http_lua_ffi_get_phase_t)dlsym(RTLD_DEFAULT, "ngx_http_lua_ffi_get_phase");
        ngx_http_lua_traceback_f = (ngx_http_lua_traceback_t)dlsym(RTLD_DEFAULT, "ngx_http_lua_traceback");
        lua_tocfunction_f = (lua_tocfunction_t)dlsym(RTLD_NEXT, "lua_tocfunction");
        lua_tointeger_f = (lua_tointeger_t)dlsym(RTLD_NEXT, "lua_tointeger");
        lua_pushinteger_f = (lua_pushinteger_t)dlsym(RTLD_NEXT, "lua_pushinteger");

        char* val = getenv("NGX_LUA_REQUEST_TIME_VAR_NAME");
        if (val != nullptr) {
            VAR_NAME = strdup(val);
        }
    }

    return VAR_NAME != NULL;
}

static void record_time(timeval* tv1, timeval* tv2, lua_State* L) {
    lua_Integer val = (tv2->tv_sec - tv1->tv_sec) * 1000000 + (tv2->tv_usec - tv1->tv_usec);
    if (val <= 0) {
        return;
    }

    lua_getfield_f(L, LUA_GLOBALSINDEX, "ngx");
    lua_getfield_f(L, -1, "ctx");
    lua_getfield_f(L, -1, VAR_NAME);
    if (lua_type_f(L, -1) == LUA_TNUMBER) {
        val += lua_tointeger_f(L, -1);
    }

    lua_pushinteger_f(L, val);
    lua_setfield_f(L, -3, VAR_NAME);
    lua_settop_f(L, -(3)-1);
}

int lua_resume(lua_State *L, int narg) {
    if (!is_enabled()) {
        return lua_resume_f(L, narg);
    }

    ngx_http_request_t *r = ngx_http_lua_get_request_f(L);
    if (!r) {
        return lua_resume_f(L, narg);
    }

    char* err;
    int phase = ngx_http_lua_ffi_get_phase_f(r, &err);
    if (phase == NGX_ERROR ||
		(phase != PHASE_SET &&
			phase != PHASE_REWRITE &&
			phase != PHASE_ACCESS &&
			phase != PHASE_CONTENT)) {
        return lua_resume_f(L, narg);
    }

    timeval tv1;
    gettimeofday(&tv1, NULL);

    int ret = lua_resume_f(L, narg);

    timeval tv2;
    gettimeofday(&tv2, NULL);

    record_time(&tv1, &tv2, L);

    return ret;
}

int lua_pcall(lua_State *L, int nargs, int nresults, int errfunc) {
    if (!is_enabled()) {
        return lua_pcall_f(L, nargs, nresults, errfunc);
    }

    if (lua_tocfunction_f(L, 1) != ngx_http_lua_traceback_f) {
        return lua_pcall_f(L, nargs, nresults, errfunc);
    }

    ngx_http_request_t *r = ngx_http_lua_get_request_f(L);
    if (!r) {
        return lua_pcall_f(L, nargs, nresults, errfunc);
    }

    char* err;
    int phase = ngx_http_lua_ffi_get_phase_f(r, &err);
    if (phase == NGX_ERROR ||
		(phase != PHASE_HEADER_FILTER &&
			phase != PHASE_BODY_FILTER &&
			phase != PHASE_BALANCER)) {
        return lua_pcall_f(L, nargs, nresults, errfunc);
    }

    timeval tv1;
    gettimeofday(&tv1, NULL);

    int ret = lua_pcall_f(L, nargs, nresults, errfunc);

    timeval tv2;
    gettimeofday(&tv2, NULL);

    record_time(&tv1, &tv2, L);

    return ret;
}
}
