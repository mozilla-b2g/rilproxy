/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
#define  RILD_SOCKET_NAME    "rild"
#define  RILB2G_SOCKET_NAME    "rilb2g"

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <pwd.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <linux/prctl.h>
#define LOG_TAG "RILB2G"
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
writeToSocket(int fd, const void *buffer, size_t len) {
  size_t write_offset = 0;
  const uint8_t *to_write;

  to_write = (const uint8_t *)buffer;

  while (write_offset < len) {
    ssize_t written;
    do {
      written = write (fd, to_write + write_offset,
                       len - write_offset);
    } while (written < 0 && errno == EINTR);

    if (written >= 0) {
      write_offset += written;
    } else {
      LOGE("RIL Response: unexpected error on write errno:%d", errno);
      close(fd);
      return -1;
    }
  }

  return 0;
}

int main(int argc, char **argv) {

  int rild_rw;
  int rilb2g_conn;
  int ret;
  struct stat r;
  stat("/dev/socket/rild", &r);
  if(!((r.st_mode & 0x1ff) == (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)))
  {
    printf("The rild socket is not perm 0666. Please reset the permissions before running this utility.\n");
    return 1;
  }

  // connect to the rilb2g socket
  rilb2g_conn = socket_local_server(
    RILB2G_SOCKET_NAME,
    ANDROID_SOCKET_NAMESPACE_RESERVED,
    SOCK_STREAM );
  if (rilb2g_conn < 0) {
    LOGE("Could not connect to %s socket: %s\n",
         RILB2G_SOCKET_NAME, strerror(errno));
    return 1;
  }

  switchUser();
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


  int connected = 0;

  while(1)
  {
    LOGD("Waiting on socket");
    int rilb2g_rw;
    struct pollfd connect_fds;
    struct sockaddr_un peeraddr;
    socklen_t socklen = sizeof (peeraddr);

    connect_fds.fd = rilb2g_conn;
    connect_fds.events = POLLIN;
    connect_fds.revents = 0;
    poll(&connect_fds, 1, -1);

    rilb2g_rw = accept(rilb2g_conn, (struct sockaddr*)&peeraddr, &socklen);

    if (rilb2g_rw < 0 ) {
      LOGE("Error on accept() errno:%d", errno);
      /* start listening for new connections again */
      continue;
    }
    ret = fcntl(rilb2g_rw, F_SETFL, O_NONBLOCK);

    if (ret < 0) {
      LOGE ("Error setting O_NONBLOCK errno:%d", errno);
    }

    LOGD("Socket connected");
    connected = 1;

    while(1) {
      LOGD("Connecting to socket %s\n", RILD_SOCKET_NAME);
      rild_rw = socket_local_client(
        RILD_SOCKET_NAME,
        ANDROID_SOCKET_NAMESPACE_RESERVED,
        SOCK_STREAM );
      if (rild_rw >= 0) {
        break;
      }
      LOGE("Could not connect to %s socket, retrying: %s\n",
           RILD_SOCKET_NAME, strerror(errno));
      sleep(1);
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

    while(connected)
    {
      poll(fds, 2, -1);
      if(fds[0].revents > 0)
      {
        fds[0].revents = 0;
        while(1)
        {
          ret = read(rilb2g_rw, data, 1024);
          if(ret > 0) {
            writeToSocket(rild_rw, data, ret);
          }
          else if (ret <= 0)
          {
            LOGE("Failed to read from rilb2g socket, closing...");
            connected = 0;
            break;
          }
          if(ret < 1024)
          {
            break;
          }
        }
      }
      if(fds[1].revents > 0)
      {
        fds[1].revents = 0;
        while(1) {
          ret = read(rild_rw, data, 1024);
          if(ret > 0) {
            writeToSocket(rilb2g_rw, data, ret);
          }
          else if (ret <= 0) {
            LOGE("Failed to read from rild socket, closing...");
            connected = 0;
            break;
          }
          if(ret < 1024) {
            break;
          }
        }
      }
    }
    close(rild_rw);
    close(rilb2g_rw);
  }
  return 0;
}
