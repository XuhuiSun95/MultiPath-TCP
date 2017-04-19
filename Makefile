all:
	gcc -o mptcp mptcp.c print_pkt.c libmptcp_32.a -lpthread
clean:
	rm -rf mptcp *~