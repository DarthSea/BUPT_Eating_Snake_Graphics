#ifndef BUPT_SNAKE_AUDIO_H
#define BUPT_SNAKE_AUDIO_H

#include <stdbool.h>
#include "common.h"

/* Audio 模块只负责播放资源，不参与游戏规则判断。 */
void Audio_init(bool musicEnabled, bool soundEnabled);
void Audio_shutdown(void);
void Audio_setMusicEnabled(bool enabled);
void Audio_setSoundEnabled(bool enabled);
void Audio_playMusic(void);
void Audio_stopMusic(void);
void Audio_playEvent(SoundEvent event);

#endif
