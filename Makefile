vmbix-3.4: vmbix-3.4.c
	gcc -shared -o vmbix.so vmbix-3.4.c -I../../../include -fPIC
vmbix-4.0: vmbix-4.0.c
	gcc -shared -o vmbix.so vmbix-4.0.c -I../../../include -fPIC
vmbix-4.2: vmbix-4.2.c
	gcc -shared -o vmbix.so vmbix-4.2.c -I../../../include -fPIC
