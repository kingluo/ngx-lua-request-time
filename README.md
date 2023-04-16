# ngx-lua-request-time
openresty hook: record the lua execution time per request in ngx.ctx

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
