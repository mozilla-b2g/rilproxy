/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
#define  RILD_SOCKET_NAME    "rild"
#define  RILB2G_SOCKET_NAME    "rilb2g"

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <pwd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <linux/prctl.h>
#include <utils/Log.h>
#include <cutils/sockets.h>


void switchUser() {
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
      LOGE("RIL Response: unexpected error on write errno:%d", errno);
      close(fd);
      return -1;
    }
  }

  return 0;
}

int main(int argc, char **argv) {

  int rild_rw;
  int rilb2g_rw;
  int ret;
  struct stat r;
  stat("/dev/socket/rild", &r);	
  if(!((r.st_mode & 0x1ff) == (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)))
  {
    printf("The rild socket is not perm 0666. Please reset the permissions before running this utility.\n");
    return 1;
  }

  // connect to the rilb2g socket
  rilb2g_rw = socket_local_server(
    RILB2G_SOCKET_NAME,
    ANDROID_SOCKET_NAMESPACE_RESERVED,
    SOCK_STREAM );   
  if (rilb2g_rw < 0) {
    LOGE("Could not connect to %s socket: %s\n",
         RILB2G_SOCKET_NAME, strerror(errno));
    return 1;
  }
  
  switchUser();
  while(1)
  {
    LOGD("Waiting on socket");
    struct pollfd connect_fds;
    connect_fds.fd = rilb2g_rw;
    connect_fds.events = POLLIN;
    connect_fds.revents = 0;
    poll(&connect_fds, 1, -1);
    LOGD("Socket connected");

    struct passwd *pwd = NULL;
    pwd = getpwuid(getuid());
    if (pwd != NULL) {
      if (strcmp(pwd->pw_name, "radio") == 0) {
        LOGD("Converted to radio account");
      } else {
        LOGE("Cannot convert to radio account");
      }
    } else {
      LOGE("Cannot convert to radio account, getpwuid error.");
    }
    LOGD("Connecting to socket %s\n", RILD_SOCKET_NAME);
    rild_rw = socket_local_client(
      RILD_SOCKET_NAME,
      ANDROID_SOCKET_NAMESPACE_RESERVED,
      SOCK_STREAM );   
    if (rild_rw < 0) {
      LOGE("Could not connect to %s socket: %s\n",
           RILD_SOCKET_NAME, strerror(errno));
      return 1;
    }
    LOGD("Connected to socket %s\n", RILD_SOCKET_NAME);

    char data[1024];

    struct pollfd fds[2];
    fds[0].fd = rilb2g_rw;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    fds[1].fd = rild_rw;
    fds[1].events = POLLIN;
    fds[1].revents = 0;

    while(1)
    {
      fds[0].revents = 0;
      fds[1].revents = 0;
      poll(fds, 2, -1);
      if(fds[0].revents > 0)
      {
        ret = read(rilb2g_rw, data, 1024);
        if(ret > 0)
          blockingWrite(rild_rw, data, ret);
        else if (ret <= 0)
          break;
      }
      if(fds[1].revents > 0)
      {
        ret = read(rild_rw, data, 1024);
        if(ret > 0) {
          blockingWrite(rilb2g_rw, data, ret);
        }
        else if (ret <= 0)
          break;
      }
    }
    close(rild_rw);
    close(rilb2g_rw);
  }
  return 0;
}
