ngx_lua_block_check.so: ngx_lua_block_check.cpp
	g++ -fPIC -shared -o $@ $< -ldl -pthread -Wall -Wextra -Werror -pedantic -ggdb -O2
