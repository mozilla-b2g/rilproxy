#include <utils/Log.h>
#include <media/AudioSystem.h>

#define LOG_TAG "B2G-MICROPHONE-TEST"

using namespace android;

int main(int argc, char **argv) {

  status_t status;
  bool state = false;

  status = AudioSystem::isMicrophoneMuted(&state);
  LOGI("AudioSystem::isMicrophoneMuted() = %s.\n", state ? "true" : "false");

  if(state){
    status = AudioSystem::setMode(AudioSystem::MODE_IN_CALL);
    LOGI("AudioSystem::setMode(AudioSystem::MODE_IN_CALL) = %d.\n", status);
  }else{
    status = AudioSystem::setMode(AudioSystem::MODE_IN_COMMUNICATION);
    LOGI("AudioSystem::setMode(AudioSystem::MODE_IN_COMMUNICATION) = %d.\n", status);
  }

  status = AudioSystem::muteMicrophone(!state);
  LOGI("AudioSystem::muteMicrophone(%s) = %d.\n", !state ? "true" : "false", status);
  
  status = AudioSystem::isMicrophoneMuted(&state);
  LOGI("AudioSystem::isMicrophoneMuted() = %s.\n", state ? "true" : "false");

  return 0;
}
