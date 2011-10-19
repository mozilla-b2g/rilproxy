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
#include <telephony/ril.h>
#include <media/AudioSystem.h>

#define AUDIO_REQUEST_FORCE_COMMUNICATION 2000
#define AUDIO_REQUEST_SPEAKER_ON_OFF  2001
#define AUDIO_REQUEST_MIC_MUTE_UNMUTE 2002
#define AUDIO_REQUEST_MODE_NORMAL 2003

using namespace android;

int onAudioRequest(int request) {
  switch(request) {
    case AUDIO_REQUEST_FORCE_COMMUNICATION: 
      printf("AUDIO_REQUEST_FORCE_COMMUNICATION\n");
      AudioSystem::setPhoneState(AudioSystem::MODE_IN_CALL);
      AudioSystem::setForceUse(AudioSystem::FOR_COMMUNICATION, AudioSystem::FORCE_NONE);
      return 0;
    case AUDIO_REQUEST_SPEAKER_ON_OFF: 
      printf("AUDIO_REQUEST_SPEAKER_ON_OFF\n");
      AudioSystem::setPhoneState(AudioSystem::MODE_IN_CALL);
      AudioSystem::setForceUse(AudioSystem::FOR_COMMUNICATION,
          AudioSystem::getForceUse(AudioSystem::FOR_COMMUNICATION) == 
          AudioSystem::FORCE_NONE ? AudioSystem::FORCE_SPEAKER : AudioSystem::FORCE_NONE);
      return 0;
    case AUDIO_REQUEST_MIC_MUTE_UNMUTE: 
      printf("AUDIO_REQUEST_MIC_MUTE_UNMUTE\n");
      bool state;
      AudioSystem::isMicrophoneMuted(&state);
      state ? AudioSystem::setPhoneState(AudioSystem::MODE_IN_CALL) :
        AudioSystem::setPhoneState(AudioSystem::MODE_IN_COMMUNICATION);    
      return 0;
    case AUDIO_REQUEST_MODE_NORMAL: 
      printf("AUDIO_REQUEST_MODE_NORMAL\n");
      AudioSystem::setPhoneState(AudioSystem::MODE_NORMAL);
      return 0;
    default: 
      printf("UNKNOWN REQUEST\n");
      return 1;
  }
}

const char *
requestToString(int request) {
  /*
     cat libs/telephony/ril_commands.h \
     | egrep "^ *{RIL_" \
     | sed -re 's/\{RIL_([^,]+),[^,]+,([^}]+).+/case RIL_\1: return "\1";/'


     cat libs/telephony/ril_unsol_commands.h \
     | egrep "^ *{RIL_" \
     | sed -re 's/\{RIL_([^,]+),([^}]+).+/case RIL_\1: return "\1";/'
   */

switch(request) {
  case RIL_REQUEST_GET_SIM_STATUS: return "GET_SIM_STATUS";
  case RIL_REQUEST_ENTER_SIM_PIN: return "ENTER_SIM_PIN";
  case RIL_REQUEST_ENTER_SIM_PUK: return "ENTER_SIM_PUK";
  case RIL_REQUEST_ENTER_SIM_PIN2: return "ENTER_SIM_PIN2";
  case RIL_REQUEST_ENTER_SIM_PUK2: return "ENTER_SIM_PUK2";
  case RIL_REQUEST_CHANGE_SIM_PIN: return "CHANGE_SIM_PIN";
  case RIL_REQUEST_CHANGE_SIM_PIN2: return "CHANGE_SIM_PIN2";
  case RIL_REQUEST_ENTER_NETWORK_DEPERSONALIZATION: return "ENTER_NETWORK_DEPERSONALIZATION";
  case RIL_REQUEST_GET_CURRENT_CALLS: return "GET_CURRENT_CALLS";
  case RIL_REQUEST_DIAL: return "DIAL";
  case RIL_REQUEST_GET_IMSI: return "GET_IMSI";
  case RIL_REQUEST_HANGUP: return "HANGUP";
  case RIL_REQUEST_HANGUP_WAITING_OR_BACKGROUND: return "HANGUP_WAITING_OR_BACKGROUND";
  case RIL_REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND: return "HANGUP_FOREGROUND_RESUME_BACKGROUND";
  case RIL_REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE: return "SWITCH_WAITING_OR_HOLDING_AND_ACTIVE";
  case RIL_REQUEST_CONFERENCE: return "CONFERENCE";
  case RIL_REQUEST_UDUB: return "UDUB";
  case RIL_REQUEST_LAST_CALL_FAIL_CAUSE: return "LAST_CALL_FAIL_CAUSE";
  case RIL_REQUEST_SIGNAL_STRENGTH: return "SIGNAL_STRENGTH";
  case RIL_REQUEST_REGISTRATION_STATE: return "REGISTRATION_STATE";
  case RIL_REQUEST_GPRS_REGISTRATION_STATE: return "GPRS_REGISTRATION_STATE";
  case RIL_REQUEST_OPERATOR: return "OPERATOR";
  case RIL_REQUEST_RADIO_POWER: return "RADIO_POWER";
  case RIL_REQUEST_DTMF: return "DTMF";
  case RIL_REQUEST_SEND_SMS: return "SEND_SMS";
  case RIL_REQUEST_SEND_SMS_EXPECT_MORE: return "SEND_SMS_EXPECT_MORE";
  case RIL_REQUEST_SETUP_DATA_CALL: return "SETUP_DATA_CALL";
  case RIL_REQUEST_SIM_IO: return "SIM_IO";
  case RIL_REQUEST_SEND_USSD: return "SEND_USSD";
  case RIL_REQUEST_CANCEL_USSD: return "CANCEL_USSD";
  case RIL_REQUEST_GET_CLIR: return "GET_CLIR";
  case RIL_REQUEST_SET_CLIR: return "SET_CLIR";
  case RIL_REQUEST_QUERY_CALL_FORWARD_STATUS: return "QUERY_CALL_FORWARD_STATUS";
  case RIL_REQUEST_SET_CALL_FORWARD: return "SET_CALL_FORWARD";
  case RIL_REQUEST_QUERY_CALL_WAITING: return "QUERY_CALL_WAITING";
  case RIL_REQUEST_SET_CALL_WAITING: return "SET_CALL_WAITING";
  case RIL_REQUEST_SMS_ACKNOWLEDGE: return "SMS_ACKNOWLEDGE";
  case RIL_REQUEST_GET_IMEI: return "GET_IMEI";
  case RIL_REQUEST_GET_IMEISV: return "GET_IMEISV";
  case RIL_REQUEST_ANSWER: return "ANSWER";
  case RIL_REQUEST_DEACTIVATE_DATA_CALL: return "DEACTIVATE_DATA_CALL";
  case RIL_REQUEST_QUERY_FACILITY_LOCK: return "QUERY_FACILITY_LOCK";
  case RIL_REQUEST_SET_FACILITY_LOCK: return "SET_FACILITY_LOCK";
  case RIL_REQUEST_CHANGE_BARRING_PASSWORD: return "CHANGE_BARRING_PASSWORD";
  case RIL_REQUEST_QUERY_NETWORK_SELECTION_MODE: return "QUERY_NETWORK_SELECTION_MODE";
  case RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC: return "SET_NETWORK_SELECTION_AUTOMATIC";
  case RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL: return "SET_NETWORK_SELECTION_MANUAL";
  case RIL_REQUEST_QUERY_AVAILABLE_NETWORKS : return "QUERY_AVAILABLE_NETWORKS ";
  case RIL_REQUEST_DTMF_START: return "DTMF_START";
  case RIL_REQUEST_DTMF_STOP: return "DTMF_STOP";
  case RIL_REQUEST_BASEBAND_VERSION: return "BASEBAND_VERSION";
  case RIL_REQUEST_SEPARATE_CONNECTION: return "SEPARATE_CONNECTION";
  case RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE: return "SET_PREFERRED_NETWORK_TYPE";
  case RIL_REQUEST_GET_PREFERRED_NETWORK_TYPE: return "GET_PREFERRED_NETWORK_TYPE";
  case RIL_REQUEST_GET_NEIGHBORING_CELL_IDS: return "GET_NEIGHBORING_CELL_IDS";
  case RIL_REQUEST_SET_MUTE: return "SET_MUTE";
  case RIL_REQUEST_GET_MUTE: return "GET_MUTE";
  case RIL_REQUEST_QUERY_CLIP: return "QUERY_CLIP";
  case RIL_REQUEST_LAST_DATA_CALL_FAIL_CAUSE: return "LAST_DATA_CALL_FAIL_CAUSE";
  case RIL_REQUEST_DATA_CALL_LIST: return "DATA_CALL_LIST";
  case RIL_REQUEST_RESET_RADIO: return "RESET_RADIO";
  case RIL_REQUEST_OEM_HOOK_RAW: return "OEM_HOOK_RAW";
  case RIL_REQUEST_OEM_HOOK_STRINGS: return "OEM_HOOK_STRINGS";
  case RIL_REQUEST_SET_BAND_MODE: return "SET_BAND_MODE";
  case RIL_REQUEST_QUERY_AVAILABLE_BAND_MODE: return "QUERY_AVAILABLE_BAND_MODE";
  case RIL_REQUEST_STK_GET_PROFILE: return "STK_GET_PROFILE";
  case RIL_REQUEST_STK_SET_PROFILE: return "STK_SET_PROFILE";
  case RIL_REQUEST_STK_SEND_ENVELOPE_COMMAND: return "STK_SEND_ENVELOPE_COMMAND";
  case RIL_REQUEST_STK_SEND_TERMINAL_RESPONSE: return "STK_SEND_TERMINAL_RESPONSE";
  case RIL_REQUEST_STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM: return "STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM";
  case RIL_REQUEST_SCREEN_STATE: return "SCREEN_STATE";
  case RIL_REQUEST_EXPLICIT_CALL_TRANSFER: return "EXPLICIT_CALL_TRANSFER";
  case RIL_REQUEST_SET_LOCATION_UPDATES: return "SET_LOCATION_UPDATES";
  case RIL_REQUEST_CDMA_SET_SUBSCRIPTION:return"CDMA_SET_SUBSCRIPTION";
  case RIL_REQUEST_CDMA_SET_ROAMING_PREFERENCE:return"CDMA_SET_ROAMING_PREFERENCE";
  case RIL_REQUEST_CDMA_QUERY_ROAMING_PREFERENCE:return"CDMA_QUERY_ROAMING_PREFERENCE";
  case RIL_REQUEST_SET_TTY_MODE:return"SET_TTY_MODE";
  case RIL_REQUEST_QUERY_TTY_MODE:return"QUERY_TTY_MODE";
  case RIL_REQUEST_CDMA_SET_PREFERRED_VOICE_PRIVACY_MODE:return"CDMA_SET_PREFERRED_VOICE_PRIVACY_MODE";
  case RIL_REQUEST_CDMA_QUERY_PREFERRED_VOICE_PRIVACY_MODE:return"CDMA_QUERY_PREFERRED_VOICE_PRIVACY_MODE";
  case RIL_REQUEST_CDMA_FLASH:return"CDMA_FLASH";
  case RIL_REQUEST_CDMA_BURST_DTMF:return"CDMA_BURST_DTMF";
  case RIL_REQUEST_CDMA_SEND_SMS:return"CDMA_SEND_SMS";
  case RIL_REQUEST_CDMA_SMS_ACKNOWLEDGE:return"CDMA_SMS_ACKNOWLEDGE";
  case RIL_REQUEST_GSM_GET_BROADCAST_SMS_CONFIG:return"GSM_GET_BROADCAST_SMS_CONFIG";
  case RIL_REQUEST_GSM_SET_BROADCAST_SMS_CONFIG:return"GSM_SET_BROADCAST_SMS_CONFIG";
  case RIL_REQUEST_CDMA_GET_BROADCAST_SMS_CONFIG:return "CDMA_GET_BROADCAST_SMS_CONFIG";
  case RIL_REQUEST_CDMA_SET_BROADCAST_SMS_CONFIG:return "CDMA_SET_BROADCAST_SMS_CONFIG";
  case RIL_REQUEST_CDMA_SMS_BROADCAST_ACTIVATION:return "CDMA_SMS_BROADCAST_ACTIVATION";
  case RIL_REQUEST_CDMA_VALIDATE_AND_WRITE_AKEY: return"CDMA_VALIDATE_AND_WRITE_AKEY";
  case RIL_REQUEST_CDMA_SUBSCRIPTION: return"CDMA_SUBSCRIPTION";
  case RIL_REQUEST_CDMA_WRITE_SMS_TO_RUIM: return "CDMA_WRITE_SMS_TO_RUIM";
  case RIL_REQUEST_CDMA_DELETE_SMS_ON_RUIM: return "CDMA_DELETE_SMS_ON_RUIM";
  case RIL_REQUEST_DEVICE_IDENTITY: return "DEVICE_IDENTITY";
  case RIL_REQUEST_EXIT_EMERGENCY_CALLBACK_MODE: return "EXIT_EMERGENCY_CALLBACK_MODE";
  case RIL_REQUEST_GET_SMSC_ADDRESS: return "GET_SMSC_ADDRESS";
  case RIL_REQUEST_SET_SMSC_ADDRESS: return "SET_SMSC_ADDRESS";
  case RIL_REQUEST_REPORT_SMS_MEMORY_STATUS: return "REPORT_SMS_MEMORY_STATUS";
  case RIL_REQUEST_REPORT_STK_SERVICE_IS_RUNNING: return "REPORT_STK_SERVICE_IS_RUNNING";
  case RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED: return "UNSOL_RESPONSE_RADIO_STATE_CHANGED";
  case RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED: return "UNSOL_RESPONSE_CALL_STATE_CHANGED";
  case RIL_UNSOL_RESPONSE_NETWORK_STATE_CHANGED: return "UNSOL_RESPONSE_NETWORK_STATE_CHANGED";
  case RIL_UNSOL_RESPONSE_NEW_SMS: return "UNSOL_RESPONSE_NEW_SMS";
  case RIL_UNSOL_RESPONSE_NEW_SMS_STATUS_REPORT: return "UNSOL_RESPONSE_NEW_SMS_STATUS_REPORT";
  case RIL_UNSOL_RESPONSE_NEW_SMS_ON_SIM: return "UNSOL_RESPONSE_NEW_SMS_ON_SIM";
  case RIL_UNSOL_ON_USSD: return "UNSOL_ON_USSD";
  case RIL_UNSOL_ON_USSD_REQUEST: return "UNSOL_ON_USSD_REQUEST(obsolete)";
  case RIL_UNSOL_NITZ_TIME_RECEIVED: return "UNSOL_NITZ_TIME_RECEIVED";
  case RIL_UNSOL_SIGNAL_STRENGTH: return "UNSOL_SIGNAL_STRENGTH";
  case RIL_UNSOL_STK_SESSION_END: return "UNSOL_STK_SESSION_END";
  case RIL_UNSOL_STK_PROACTIVE_COMMAND: return "UNSOL_STK_PROACTIVE_COMMAND";
  case RIL_UNSOL_STK_EVENT_NOTIFY: return "UNSOL_STK_EVENT_NOTIFY";
  case RIL_UNSOL_STK_CALL_SETUP: return "UNSOL_STK_CALL_SETUP";
  case RIL_UNSOL_SIM_SMS_STORAGE_FULL: return "UNSOL_SIM_SMS_STORAGE_FUL";
  case RIL_UNSOL_SIM_REFRESH: return "UNSOL_SIM_REFRESH";
  case RIL_UNSOL_DATA_CALL_LIST_CHANGED: return "UNSOL_DATA_CALL_LIST_CHANGED";
  case RIL_UNSOL_CALL_RING: return "UNSOL_CALL_RING";
  case RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED: return "UNSOL_RESPONSE_SIM_STATUS_CHANGED";
  case RIL_UNSOL_RESPONSE_CDMA_NEW_SMS: return "UNSOL_NEW_CDMA_SMS";
  case RIL_UNSOL_RESPONSE_NEW_BROADCAST_SMS: return "UNSOL_NEW_BROADCAST_SMS";
  case RIL_UNSOL_CDMA_RUIM_SMS_STORAGE_FULL: return "UNSOL_CDMA_RUIM_SMS_STORAGE_FULL";
  case RIL_UNSOL_RESTRICTED_STATE_CHANGED: return "UNSOL_RESTRICTED_STATE_CHANGED";
  case RIL_UNSOL_ENTER_EMERGENCY_CALLBACK_MODE: return "UNSOL_ENTER_EMERGENCY_CALLBACK_MODE";
  case RIL_UNSOL_CDMA_CALL_WAITING: return "UNSOL_CDMA_CALL_WAITING";
  case RIL_UNSOL_CDMA_OTA_PROVISION_STATUS: return "UNSOL_CDMA_OTA_PROVISION_STATUS";
  case RIL_UNSOL_CDMA_INFO_REC: return "UNSOL_CDMA_INFO_REC";
  case RIL_UNSOL_OEM_HOOK_RAW: return "UNSOL_OEM_HOOK_RAW";
  case RIL_UNSOL_RINGBACK_TONE: return "UNSOL_RINGBACK_TONE";
  case RIL_UNSOL_RESEND_INCALL_MUTE: return "UNSOL_RESEND_INCALL_MUTE";
  default: return "<unknown request>";
}
}


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
        if(ret > 0) {
          uint32_t *preq = reinterpret_cast<uint32_t*>(data);
          uint32_t req = *(preq+1);
          printf("Message = %d\n", req);
          if(req > RIL_UNSOL_RESEND_INCALL_MUTE) {
            printf("Audio message\n");
            onAudioRequest(req);
          } else {
            printf("RIL Message = %d %s\n", req, requestToString(req));
            blockingWrite(fd, data, ret);
          }
        }
        else if (ret <= 0)
          break;
      }
    }
    close(fd);
    close(net_rw);
  }
  return 0;
}
