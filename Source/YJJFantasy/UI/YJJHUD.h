#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "YJJHUD.generated.h"

class AYJJHero;
class UYJJAdventureSubsystem;
struct FCompositeFont;

/**
 * 게임 HUD (Canvas, 위젯 에셋 없음).
 * 목표, 하트, 기력 고리, 목표 표시, 상호작용 안내, 섬 이름 카드, 알림, 엔딩 카드.
 */
UCLASS()
class YJJFANTASY_API AYJJHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	enum class EAlign : uint8 { Left, Center, Right };

	float S(float V) const { return V * UIScale; }
	void Text(const FString& Str, float X, float Y, float Size, const FLinearColor& Color, EAlign Align = EAlign::Left, float Outline = 2.f, const FLinearColor& OutlineColor = FLinearColor(0.05f, 0.07f, 0.12f, 1.f));
	FVector2D Measure(const FString& Str, float Size);
	void Panel(float X, float Y, float W, float H, const FLinearColor& Color);
	void Circle(float X, float Y, float R, const FLinearColor& Color, int32 Sides = 20);
	void Diamond(float X, float Y, float R, const FLinearColor& Color);
	void Heart(float X, float Y, float R, const FLinearColor& Color);

	void DrawObjective(const AYJJHero& Hero, const UYJJAdventureSubsystem& Adv);
	void DrawHearts(const AYJJHero& Hero);
	void DrawStamina(const AYJJHero& Hero);
	void DrawMarker(const AYJJHero& Hero, const UYJJAdventureSubsystem& Adv);
	void DrawPrompt(const AYJJHero& Hero);
	void DrawZoneCard(const UYJJAdventureSubsystem& Adv);
	void DrawToasts(const UYJJAdventureSubsystem& Adv);
	void DrawIntro(const UYJJAdventureSubsystem& Adv);
	void DrawEnding(const UYJJAdventureSubsystem& Adv);
	void DrawOverlays(const AYJJHero& Hero);

	TSharedPtr<const FCompositeFont> GetGameFont();
	TSharedPtr<const FCompositeFont> GameFont;

	float UIScale = 1.f;
	float StaminaAlpha = 0.f;
	float DisplayStamina = 1.f;
};
