all:	clientudp serverudp

clientudp:	client_udp.c
		gcc -o clientudp client_udp.c

serverudp:	server_udp.c
		gcc -o serverudp server_udp.c

clean: 
		rm clientudp serverudp
