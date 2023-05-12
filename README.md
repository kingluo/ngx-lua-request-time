# ngx-lua-request-time

It records the lua execution time per request in `ngx.ctx`, in microsecond.

Check this blog for details:

http://luajit.io/posts/openresty-lua-request-time/

The request time is the sum of the following metrics:

* `lua_resume`
  * phases
    * REWRITE
    * ACCESS
    * CONTENT
    * BALANCER
* `lua_pcall`
  * filters
    * header_filter
    * body_filter

**It's non-intrusive and zero-cost.**

Usage:

```bash
NGX_LUA_REQUEST_TIME_VAR_NAME=openresty_request_time_us \
LD_PRELOAD=/opt/ngx-lua-request-time/ngx_lua_request_time.so \
openresty -p $PWD -c nginx.conf
```

* `NGX_LUA_REQUEST_TIME_VAR_NAME`: variable name in `ngx.ctx.<var>`
* `LD_PRELOAD`: preload the `ngx_lua_request_time.so`

## build

```bash
make
# output ngx_lua_request_time.so
```

## demo

*Do not use the Alpine docker image, where the musl libc seems not LD_PRELOAD compatible.*

* [openresty](https://github.com/kingluo/ngx-lua-request-time/tree/main/demo/openresty)
* [apisix](https://github.com/kingluo/ngx-lua-request-time/tree/main/demo/apisix)
