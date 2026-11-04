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

namespace YJJAudio
{
	/** Location이 0이면 2D로 재생 */
	void Play(const UObject* WorldContext, EYJJSound Sound, const FVector& Location = FVector::ZeroVector, float Volume = 1.f, float Pitch = 1.f);

	/** 배경음 + 바람 소리 (반복) */
	void StartAmbience(const UObject* WorldContext);
}
