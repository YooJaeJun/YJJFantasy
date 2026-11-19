#include "Core/YJJAdventureSubsystem.h"

#include "Data/YJJData.h"
#include "Data/YJJLoc.h"
#include "Core/YJJAudio.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Gameplay/YJJLantern.h"
#include "Gameplay/YJJShrine.h"
#include "YJJFantasy.h"

UYJJAdventureSubsystem* UYJJAdventureSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UYJJAdventureSubsystem>() : nullptr;
}

TStatId UYJJAdventureSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UYJJAdventureSubsystem, STATGROUP_Tickables);
}

bool UYJJAdventureSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UYJJAdventureSubsystem::Tick(float DeltaTime)
{
	PlayTime += DeltaTime;
	PhaseTime += DeltaTime;

	for (int32 i = Toasts.Num() - 1; i >= 0; --i)
	{
		Toasts[i].Age += DeltaTime;
		if (Toasts[i].Age > Toasts[i].Duration)
		{
			Toasts.RemoveAt(i);
		}
	}

	if (CardAge >= 0.f)
	{
		CardAge += DeltaTime;
		if (CardAge > FYJJData::Tune(TEXT("Adventure.ZoneCardDuration")))
		{
			CardAge = -1.f;
		}
	}

	UpdateZones(DeltaTime);

	if (Phase == EYJJPhase::Finale && PhaseTime > FYJJData::Tune(TEXT("Adventure.FinaleDuration")))
	{
		Phase = EYJJPhase::Ended;
		PhaseTime = 0.f;
		UE_LOG(LogYJJFantasy, Display, TEXT("[Adventure] ended playtime=%.0fs stardust=%d/%d"), PlayTime, Stardust, StardustTotal);
	}
}

void UYJJAdventureSubsystem::UpdateZones(float DeltaTime)
{
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn)
	{
		return;
	}

	// 가장 가까운(수평 거리 기준) 섬 안에 있으면 그 섬
	const FVector P = Pawn->GetActorLocation();
	int32 Best = INDEX_NONE;
	float BestDist = TNumericLimits<float>::Max();
	for (int32 i = 0; i < Zones.Num(); ++i)
	{
		const float Dist = FVector::Dist2D(P, Zones[i].Center);
		if (Dist < Zones[i].Radius && FMath::Abs(P.Z - Zones[i].Center.Z) < FYJJData::Tune(TEXT("Adventure.ZoneHeight")) && Dist < BestDist)
		{
			Best = i;
			BestDist = Dist;
		}
	}

	if (Best != CurrentZone)
	{
		CurrentZone = Best;
		if (Best != INDEX_NONE && (!VisitedZones.Contains(Best) || CardZone != Best))
		{
			// 처음 온 섬이거나, 다른 섬에 갔다 온 경우 이름 카드를 띄운다
			const bool bFirst = !VisitedZones.Contains(Best);
			VisitedZones.Add(Best);
			if (bFirst || CardAge < 0.f)
			{
				CardZone = Best;
				CardAge = 0.f;
				YJJAudio::Play(this, EYJJSound::ZoneEnter, FVector::ZeroVector, FYJJData::Tune(TEXT("Adventure.ZoneSoundVolume")));
			}
			UE_LOG(LogYJJFantasy, Display, TEXT("[Adventure] zone %s"), *Zones[Best].Name);
		}
	}
}

float UYJJAdventureSubsystem::GetStaminaBonus() const
{
	const int32 Every = FMath::Max(1, FMath::RoundToInt(FYJJData::Tune(TEXT("Adventure.StardustPerStaminaStep"))));
	return (Stardust / Every) * FYJJData::Tune(TEXT("Adventure.StaminaStep"));
}

const FYJJZone* UYJJAdventureSubsystem::GetZoneCard(float& OutAge) const
{
	OutAge = CardAge;
	return (CardAge >= 0.f && Zones.IsValidIndex(CardZone)) ? &Zones[CardZone] : nullptr;
}

void UYJJAdventureSubsystem::RegisterShrine(AYJJShrine* Shrine)
{
	Shrine_ = Shrine;
}

void UYJJAdventureSubsystem::RegisterLantern(AYJJLantern* Lantern)
{
	Lanterns.AddUnique(Lantern);
}

int32 UYJJAdventureSubsystem::GetLanternsLit() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AYJJLantern>& Lantern : Lanterns)
	{
		if (Lantern.IsValid() && Lantern->IsLit())
		{
			Count++;
		}
	}
	return Count;
}

void UYJJAdventureSubsystem::OnLanternLit(AYJJLantern* Lantern)
{
	const int32 Lit = GetLanternsLit();
	UE_LOG(LogYJJFantasy, Display, TEXT("[Adventure] lantern %s lit (%d/%d) t=%.0fs"), *Lantern->GetLanternName(), Lit, Lanterns.Num(), PlayTime);
	Toast(FYJJLoc::Format(TEXT("Toast.LanternLit"), { Lantern->GetLanternName(), Lit, Lanterns.Num() }), FYJJData::Color(TEXT("ToastLantern")), FYJJData::Tune(TEXT("Adventure.ToastLantern")));
	if (Lit == Lanterns.Num())
	{
		Toast(FYJJLoc::Get(TEXT("Toast.AllLanterns")), FYJJData::Color(TEXT("ToastAllLanterns")), FYJJData::Tune(TEXT("Adventure.ToastAllLanterns")));
		if (AYJJShrine* Shrine = Shrine_.Get())
		{
			Shrine->SetAwake(true);
		}
	}
}

void UYJJAdventureSubsystem::OnStardustCollected()
{
	const float OldBonus = GetStaminaBonus();
	Stardust++;
	if (GetStaminaBonus() > OldBonus)
	{
		Toast(FYJJLoc::Get(TEXT("Toast.StaminaUp")), FYJJData::Color(TEXT("ToastStamina")), FYJJData::Tune(TEXT("Adventure.ToastStamina")));
	}
}

void UYJJAdventureSubsystem::OnChestOpened()
{
	ChestsOpened++;
	UE_LOG(LogYJJFantasy, Display, TEXT("[Adventure] chest %d/%d"), ChestsOpened, ChestTotal);
	Toast(FYJJLoc::Format(TEXT("Toast.ChestFound"), { ChestsOpened, ChestTotal }), FYJJData::Color(TEXT("ToastChest")), FYJJData::Tune(TEXT("Adventure.ToastChest")));
}

void UYJJAdventureSubsystem::SetCheckpoint(const FVector& Location, float Yaw)
{
	Checkpoint = Location;
	CheckpointYaw = Yaw;
}

void UYJJAdventureSubsystem::StartFinale()
{
	if (Phase != EYJJPhase::Explore)
	{
		return;
	}
	Phase = EYJJPhase::Finale;
	PhaseTime = 0.f;
	UE_LOG(LogYJJFantasy, Display, TEXT("[Adventure] finale t=%.0fs"), PlayTime);
}

void UYJJAdventureSubsystem::Toast(const FString& Text, const FLinearColor& Color, float Duration)
{
	FToast& T = Toasts.AddDefaulted_GetRef();
	T.Text = Text;
	T.Color = Color;
	T.Duration = Duration;
	if (Toasts.Num() > FMath::RoundToInt(FYJJData::Tune(TEXT("Adventure.MaxToasts"))))
	{
		Toasts.RemoveAt(0);
	}
}

bool UYJJAdventureSubsystem::GetObjectiveLocation(const FVector& From, FVector& OutLocation) const
{
	if (Phase != EYJJPhase::Explore)
	{
		return false;
	}

	float BestDist = TNumericLimits<float>::Max();
	bool bFound = false;
	for (const TWeakObjectPtr<AYJJLantern>& Lantern : Lanterns)
	{
		if (Lantern.IsValid() && !Lantern->IsLit())
		{
			const float Dist = FVector::DistSquared(From, Lantern->GetActorLocation());
			if (Dist < BestDist)
			{
				BestDist = Dist;
				OutLocation = Lantern->GetActorLocation();
				bFound = true;
			}
		}
	}
	if (!bFound && Shrine_.IsValid())
	{
		OutLocation = Shrine_->GetActorLocation();
		bFound = true;
	}
	return bFound;
}

FString UYJJAdventureSubsystem::GetObjectiveText() const
{
	switch (Phase)
	{
	case EYJJPhase::Explore:
		if (GetLanternsLit() < Lanterns.Num())
		{
			return FYJJLoc::Format(TEXT("Goal.Lanterns"), { GetLanternsLit(), Lanterns.Num() });
		}
		return FYJJLoc::Get(TEXT("Goal.Shrine"));
	case EYJJPhase::Finale:
		return TEXT("");
	case EYJJPhase::Ended:
		return FYJJLoc::Get(TEXT("Goal.Free"));
	}
	return TEXT("");
}
