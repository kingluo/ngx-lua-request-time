# openresty demo

Use sleep function from libc to do blocking sleep for 1 second, and check the `ngx.ctx.openresty_request_time_us`.

## demo

```bash
mkdir logs

NGX_LUA_REQUEST_TIME_VAR_NAME=openresty_request_time_us \
LD_PRELOAD=/opt/ngx-lua-request-time/ngx_lua_request_time.so \
openresty -p $PWD -c nginx.conf

curl http://localhost:20000/get
```

output:

> 2023/04/16 09:59:43 [warn] 2868937#2868937: *3 [lua] log_by_lua(server.conf:17):2: ngx.ctx.openresty_request_time_us: 1000120 while logging request, client: 127.0.0.1,
server: , request: "GET /get HTTP/1.1", upstream: "https://34.193.132.77:443/get", host: "localhost:20000"


## docker

```bash
docker run --rm --name openresty \
-v $PWD/server.conf:/opt/bitnami/openresty/nginx/conf/server_blocks/my_server_block.conf:ro \
-v $PWD/../../ld.so.preload:/etc/ld.so.preload:ro \
-v $PWD/../../ngx_lua_request_time.so:/opt/ngx-lua-request-time/ngx_lua_request_time.so:ro \
-e NGX_LUA_REQUEST_TIME_VAR_NAME=openresty_request_time_us \
-p 20000:20000 \
bitnami/openresty:latest

curl http://localhost:20000/get
```
