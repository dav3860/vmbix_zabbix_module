vmbix-3.0: vmbix-3.0.c
	gcc -shared -o vmbix.so vmbix-3.0.c -I../../../include -fPIC
vmbix-3.2: vmbix-3.2.c
	gcc -shared -o vmbix.so vmbix-3.2.c -I../../../include -fPIC
vmbix-3.4: vmbix-3.4.c
        gcc -shared -o vmbix.so vmbix-3.2.c -I../../../include -fPIC
