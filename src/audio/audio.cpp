#include <utils/Log.h>
#include <media/AudioSystem.h>

using namespace android;

int main(int argc, char **argv) {
  bool state;

  if(argc <= 1) {
    fprintf(stderr, "Usage: b2g-dialer-audio <dial|dial-speaker-on-off|dial-microphone-mute-unmute|hangup>");
    return -1;
  }
  
  if(strcmp(argv[1], "dial") == 0){
    AudioSystem::setPhoneState(AudioSystem::MODE_IN_CALL);
    AudioSystem::setForceUse(AudioSystem::FOR_COMMUNICATION, AudioSystem::FORCE_NONE);
  }else if(strcmp(argv[1], "dial-speaker-on-off") == 0){
    AudioSystem::setPhoneState(AudioSystem::MODE_IN_CALL);
    AudioSystem::setForceUse(AudioSystem::FOR_COMMUNICATION, 
			     AudioSystem::getForceUse(AudioSystem::FOR_COMMUNICATION) == AudioSystem::FORCE_NONE ? AudioSystem::FORCE_SPEAKER : AudioSystem::FORCE_NONE);
  }else if(strcmp(argv[1], "dial-microphone-mute-unmute") == 0){
    AudioSystem::isMicrophoneMuted(&state);

    state ? AudioSystem::setPhoneState(AudioSystem::MODE_IN_CALL) :
      AudioSystem::setPhoneState(AudioSystem::MODE_IN_COMMUNICATION); 

    AudioSystem::muteMicrophone(!state);
  }else if(strcmp(argv[1], "hangup") == 0){
    AudioSystem::setPhoneState(AudioSystem::MODE_NORMAL);
  }else{
    fprintf(stderr, "Usage: b2g-dialer-audio <dial|dial-speaker-on-off|dial-microphone-mute-unmute|hangup>");
    return -1;
  } 

  return 0;
}
