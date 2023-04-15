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
#define ACCESS 0x0004
#define HEADER_FILTER 0x0020
#define BODY_FILTER 0x0040
#define LUA_TSTRING 4
#define LUA_GLOBALSINDEX (-10002)
struct lua_State;
struct ngx_http_request_t;
struct ngx_chain_t;
typedef intptr_t ngx_int_t;

typedef int (*lua_resume_t)(lua_State *L, int narg);
typedef int (*lua_pcall_t)(lua_State *L, int nargs, int nresults, int errfunc);
typedef int (*lua_type_t)(lua_State *L, int idx);
typedef const char * (*lua_tostring_t) (lua_State *L, int index, size_t* len);
typedef void (*lua_settop_t)(lua_State *L, int idx);
typedef void (*lua_setfield_t)(lua_State *L, int idx, const char *k);
typedef void (*lua_getfield_t) (lua_State *L, int index, const char *k);
typedef const char *(*lua_tolstring_t)(lua_State *L, int idx, size_t *len);
typedef void (*lua_pushstring_t)(lua_State *L, const char *str);
typedef ngx_http_request_t *(*ngx_http_lua_get_request_t)(lua_State *L);
typedef int (*ngx_http_lua_ffi_get_phase_t)(ngx_http_request_t *r, char **err);
typedef int (*ngx_http_lua_traceback_t)(lua_State *L);
typedef int (*lua_CFunction) (lua_State *L);
typedef lua_CFunction (*lua_tocfunction_t)(lua_State *L, int idx);

static lua_resume_t lua_resume_f = NULL;
static lua_pcall_t lua_pcall_f = NULL;
static lua_type_t lua_type_f = NULL;
static lua_tostring_t lua_tostring_f = NULL;
static lua_settop_t lua_settop_f = NULL;
static lua_setfield_t lua_setfield_f = NULL;
static lua_getfield_t lua_getfield_f = NULL;
static lua_tolstring_t lua_tolstring_f = NULL;
static lua_pushstring_t lua_pushstring_f = NULL;
static ngx_http_lua_get_request_t ngx_http_lua_get_request_f = NULL;
static ngx_http_lua_ffi_get_phase_t ngx_http_lua_ffi_get_phase_f = NULL;
static ngx_http_lua_traceback_t ngx_http_lua_traceback_f = NULL;
static lua_tocfunction_t lua_tocfunction_f = NULL;

static const char* VAR_NAME = NULL;

static bool is_enabled() {
    if (!lua_resume_f) {
        lua_resume_f = (lua_resume_t)dlsym(RTLD_NEXT, "lua_resume");
        lua_pcall_f = (lua_pcall_t)dlsym(RTLD_NEXT, "lua_pcall");
        lua_type_f = (lua_type_t)dlsym(RTLD_NEXT, "lua_type");
        lua_tostring_f = (lua_tostring_t)dlsym(RTLD_NEXT, "lua_tolstring");
        lua_settop_f = (lua_settop_t)dlsym(RTLD_NEXT, "lua_settop");
        lua_getfield_f = (lua_getfield_t)dlsym(RTLD_NEXT, "lua_getfield");
        lua_setfield_f = (lua_setfield_t)dlsym(RTLD_NEXT, "lua_setfield");
        lua_tolstring_f = (lua_tolstring_t)dlsym(RTLD_NEXT, "lua_tolstring");
        lua_pushstring_f = (lua_pushstring_t)dlsym(RTLD_NEXT, "lua_pushstring");
        ngx_http_lua_get_request_f = (ngx_http_lua_get_request_t)dlsym(RTLD_DEFAULT, "ngx_http_lua_get_request");
        ngx_http_lua_ffi_get_phase_f = (ngx_http_lua_ffi_get_phase_t)dlsym(RTLD_DEFAULT, "ngx_http_lua_ffi_get_phase");
        ngx_http_lua_traceback_f = (ngx_http_lua_traceback_t)dlsym(RTLD_DEFAULT, "ngx_http_lua_traceback");
        lua_tocfunction_f = (lua_tocfunction_t)dlsym(RTLD_NEXT, "lua_tocfunction");

        char* val = getenv("NGX_LUA_REQUEST_TIME_VAR_NAME");
        if (val != nullptr) {
            VAR_NAME = strdup(val);
        }
    }

    return VAR_NAME != NULL;
}

#define RESET_STACK() lua_settop_f(L, -(3)-1);

static void record_time(timeval* tv1, timeval* tv2, lua_State* L) {
    unsigned long long elapsed_us = (tv2->tv_sec - tv1->tv_sec) * 1000000 + (tv2->tv_usec - tv1->tv_usec);
    if (elapsed_us <= 0) {
        return;
    }

    lua_getfield_f(L, LUA_GLOBALSINDEX, "ngx");
    lua_getfield_f(L, -1, "var");
    lua_getfield_f(L, -1, VAR_NAME);
    if (lua_type_f(L, -1) != LUA_TSTRING) {
        RESET_STACK();
        return;
    }

    const char* str = lua_tolstring_f(L, -1, NULL);
    char* endptr;
    errno = 0;
    long long int val = strtoll(str, &endptr, 10);

    if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN))
            || (errno != 0 && val == 0)) {
        RESET_STACK();
        //perror("strtol");
        return;
    }

    if (endptr == str) {
        RESET_STACK();
        //fprintf(stderr, "No digits were found\n");
        return;
    }

    val += elapsed_us;
    char buf[32];
    sprintf(buf, "%lld", val);
    lua_pushstring_f(L, buf);
    lua_setfield_f(L, -3, VAR_NAME);
    RESET_STACK();
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
    if (phase == NGX_ERROR || phase != ACCESS) {
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
    if (phase == NGX_ERROR || (phase != HEADER_FILTER && phase != BODY_FILTER)) {
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
