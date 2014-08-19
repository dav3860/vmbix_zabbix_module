zbx_vmbix: zbx_vmbix.c
	gcc -shared -o zbx_vmbix.so zbx_vmbix.c -I../../../include -fPIC
