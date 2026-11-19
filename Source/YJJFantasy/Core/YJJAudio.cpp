#include "Core/YJJAudio.h"

#include "Camera/PlayerCameraManager.h"
#include "Data/YJJData.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"

TMap<FString, TWeakObjectPtr<USoundBase>> YJJAudio::Cache;

const TCHAR* YJJAudio::SoundName(EYJJSound Sound)
{
	switch (Sound)
	{
	case EYJJSound::Stardust: return TEXT("SFX_Stardust");
	case EYJJSound::LanternLit: return TEXT("SFX_LanternLit");
	case EYJJSound::Jump: return TEXT("SFX_Jump");
	case EYJJSound::Land: return TEXT("SFX_Land");
	case EYJJSound::GlideOpen: return TEXT("SFX_GlideOpen");
	case EYJJSound::Updraft: return TEXT("SFX_Updraft");
	case EYJJSound::Bounce: return TEXT("SFX_Bounce");
	case EYJJSound::Swing: return TEXT("SFX_Swing");
	case EYJJSound::SlimeHit: return TEXT("SFX_SlimeHit");
	case EYJJSound::SlimePop: return TEXT("SFX_SlimePop");
	case EYJJSound::Hurt: return TEXT("SFX_Hurt");
	case EYJJSound::Respawn: return TEXT("SFX_Respawn");
	case EYJJSound::ShrineAwake: return TEXT("SFX_ShrineAwake");
	case EYJJSound::Finale: return TEXT("SFX_Finale");
	case EYJJSound::ZoneEnter: return TEXT("SFX_ZoneEnter");
	}
	return nullptr;
}

USoundBase* YJJAudio::LoadSound(const TCHAR* Name)
{
	if (TWeakObjectPtr<USoundBase>* Found = Cache.Find(Name))
	{
		if (Found->IsValid())
		{
			return Found->Get();
		}
	}
	const FString Path = FString::Printf(TEXT("/Game/YJJFantasy/Audio/%s.%s"), Name, Name);
	USoundBase* Sound = LoadObject<USoundBase>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (Sound)
	{
		Sound->AddToRoot();
		Cache.Add(Name, Sound);
	}
	return Sound;
}

void YJJAudio::Play(const UObject* WorldContext, EYJJSound Sound, const FVector& Location, float Volume, float Pitch)
{
	const TCHAR* Name = SoundName(Sound);
	USoundBase* Asset = Name ? LoadSound(Name) : nullptr;
	if (!Asset || !WorldContext)
	{
		return;
	}
	// 소리가 너무 멀어서 안 들리는 일이 없도록 3D 감쇠 없이 2D로 재생하고, 거리만큼 줄인다
	float Scale = 1.f;
	if (!Location.IsZero())
	{
		if (const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(WorldContext, 0))
		{
			const float Dist = FVector::Dist(Camera->GetCameraLocation(), Location);
			Scale = FMath::Clamp(1.f - (Dist - FYJJData::Tune(TEXT("Audio.FullVolumeDistance"))) / FYJJData::Tune(TEXT("Audio.FadeDistance")), 0.f, 1.f);
		}
	}
	if (Scale > MinAudibleScale)
	{
		UGameplayStatics::PlaySound2D(WorldContext, Asset, Volume * Scale, Pitch);
	}
}

void YJJAudio::StartAmbience(const UObject* WorldContext)
{
	if (USoundWave* Bgm = Cast<USoundWave>(LoadSound(TEXT("BGM_SkyIslands"))))
	{
		Bgm->bLooping = true;
		UGameplayStatics::SpawnSound2D(WorldContext, Bgm, FYJJData::Tune(TEXT("Audio.MusicVolume")));
	}
	if (USoundWave* Wind = Cast<USoundWave>(LoadSound(TEXT("AMB_Wind"))))
	{
		Wind->bLooping = true;
		UGameplayStatics::SpawnSound2D(WorldContext, Wind, FYJJData::Tune(TEXT("Audio.WindVolume")));
	}
}
