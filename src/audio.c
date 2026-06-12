#include <windows.h>
#include <mmsystem.h>
#include <tchar.h>
#include <io.h>

#include "audio.h"

#ifdef _MSC_VER
#pragma comment(lib, "winmm.lib")
#endif

static bool gMusicEnabled = true;
static bool gSoundEnabled = true;
static bool gMusicOpened = false;

static const TCHAR *soundPath(SoundEvent event)
{
    switch (event) {
    case SOUND_START:
        return _T("assets\\audio\\start.wav");
    case SOUND_EAT_NORMAL:
        return _T("assets\\audio\\eat_food.wav");
    case SOUND_EAT_BONUS:
        return _T("assets\\audio\\eat_bonus.wav");
    case SOUND_EAT_SPEED:
        return _T("assets\\audio\\eat_speed.wav");
    case SOUND_EAT_SLOW:
        return _T("assets\\audio\\eat_slow.wav");
    case SOUND_SHIELD:
        return _T("assets\\audio\\shield.wav");
    case SOUND_TRAP:
        return _T("assets\\audio\\trap.wav");
    case SOUND_COLLISION:
        return _T("assets\\audio\\collision.wav");
    case SOUND_BOW:
        return _T("assets\\audio\\bow.wav");
    case SOUND_ARROW_HIT:
        return _T("assets\\audio\\arrow_hit.wav");
    default:
        return NULL;
    }
}

void Audio_init(bool musicEnabled, bool soundEnabled)
{
    gMusicEnabled = musicEnabled;
    gSoundEnabled = soundEnabled;
    Audio_playMusic();
}

void Audio_shutdown(void)
{
    Audio_stopMusic();
}

void Audio_setMusicEnabled(bool enabled)
{
    gMusicEnabled = enabled;
    if (gMusicEnabled) {
        Audio_playMusic();
    } else {
        Audio_stopMusic();
    }
}

void Audio_setSoundEnabled(bool enabled)
{
    gSoundEnabled = enabled;
}

void Audio_playMusic(void)
{
    if (!gMusicEnabled || _taccess(_T("assets\\audio\\bgm.wav"), 0) != 0) {
        return;
    }

    if (!gMusicOpened) {
        if (mciSendString(_T("open \"assets\\audio\\bgm.wav\" type waveaudio alias snake_bgm"),
                NULL, 0, NULL) != 0) {
            return;
        }
        gMusicOpened = true;
    }

    mciSendString(_T("play snake_bgm repeat"), NULL, 0, NULL);
}

void Audio_stopMusic(void)
{
    if (!gMusicOpened) {
        return;
    }

    mciSendString(_T("stop snake_bgm"), NULL, 0, NULL);
    mciSendString(_T("close snake_bgm"), NULL, 0, NULL);
    gMusicOpened = false;
}

void Audio_playEvent(SoundEvent event)
{
    const TCHAR *path;

    if (!gSoundEnabled || event == SOUND_NONE) {
        return;
    }

    path = soundPath(event);
    if (path == NULL || _taccess(path, 0) != 0) {
        return;
    }

    PlaySound(path, NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
}
