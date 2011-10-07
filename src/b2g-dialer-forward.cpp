#define HAVE_SYS_SOCKET_H 1
#define HAVE_SYS_UIO_H

#define  RILD_SOCKET_NAME    "rild"

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <pwd.h>
#include <sys/stat.h>
#include <linux/prctl.h>
#include <cutils/sockets.h>

/*
 * switchUser - Switches UID to radio, preserving CAP_NET_ADMIN capabilities.
 * Our group, cache, was set by init.
 */
void switchUser()
{
    prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0);
    setuid(1001);

    struct __user_cap_header_struct header;
    struct __user_cap_data_struct cap;
    header.version = _LINUX_CAPABILITY_VERSION;
    header.pid = 0;
    cap.effective = cap.permitted = 1 << CAP_NET_ADMIN;
    cap.inheritable = 0;
    capset(&header, &cap);
}

static int
blockingWrite(int fd, const void *buffer, size_t len) {
    size_t writeOffset = 0;
    const uint8_t *toWrite;

    toWrite = (const uint8_t *)buffer;

    while (writeOffset < len) {
        ssize_t written;
        do {
            written = write (fd, toWrite + writeOffset,
							 len - writeOffset);
        } while (written < 0 && errno == EINTR);

        if (written >= 0) {
            writeOffset += written;
        } else {   // written < 0
            printf ("RIL Response: unexpected error on write errno:%d", errno);
            close(fd);
            return -1;
        }
    }

    return 0;
}

int main(int argc, char **argv) {

	int fd;
	int net_connect;
	int net_rw;
	int ret;
	struct stat r;
	stat("/dev/socket/rild", &r);	
	if(!((r.st_mode & 0x1ff) == (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)))
	{
		printf("The rild socket is not perm 0666. Please reset the permissions before running this utility.\n");
		return 1;
	}
	
	net_connect = socket_inaddr_any_server(
		5555,
		SOCK_STREAM );
	if(net_connect < 0)
	{
		printf("Cannot create network socket!\n");
		return 1;
	}
	switchUser();
	while(1)
	{
		printf("Waiting on socket\n");
		struct pollfd connect_fds;
		connect_fds.fd = net_connect;
		connect_fds.events = POLLIN;
		connect_fds.revents = 0;
		poll(&connect_fds, 1, -1);
		printf("Socket connected!\n");
		net_rw = accept(net_connect, NULL, NULL);

		struct passwd *pwd = NULL;
		pwd = getpwuid(getuid());
		if (pwd != NULL) {
			if (strcmp(pwd->pw_name, "radio") == 0) {
				printf("Radio.\n");
			} else {
				printf("No radio.\n");
			}
		} else {
			fprintf(stderr, "getpwuid error.");
		}
		printf("Connecting to socket %s\n", RILD_SOCKET_NAME);
		fd = socket_local_client(
			RILD_SOCKET_NAME,
			ANDROID_SOCKET_NAMESPACE_RESERVED,
			SOCK_STREAM );   
		if (fd < 0) {
			printf("could not connect to %s socket: %s\n",
				   RILD_SOCKET_NAME, strerror(errno));
			return 1;
		}
		printf("connected to socket %s\n", RILD_SOCKET_NAME);
	
		char data[1024];
	
		struct pollfd fds[2];
		fds[0].fd = fd;
		fds[0].events = POLLIN;
		fds[0].revents = 0;
		fds[1].fd = net_rw;
		fds[1].events = POLLIN;
		fds[1].revents = 0;

		while(1)
		{
			fds[0].revents = 0;
			fds[1].revents = 0;
			poll(fds, 2, -1);
			if(fds[0].revents > 0)
			{
				ret = read(fd, data, 1024);
				printf("Read %d from radio\n", ret);
				if(ret > 0)
					blockingWrite(net_rw, data, ret);
				else if (ret <= 0)
					break;
			}
			if(fds[1].revents > 0)
			{
				ret = read(net_rw, data, 1024);
				printf("Read %d from network\n", ret);
				if(ret > 0)
					blockingWrite(fd, data, ret);
				else if (ret <= 0)
					break;
			}
		}
		close(fd);
		close(net_rw);
	}
	return 0;
}
