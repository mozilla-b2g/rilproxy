#include <utils/Log.h>
#include <media/AudioSystem.h>

#ifndef LOG_TAG
#define LOG_TAG "B2G-AUDIO-TEST"
#endif

using namespace android;

int main(int argc, char **argv) {
  if(argc <= 1) {
    printf("Usage: ./b2g-audio-test <mic|speaker>");
    return -1;
  }

  status_t status;
  bool state = false;

  if(strcmp(argv[1],"mic")) {
    status = AudioSystem::isMicrophoneMuted(&state);
    LOGI("AudioSystem::isMicrophoneMuted() = %s.\n", state ? "true" : "false");

    if(state){
      /**
      * setMode is called when the audio mode changes. NORMAL mode is for
      * standard audio playback, RINGTONE when a ringtone is playing and IN_CALL
   	  * when a call is in progress.
  	  */
      status = AudioSystem::setMode(AudioSystem::MODE_IN_CALL);
      LOGI("AudioSystem::setMode(AudioSystem::MODE_IN_CALL) = %d.\n", status);
    }	else{
      status = AudioSystem::setMode(AudioSystem::MODE_IN_COMMUNICATION);
      LOGI("AudioSystem::setMode(AudioSystem::MODE_IN_COMMUNICATION) = %d.\n", status);
    }

    status = AudioSystem::muteMicrophone(!state);
    LOGI("AudioSystem::muteMicrophone(%s) = %d.\n", !state ? "true" : "false", status);

    status = AudioSystem::isMicrophoneMuted(&state);
    LOGI("AudioSystem::isMicrophoneMuted() = %s.\n", state ? "true" : "false");
    
  } else if (strcmp(argv[1],"speaker")) {
    status = AudioSystem::setMode(AudioSystem::MODE_RINGTONE); 
    LOGI("AudioSystem::setMode(AudioSystem::MODE_RINGTONE) = %d.\n", status);
    status = AudioSystem::getMasterMute(&state);
    LOGI("AudioSystem::getMasterMute() = %d. %s\n", status, state? "true":"false");
    status = AudioSystem::setMasterMute(!state);
    LOGI("AudioSystem::setMasterMute() = %d. %s\n", status);    
    status = AudioSystem::getMasterMute(&state);
    LOGI("AudioSystem::getMasterMute() = %d. %s\n", status, state? "true":"false"); 
  }

  return 0;
}
