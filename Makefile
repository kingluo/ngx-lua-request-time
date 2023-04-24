ngx_lua_request_time.so: ngx_lua_request_time.c
	gcc -fPIC -shared -o $@ $< -ldl -pthread -Wall -Wextra -Werror -ggdb -O2
