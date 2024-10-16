#pragma once
#include "libspeexdsp/include/speex_preprocess.h"

class CSpeexPreprocess
{
public:
	CSpeexPreprocess(int sampleRate, int samplesPerBuffer);
	~CSpeexPreprocess();

	// автоматическая настройка микрофона
	bool EnableAGC(bool enable);
	bool IsAGCEnabled();

	bool IsDenoiseEnabled();
	bool EnableDenoise(bool enable);

	int GetDenoiseLevel();
	bool SetDenoiseLevel(int level);

	void RunPreprocess(short* buffer);

private:
	SpeexPreprocessState* m_pState = nullptr;
};
