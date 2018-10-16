#include <iostream>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "pack.h"
#include "sock.h"

int die(const char* err)
{
	std::cout << "fatal: " << err << ": " << errno << std::endl;
	exit(1);
}

int print_hex(char* buf)
{
	for (int i = 0; i < strlen(buf); i++)
		printf("%02X", buf[i]);
	return 0;
}

int main(int argc, char** argv)
{
	int err;
	FILE* w_file;
	char* holder = new char[5120];
	int cur_blk = 0;

	sock cli_sock("192.168.1.69");

	pack rrq0, get_pack, put_pack;
	rrq0.mk_RRQ(argv[1], "octet");

	//w_file.open("out-client/smalltest.txt", std::ios::out | std::ios::app);
	//send file request
	cli_sock.set_send_pack(rrq0);
	if ((err = cli_sock.put()) < 0)
		die("put()");

	while (1)
	{
		w_file = fopen("out-client/testo.bmp", "ab");
		//listen
		if ((err = cli_sock.get()) < 0)
			die("get()");
		std::cout << "recv_len: " << err << std::endl;
		get_pack = cli_sock.get_recv_pack();
		std::cout << "g:" << get_pack.get_opcode() << std::endl;

		//identify and respond
		switch (get_pack.get_opcode())
		{
			case 0: //invalid operation
			{
				put_pack.mk_ERROR(4, "illegal tftp opcode");
				cli_sock.set_send_pack(put_pack);
				break;
			}
			case 1: //rrq and wrq invalid on client
			case 2:
			{
				put_pack.mk_ERROR(4, "illegal tftp client opcode");
				cli_sock.set_send_pack(put_pack);
				break;
			}
			case 3: //data
			{
				//open file and write
				int dat_len = strlen(get_pack.get_data());
				cur_blk = get_pack.get_blkno();
				if (dat_len > 511) //full data, so more to come (514 bytes WHY)
				{
					//DEAL WITH BIG FILE
					print_hex(get_pack.get_data());
					std::cout << std::endl << strlen(get_pack.get_data()) << std::endl;
					strcat(holder, get_pack.get_data());
					fputs(get_pack.get_data(), w_file);
					//ack
					put_pack.mk_ACK(get_pack.get_blkno());
				}
				else if (dat_len == 0)
				{
					//data done, ack and terminate
					fclose(w_file);
					put_pack.mk_ACK(get_pack.get_blkno());
				}
				else
				{
					//write last bit of data
					print_hex(get_pack.get_data());
					std::cout << std::endl << strlen(get_pack.get_data()) << std::endl;
					fputs(get_pack.get_data(), w_file);
					fclose(w_file);
					put_pack.mk_ACK(get_pack.get_blkno());
				}
				cli_sock.set_send_pack(put_pack);
				break;
			}
			case 4: //ack
			{
				//send next data
				break;
			}
			case 5: //error
			{
				//show error and terminate
				std::cout << get_pack.get_errno() << get_pack.get_errmsg() << std::endl;
				return 1;
				break;
			}
		}
		if ((err = cli_sock.put()) < 0)
			die("put()");
		std::cout << "s:" << put_pack.get_opcode() << std::endl;
		fclose(w_file);
	}
	
	return 0;
}
