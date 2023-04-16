ngx_lua_request_time.so: ngx_lua_request_time.cpp
	g++ -fPIC -shared -o $@ $< -ldl -pthread -Wall -Wextra -Werror -pedantic -ggdb -O2
