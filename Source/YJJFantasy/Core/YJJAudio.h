#pragma once

#include "CoreMinimal.h"

/** 효과음·배경음. 소리는 Tools/Audio/generate_audio.py가 직접 합성한다 (외부 음원 없음) */
enum class EYJJSound : uint8
{
	Stardust,		// 별가루 줍기 (맑은 종소리)
	LanternLit,		// 등불 켜짐 (화음 + 반짝임)
	Jump,
	Land,
	GlideOpen,		// 활공 날개 펼침 (바람)
	Updraft,		// 상승 기류 진입
	Bounce,			// 버섯 튕김
	Swing,			// 검 휘두르기
	SlimeHit,
	SlimePop,
	Hurt,
	Respawn,
	ShrineAwake,	// 성소 깨어남
	Finale,			// 엔딩 화음
	ZoneEnter,		// 새 섬 도착
};

class USoundBase;

/** 소리 재생 도우미. 모든 함수는 정적. 소리 에셋은 /Game/YJJFantasy/Audio/<이름> */
class YJJFANTASY_API YJJAudio
{
public:
	/** Location이 0이면 2D로 재생 (멀수록 작게) */
	static void Play(const UObject* WorldContext, EYJJSound Sound, const FVector& Location = FVector::ZeroVector, float Volume = 1.f, float Pitch = 1.f);

	/** 배경음 + 바람 소리 (반복) */
	static void StartAmbience(const UObject* WorldContext);

private:
	static const TCHAR* SoundName(EYJJSound Sound);
	static USoundBase* LoadSound(const TCHAR* Name);

	/** 거리 감쇠를 이 아래로는 무시 */
	static constexpr float MinAudibleScale = 0.02f;

	static TMap<FString, TWeakObjectPtr<USoundBase>> Cache;
};
