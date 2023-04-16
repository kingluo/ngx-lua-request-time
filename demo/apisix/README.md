# APISIX demo

Use sleep function from libc to do blocking sleep for 1 second, and check the `ngx.ctx.apisix_request_time_us`.

```bash
docker compose up

# in another terminal

curl http://127.0.0.1:9180/apisix/admin/routes/1 \
-H 'X-API-KEY: edd1c9f034335f136f87ad84b625c8f1' -X PUT -d '
{
    "uri": "/*",
    "plugins": {
        "serverless-pre-function": {
            "phase": "access",
            "functions" : ["return function(conf, ctx)
                require(\"apisix.core\").os.usleep(1000000)
            end"]
        },
        "serverless-post-function": {
            "phase": "log",
            "functions" : ["return function(conf, ctx)
                ngx.log(ngx.WARN, \"ngx.ctx.apisix_request_time_us: \",
                    ngx.ctx.apisix_request_time_us)
            end"]
        }
    },
    "upstream": {
        "type": "roundrobin",
        "nodes": {
            "httpbin.org": 1
        }
    }
}'

curl http://127.0.0.1:9080/get
```

output:

> apisix-apisix-1  | 2023/04/16 02:45:33 [warn] 47#47: *572 [lua] [string "return function(conf, ctx)..."]:2: func(): ngx.ctx.apisix_request_time_us: 1001390 while logging request, client: 172.20.0.1, server: _, request: "GET /get HTTP/1.1", upstream: "http://34.235.32.249:80/get", host: "127.0.0.1:9080"

> apisix-apisix-1  | 172.20.0.1 - - [16/Apr/2023:02:45:33 +0000] 127.0.0.1:9080 "GET /get HTTP/1.1" 200 301 1.471 "-" "curl/7.68.0" 34.235.32.249:80 200 0.468 "http://127.0.0.1:9080"
