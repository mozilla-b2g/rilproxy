/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp: */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Code.
 *
 * The Initial Developer of the Original Code is
 *   The Mozilla Foundation
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Kyle Machulis <kyle@nonpolynomial.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#define RILD_SOCKET_NAME       "rild"
#define RILPROXY_SOCKET_NAME   "rilproxy"

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
#define LOG_TAG "RILPROXY"
#include <utils/Log.h>
#include <cutils/sockets.h>

#define LISTEN_BACKLOG 4

void setCapability() {
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

const char*
getSocketName(const char* defval, unsigned int client_id) {
  static char buf[128];
  static int offset = 0;

  if (client_id > 0) {
    const char *p = buf + offset;
    offset += snprintf(buf + offset, sizeof(buf) - offset, "%s%u",
                       defval, client_id);
    offset++;
    return p;
  }

  return defval;
}

int main(int argc, char **argv) {

  int rild_rw;
  int rilproxy_conn;
  int ret;
  unsigned int client_id = 0;
  const char* rild_socket = NULL;
  const char* rilproxy_socket = NULL;

  while ((ret = getopt(argc, argv, "c:s:")) != -1) {
    switch (ret) {
      case 'c': {
        if (sscanf(optarg, "%u", &client_id) != 1) {
          LOGE("invalid client id specified: %s", optarg);
          return -1;
        }
        break;
      }
      case 's': {
        rild_socket = optarg;
        break;
      }
      default:
        return -1;
    }
  }

  rilproxy_socket = getSocketName(RILPROXY_SOCKET_NAME, client_id);
  if (!rild_socket) {
    rild_socket = getSocketName(RILD_SOCKET_NAME, client_id);
  }

  // get control rilproxy socket
  rilproxy_conn = android_get_control_socket(rilproxy_socket);
  if (rilproxy_conn < 0) {
    LOGE("Could not connect to %s socket: %s\n",
         rilproxy_socket, strerror(errno));
    return 1;
  }

  ret = listen(rilproxy_conn, LISTEN_BACKLOG);
  if (ret < 0) {
    LOGE("Failed to listen on control socket '%d': %s",
         rilproxy_conn, strerror(errno));
    return 1;
  }

  setCapability();
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
    int rilproxy_rw;
    struct pollfd connect_fds;
    struct sockaddr_un peeraddr;
    socklen_t socklen = sizeof (peeraddr);

    connect_fds.fd = rilproxy_conn;
    connect_fds.events = POLLIN;
    connect_fds.revents = 0;
    poll(&connect_fds, 1, -1);

    rilproxy_rw = accept(rilproxy_conn, (struct sockaddr*)&peeraddr, &socklen);

    if (rilproxy_rw < 0 ) {
      LOGE("Error on accept() errno:%d", errno);
      /* start listening for new connections again */
      continue;
    }
    ret = fcntl(rilproxy_rw, F_SETFL, O_NONBLOCK);

    if (ret < 0) {
      LOGE ("Error setting O_NONBLOCK errno:%d", errno);
    }

    LOGD("Socket connected");
    connected = 1;

    while(1) {
      LOGD("Connecting to socket %s\n", rild_socket);
      rild_rw = socket_local_client(
        rild_socket,
        ANDROID_SOCKET_NAMESPACE_RESERVED,
        SOCK_STREAM );
      if (rild_rw >= 0) {
        break;
      }
      LOGE("Could not connect to %s socket, retrying: %s\n",
           rild_socket, strerror(errno));

      connect_fds.fd = rilproxy_conn;
      connect_fds.events = POLLIN;
      connect_fds.revents = 0;
      if (poll(&connect_fds, 1, 1000) > 0) {
        goto reconnect_rilproxy;
      }
    }
    LOGD("Connected to socket %s\n", rild_socket);
    char data[1024];

    struct pollfd fds[2];
    fds[0].fd = rilproxy_rw;
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
          ret = read(rilproxy_rw, data, 1024);
          if(ret > 0) {
            writeToSocket(rild_rw, data, ret);
          }
          else if (ret <= 0)
          {
            LOGE("Failed to read from rilproxy socket, closing...");
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
            writeToSocket(rilproxy_rw, data, ret);
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
  reconnect_rilproxy:
    close(rilproxy_rw);
  }
  return 0;
}
