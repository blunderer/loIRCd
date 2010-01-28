
#include <loIRCd.h>

static int g_running = 1;
static int main_socket = -1;
static int listen_port = 6667;
static char g_host[256];

static loIRCd_client_t clients[MAX_CLIENT];
static loIRCd_chan_t chans[MAX_CHANS];

void loIRCd_usage(void) {
	printf("loIRCd [-l <listening port>]\n");
	exit(0);
}

int loIRCd_parse_args(int argc, const char ** argv)
{
	if(argc == 3) {
		if(strcmp(argv[1], "-l") == 0) {
			listen_port = atoi(argv[2]);
		} else {
			return 0;
		}
	} else {
		return 0;
	}
	return 1;
}

int loIRCd_write_line(int soc, char * buf)
{
	send(soc, buf, strlen(buf), 0);
	printf("send top %d: %s",soc,buf);
	return 0;
}

int loIRCd_read_line(int soc, char * cmd, char * buf, int buflen)
{
	char c = 0;
	int status = 1;
	int offset = 0;
	int command = 1;
	while((c != '\n')) {
		status = recv(soc, buf+offset, 1, 0);
		if(status > 0) {
			c = buf[offset];
			if(offset < buflen) {
				offset++;
			}
			if((c == ' ')&&(command == 1)) {
				command = 0;
				buf[offset-1] = '\0';
				strncpy(cmd, buf, CMD_LEN);
				offset = 0;
			}
		} else {
			usleep(1000000);
		}
	}
	buf[offset-2] = '\0';
	if(command) {
		strncpy(cmd, buf, CMD_LEN);
	}
	return (status>0)?offset:status;
}

int loIRCd_new_chan(char * name)
{
	int i;
	for(i = 0; i < MAX_CHANS; i++) {
		if(strlen(chans[i].name) == 0) {
			strcpy(chans[i].name, name);
			memset(chans[i].clients, 0, sizeof(int)*MAX_CLIENT);
			break;
		}
	}
	if(i == MAX_CHANS) {
		return -1;
	}
	return i;
}

void loIRCd_talk(char *name, char *buf)
{
	char msg[512];
	char dest[128];
	char *ptr = NULL;
	strcpy(dest, buf);

	ptr = strstr(dest, " ");
	if(ptr) {
		*ptr = 0;
	}

	sprintf(msg, ":%s PRIVMSG %s\n", name, buf);

	if(buf[0] == '#')	{
		int i;
		for(i = 0; i < MAX_CHANS; i++) {
			if(strncmp(chans[i].name, dest, strlen(chans[i].name)) == 0) {
				int j;
				for(j = 0; j < MAX_CHANS; j++) {
					if(chans[i].clients[j]) {
						loIRCd_write_line(clients[j].soc, msg);
					}
				}
				break;
			}
		}
	} else {
		int i;
		for(i = 0; i < MAX_CLIENT; i++) {
			if(strcmp(clients[i].name, dest) == 0) {
				loIRCd_write_line(clients[i].soc, msg);
			}
		}
	}

}

void loIRCd_join(loIRCd_client_t *self, char *name)
{
	int i;
	int chan_exists = -1;
	for(i = 0; i < MAX_CHANS; i++) {
		printf("=====> JOIN chan\n");
		if(strcmp(chans[i].name, name)==0) {
			chan_exists = i;
			break;
		}
	}
	if(chan_exists < 0) {
		printf("=====> CREATE chan\n");
		chan_exists = loIRCd_new_chan(name);
	}
	if(chan_exists >= 0) {
		int j;
		char msg[512];
		char list[1024];

		self->chans[chan_exists] = 1;

		sprintf(msg, ":%s JOIN :%s\n", self->name, name);
		sprintf(list, ":loIRCd 353 %s @ %s :%s", self->name, name, self->name);
		loIRCd_write_line(self->soc, msg);

		for(j = 0; j < MAX_CLIENT; j++) {
			if(chans[chan_exists].clients[j]) {
				strcat(list, " ");
				strcat(list, clients[j].name);
				loIRCd_write_line(clients[j].soc, msg);
			}
		}
		strcat(list, "\n");

		chans[chan_exists].clients[self->id] = 1;
		strcpy(chans[chan_exists].name, name);

		sprintf(msg, ":loIRCd 366 %s %s :End of /NAMES list.\n",self->name, name);
		loIRCd_write_line(self->soc, list);
		loIRCd_write_line(self->soc, msg);
	}
}

void loIRCd_part(loIRCd_client_t *self, char *buf)
{
	int i;
	int chan_exists = -1;
	char name[512];
	char *ptr = NULL;
	strcpy(name, buf);

	ptr = strstr(name, " ");
	if(ptr) {
		*ptr = 0;
	}

	for(i = 0; i < MAX_CHANS; i++) {
		printf("=====> PART chan %s?\n", chans[i].name);
		if(strcmp(chans[i].name, name)==0) {
			chan_exists = i;
			break;
		}
	}
	if(chan_exists >= 0) {
		int j;
		char msg[512];

		sprintf(msg, ":%s PART :%s\n", self->name, buf);
		for(j = 0; j < MAX_CLIENT; j++) {
			if(chans[chan_exists].clients[j]) {
				loIRCd_write_line(clients[j].soc, msg);
			}
		}

		chans[chan_exists].clients[self->id] = 0;
		self->chans[chan_exists] = 0;
	}

}

void * loIRCd_new_client(void * t)
{
	int user = 0, nick = 0;
	char cmd1[CMD_LEN], cmd2[CMD_LEN];
	char param1[256], param2[256];
	int status;

	loIRCd_client_t *self = (loIRCd_client_t*)t;
	pthread_detach(self->service);

	loIRCd_write_line(self->soc, "NOTICE AUTH :*** Looking up your hostname...\n");
	// todo
	loIRCd_write_line(self->soc, "NOTICE AUTH :*** Checking ident\n");
	loIRCd_write_line(self->soc, "NOTICE AUTH :*** Found your hostname\n");

	status = loIRCd_read_line(self->soc, cmd1, param1, 256);
	status = loIRCd_read_line(self->soc, cmd2, param2, 256);

	if(strcmp(cmd1, "USER") == 0) {
		user = 1;
	} else if(strcmp(cmd2, "USER") == 0) {
		user = 1;
	}
	if(strcmp(cmd1, "NICK") == 0) {
		nick = 1;
		strcpy(self->name, param1);
	} else if(strcmp(cmd2, "NICK") == 0) {
		nick = 1;
		strcpy(self->name, param2);
	}

	if(nick && user) {
		char MOTD[1256];
		sprintf(MOTD, ":localhost 001 %s :Welcome on loIRCd\n", self->name);
		loIRCd_write_line(self->soc, MOTD);
		sprintf(MOTD, ":localhost 375 %s :MOTD\n", self->name); 
		loIRCd_write_line(self->soc, MOTD);
		sprintf(MOTD, ":localhost 372 %s :Welcome to loIRCd hosted on %s\n", self->name, g_host); 
		loIRCd_write_line(self->soc, MOTD);
		sprintf(MOTD, ":localhost 376 %s :End of /MOTD command.\n", self->name);
		loIRCd_write_line(self->soc, MOTD);
		sprintf(MOTD, ":loIRC PRIVMSG %s :Welcome to loIRCd hosted by %s\n", self->name, g_host);
		loIRCd_write_line(self->soc, MOTD);
	} else {
		close(self->soc);
		self->soc = -1;
		pthread_exit(NULL);
	}

	while(g_running) {
		char buf[256];
		char cmd[CMD_LEN];
		loIRCd_read_line(self->soc, cmd, buf, 256);

		printf("got %s '%s'\n", cmd,buf);

		if(strcasecmp(cmd, "JOIN") == 0) {
			loIRCd_join(self, buf);
		} else if(strcasecmp(cmd, "PART") == 0) {
			loIRCd_part(self, buf);
		} else if(strcasecmp(cmd, "PRIVMSG") == 0) {
			loIRCd_talk(self->name, buf);
		} else if(strcasecmp(cmd, "QUIT") == 0) {
			loIRCd_talk(self->name, buf);
			break;
		}
	}

	close(self->soc);
	self->soc = -1;
	return NULL;
}

int loIRCd_connect(void)
{
	struct sockaddr_in addr;
	int status = 0;
	int opt_val = 1;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(listen_port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	main_socket = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(main_socket,SOL_SOCKET,SO_REUSEADDR, (const char*)&opt_val, sizeof(int));
	status = bind(main_socket, (const struct sockaddr*)&addr, sizeof(struct sockaddr_in));
	status = listen(main_socket, MAX_CLIENT);
	return 0;
}

int main(int argc, const char ** argv)
{
	int i;

	if(loIRCd_parse_args(argc, argv) == 0) {
		loIRCd_usage();
	}

	gethostname(g_host, 256);

	for(i = 0; i < MAX_CLIENT; i++) {
		clients[i].soc = -1;
		clients[i].id = i;
	}
	for(i = 0; i < MAX_CHANS; i++) {
		chans[i].id = i;
		memset(chans[i].name, 0, 256);
	}

	loIRCd_connect();

	while(g_running) {
		struct sockaddr_in client_addr;
		unsigned int addrlen;
		int client = accept(main_socket, (struct sockaddr*)&client_addr, &addrlen);

		for(i = 0; i < MAX_CLIENT; i++) {
			if(clients[i].soc == -1) {
				clients[i].soc = client;
				break;
			}
		}

		if(i < MAX_CLIENT) {
			pthread_create(&clients[i].service, NULL, loIRCd_new_client, (void*)&clients[i]);
		} else {
			close(client);
		}
	}

	close(main_socket);
	return 0;
}

