#include "UI/YJJHUD.h"

#include "CanvasItem.h"
#include "Core/YJJAdventureSubsystem.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "TextureResource.h"
#include "Fonts/CompositeFont.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Paths.h"
#include "Player/YJJHero.h"
#include "YJJAssetPaths.h"

namespace
{
	const FLinearColor Ink(0.05f, 0.07f, 0.12f, 1.f);
	const FLinearColor Cream(1.f, 0.97f, 0.9f, 1.f);
	const FLinearColor GoldText(1.f, 0.84f, 0.45f, 1.f);
	const FLinearColor Sky(0.6f, 0.9f, 1.f, 1.f);

	const UFont* BaseFont()
	{
		static const UFont* Font = LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
		return Font;
	}

	FLinearColor WithAlpha(FLinearColor C, float A)
	{
		C.A *= A;
		return C;
	}
}

TSharedPtr<const FCompositeFont> AYJJHUD::GetGameFont()
{
	if (GameFont.IsValid())
	{
		return GameFont;
	}
	// TTF 파일을 직접 읽는다 (커맨드릿에선 폰트 임포트가 안 되므로 에셋 대신 파일)
	const FString FontPath = FPaths::ProjectContentDir() / YJJAssetPaths::GameFontFile;
	if (!FPaths::FileExists(FontPath))
	{
		return nullptr;
	}
	TSharedRef<FCompositeFont> Composite = MakeShared<FCompositeFont>();
	Composite->DefaultTypeface.Fonts.Add(FTypefaceEntry(TEXT("Regular"), FontPath, EFontHinting::Default, EFontLoadingPolicy::LazyLoad));
	if (const UFont* Base = BaseFont())
	{
		if (const FCompositeFont* Fallback = Base->GetCompositeFont())
		{
			Composite->FallbackTypeface.Typeface = Fallback->DefaultTypeface;
		}
	}
	GameFont = Composite;
	return GameFont;
}

void AYJJHUD::Text(const FString& Str, float X, float Y, float Size, const FLinearColor& Color, EAlign Align, float Outline, const FLinearColor& OutlineColor)
{
	const UFont* Base = BaseFont();
	if (!Base || Str.IsEmpty() || Color.A <= 0.01f)
	{
		return;
	}
	const int32 FontSize = FMath::Max(FMath::RoundToInt(S(Size)), 6);
	const TSharedPtr<const FCompositeFont> Composite = GetGameFont();
	FSlateFontInfo Font = Composite.IsValid() ? FSlateFontInfo(Composite, FontSize) : FSlateFontInfo(Base, FontSize);
	if (Outline > 0.f)
	{
		Font.OutlineSettings.OutlineSize = FMath::Max(FMath::RoundToInt(S(Outline)), 1);
		Font.OutlineSettings.OutlineColor = FLinearColor(OutlineColor.R, OutlineColor.G, OutlineColor.B, OutlineColor.A * Color.A);
	}

	float DrawX = X;
	if (Align != EAlign::Left)
	{
		const FVector2D Size2 = Measure(Str, Size);
		DrawX = (Align == EAlign::Center) ? X - Size2.X * 0.5f : X - Size2.X;
	}
	FCanvasTextItem Item(FVector2D(DrawX, Y), FText::FromString(Str), Font, Color);
	Item.Font = Base;
	Item.bCentreY = true;
	Canvas->DrawItem(Item);
}

FVector2D AYJJHUD::Measure(const FString& Str, float Size)
{
	const UFont* Base = BaseFont();
	const TSharedPtr<const FCompositeFont> Composite = GetGameFont();
	const int32 FontSize = FMath::Max(FMath::RoundToInt(S(Size)), 6);
	FSlateFontInfo Font = Composite.IsValid() ? FSlateFontInfo(Composite, FontSize) : FSlateFontInfo(Base, FontSize);
	if (FSlateApplication::IsInitialized() && FSlateApplication::Get().GetRenderer())
	{
		return FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Str, Font);
	}
	return FVector2D(Str.Len() * S(Size) * 0.6f, S(Size));
}

void AYJJHUD::Panel(float X, float Y, float W, float H, const FLinearColor& Color)
{
	// 둥근 모서리 반투명 패널: 겹쳐 그리면 반투명이 진해지므로 테두리 다각형 하나를 부채꼴로 채운다
	const float C = FMath::Min(S(12.f), H * 0.5f);
	TArray<FVector2D> Outline;
	const FVector2D Corners[4] = { FVector2D(X + W - C, Y + H - C), FVector2D(X + C, Y + H - C), FVector2D(X + C, Y + C), FVector2D(X + W - C, Y + C) };
	for (int32 c = 0; c < 4; ++c)
	{
		for (int32 k = 0; k <= 5; ++k)
		{
			const float A = (c * 90.f + k * 18.f) * PI / 180.f;
			Outline.Add(Corners[c] + FVector2D(FMath::Cos(A), FMath::Sin(A)) * C);
		}
	}
	const FVector2D Center(X + W * 0.5f, Y + H * 0.5f);
	TArray<FCanvasUVTri> Tris;
	for (int32 i = 0; i < Outline.Num(); ++i)
	{
		FCanvasUVTri& T = Tris.AddDefaulted_GetRef();
		T.V0_Pos = Center;
		T.V1_Pos = Outline[i];
		T.V2_Pos = Outline[(i + 1) % Outline.Num()];
		T.V0_Color = T.V1_Color = T.V2_Color = Color;
	}
	FCanvasTriangleItem Item(Tris, GWhiteTexture);
	Item.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Item);
}

void AYJJHUD::Circle(float X, float Y, float R, const FLinearColor& Color, int32 Sides)
{
	TArray<FCanvasUVTri> Tris;
	for (int32 i = 0; i < Sides; ++i)
	{
		const float A0 = i * TWO_PI / Sides;
		const float A1 = (i + 1) * TWO_PI / Sides;
		FCanvasUVTri& T = Tris.AddDefaulted_GetRef();
		T.V0_Pos = FVector2D(X, Y);
		T.V1_Pos = FVector2D(X + FMath::Cos(A0) * R, Y + FMath::Sin(A0) * R);
		T.V2_Pos = FVector2D(X + FMath::Cos(A1) * R, Y + FMath::Sin(A1) * R);
		T.V0_Color = T.V1_Color = T.V2_Color = Color;
	}
	FCanvasTriangleItem Item(Tris, GWhiteTexture);
	Item.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Item);
}

void AYJJHUD::Diamond(float X, float Y, float R, const FLinearColor& Color)
{
	Circle(X, Y, R, Color, 4);
}

void AYJJHUD::Heart(float X, float Y, float R, const FLinearColor& Color)
{
	// 원 두 개 + 아래 삼각형
	Circle(X - R * 0.5f, Y - R * 0.15f, R * 0.56f, Color, 16);
	Circle(X + R * 0.5f, Y - R * 0.15f, R * 0.56f, Color, 16);
	TArray<FCanvasUVTri> Tris;
	FCanvasUVTri& T = Tris.AddDefaulted_GetRef();
	T.V0_Pos = FVector2D(X - R * 1.03f, Y);
	T.V1_Pos = FVector2D(X + R * 1.03f, Y);
	T.V2_Pos = FVector2D(X, Y + R * 1.05f);
	T.V0_Color = T.V1_Color = T.V2_Color = Color;
	FCanvasTriangleItem Item(Tris, GWhiteTexture);
	Item.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Item);
}

// ============================================================

void AYJJHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas)
	{
		return;
	}
	UIScale = FMath::Clamp(Canvas->ClipY / 1080.f, 0.5f, 2.f);

	const APlayerController* PC = GetOwningPlayerController();
	const AYJJHero* Hero = PC ? Cast<AYJJHero>(PC->GetPawn()) : nullptr;
	const UYJJAdventureSubsystem* Adv = UYJJAdventureSubsystem::Get(this);
	if (!Hero || !Adv)
	{
		return;
	}

	if (Adv->GetPhase() == EYJJPhase::Explore)
	{
		DrawMarker(*Hero, *Adv);
		DrawStamina(*Hero);
		DrawObjective(*Hero, *Adv);
		DrawHearts(*Hero);
		DrawPrompt(*Hero);
		DrawZoneCard(*Adv);
		DrawToasts(*Adv);
		DrawIntro(*Adv);
	}
	else
	{
		DrawEnding(*Adv);
		if (Adv->GetPhase() == EYJJPhase::Ended)
		{
			DrawObjective(*Hero, *Adv);
			DrawToasts(*Adv);
		}
	}
	DrawOverlays(*Hero);
}

void AYJJHUD::DrawObjective(const AYJJHero& Hero, const UYJJAdventureSubsystem& Adv)
{
	const FString Goal = Adv.GetObjectiveText();
	const float X = S(36.f);
	const float Y = S(36.f);
	const FVector2D Size = Measure(Goal, 26.f);
	Panel(X, Y, Size.X + S(74.f), S(52.f), FLinearColor(0.03f, 0.06f, 0.12f, 0.45f));
	Diamond(X + S(28.f), Y + S(26.f), S(11.f), GoldText);
	Diamond(X + S(28.f), Y + S(26.f), S(5.f), FLinearColor(1.f, 1.f, 1.f, 0.9f));
	Text(Goal, X + S(50.f), Y + S(26.f), 26.f, Cream);

	// 등불 진행 점
	const int32 Count = Adv.GetLanternCount();
	const int32 Lit = Adv.GetLanternsLit();
	for (int32 i = 0; i < Count; ++i)
	{
		const float CX = X + S(28.f + i * 26.f);
		const float CY = Y + S(76.f);
		Circle(CX, CY, S(9.f), FLinearColor(0.f, 0.f, 0.f, 0.35f));
		Circle(CX, CY, S(6.5f), i < Lit ? GoldText : FLinearColor(1.f, 1.f, 1.f, 0.25f));
	}

	// 별가루
	const FString Dust = FString::Printf(TEXT("%d / %d"), Adv.GetStardust(), Adv.GetStardustTotal());
	const float DY = Y + S(112.f);
	Circle(X + S(28.f), DY, S(10.f), FLinearColor(1.f, 0.85f, 0.45f, 0.35f));
	Diamond(X + S(28.f), DY, S(8.f), FLinearColor(1.f, 0.95f, 0.75f, 1.f));
	Text(Dust, X + S(48.f), DY, 22.f, FLinearColor(1.f, 0.92f, 0.7f, 1.f));

	// 보물 상자
	if (Adv.GetChestTotal() > 0)
	{
		const float CY = DY + S(34.f);
		Panel(X + S(18.f), CY - S(9.f), S(20.f), S(18.f), FLinearColor(0.85f, 0.55f, 0.25f, 1.f));
		Panel(X + S(18.f), CY - S(10.f), S(20.f), S(6.f), FLinearColor(1.f, 0.8f, 0.35f, 1.f));
		Text(FString::Printf(TEXT("%d / %d"), Adv.GetChestsOpened(), Adv.GetChestTotal()), X + S(48.f), CY, 22.f, FLinearColor(1.f, 0.85f, 0.6f, 1.f));
	}
}

void AYJJHUD::DrawHearts(const AYJJHero& Hero)
{
	const float R = S(15.f);
	const float Gap = S(40.f);
	const int32 Max = Hero.GetMaxHealth();
	const float X0 = Canvas->ClipX - S(40.f) - (Max - 1) * Gap;
	const float Y = S(58.f);
	const float Pulse = Hero.GetHurtFlash();
	for (int32 i = 0; i < Max; ++i)
	{
		const float X = X0 + i * Gap;
		Heart(X, Y, R * 1.22f, FLinearColor(0.f, 0.f, 0.f, 0.4f));
		const bool bFull = i < Hero.GetHealth();
		const float Bounce = (bFull && i == Hero.GetHealth() - 1) ? Pulse * 0.3f : 0.f;
		Heart(X, Y, R * (1.f + Bounce), bFull ? FLinearColor(1.f, 0.35f, 0.42f, 1.f) : FLinearColor(1.f, 1.f, 1.f, 0.18f));
		if (bFull)
		{
			Circle(X - R * 0.45f, Y - R * 0.35f, R * 0.2f, FLinearColor(1.f, 1.f, 1.f, 0.7f), 8);
		}
	}
}

void AYJJHUD::DrawStamina(const AYJJHero& Hero)
{
	const float Max = Hero.GetMaxStamina();
	const float Frac = Max > 0.f ? Hero.GetStamina() / Max : 1.f;
	const float Dt = GetWorld()->GetDeltaSeconds();
	DisplayStamina = FMath::FInterpTo(DisplayStamina, Frac, Dt, 10.f);
	StaminaAlpha = FMath::FInterpTo(StaminaAlpha, Frac < 0.999f ? 1.f : 0.f, Dt, Frac < 0.999f ? 10.f : 2.f);
	if (StaminaAlpha < 0.02f)
	{
		return;
	}

	// 영웅 오른쪽 옆에 세로 원호
	FVector2D Screen;
	const APlayerController* PC = GetOwningPlayerController();
	if (!PC || !PC->ProjectWorldLocationToScreen(Hero.GetActorLocation() + FVector(0, 0, 30), Screen))
	{
		return;
	}
	const float CX = Screen.X + S(70.f);
	const float CY = Screen.Y;
	const float R = S(46.f);
	const int32 Dots = 26;
	const bool bWarn = Hero.IsExhausted() || Frac < 0.25f;
	const float Blink = bWarn ? 0.6f + 0.4f * FMath::Sin(GetWorld()->GetTimeSeconds() * 14.f) : 1.f;
	const FLinearColor Fill = bWarn ? FLinearColor(1.f, 0.4f, 0.3f, StaminaAlpha * Blink) : FLinearColor(0.75f, 1.f, 0.45f, StaminaAlpha);
	for (int32 i = 0; i < Dots; ++i)
	{
		const float T = i / static_cast<float>(Dots - 1);
		const float A = FMath::DegreesToRadians(-70.f + 140.f * (1.f - T));
		const float X = CX + FMath::Cos(A) * R * 0.45f;
		const float Y = CY - FMath::Sin(A) * R;
		Circle(X, Y, S(4.6f), FLinearColor(0.f, 0.f, 0.f, 0.45f * StaminaAlpha), 8);
		if (T <= DisplayStamina)
		{
			Circle(X, Y, S(3.4f), Fill, 8);
		}
	}
}

void AYJJHUD::DrawMarker(const AYJJHero& Hero, const UYJJAdventureSubsystem& Adv)
{
	FVector Target;
	if (!Adv.GetObjectiveLocation(Hero.GetActorLocation(), Target))
	{
		return;
	}
	Target += FVector(0, 0, 520);
	const APlayerController* PC = GetOwningPlayerController();
	FVector CamLoc;
	FRotator CamRot;
	PC->GetPlayerViewPoint(CamLoc, CamRot);
	const float Dist = FVector::Dist(Hero.GetActorLocation(), Target) / 100.f;
	if (Dist < 6.f)
	{
		return;
	}

	FVector2D Screen;
	const bool bInFront = FVector::DotProduct(CamRot.Vector(), (Target - CamLoc).GetSafeNormal()) > 0.f;
	const bool bProjected = PC->ProjectWorldLocationToScreen(Target, Screen);
	const float Margin = S(60.f);
	const float Time = GetWorld()->GetTimeSeconds();
	const float Bob = FMath::Sin(Time * 3.f) * S(4.f);

	if (bInFront && bProjected && Screen.X > Margin && Screen.X < Canvas->ClipX - Margin && Screen.Y > Margin && Screen.Y < Canvas->ClipY - Margin)
	{
		Diamond(Screen.X, Screen.Y + Bob, S(16.f), FLinearColor(0.f, 0.f, 0.f, 0.35f));
		Diamond(Screen.X, Screen.Y + Bob, S(12.f), GoldText);
		Diamond(Screen.X, Screen.Y + Bob, S(5.f), FLinearColor::White);
		Text(FString::Printf(TEXT("%.0fm"), Dist), Screen.X, Screen.Y + Bob + S(28.f), 18.f, Cream, EAlign::Center);
		return;
	}

	// 화면 밖: 가장자리 화살표
	const FVector Local = CamRot.UnrotateVector(Target - CamLoc);
	FVector2D Dir(Local.Y, -Local.Z);
	if (Dir.IsNearlyZero())
	{
		Dir = FVector2D(0, 1);
	}
	Dir.Normalize();
	const FVector2D Center(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);
	const float Scale = FMath::Min((Canvas->ClipX * 0.5f - Margin) / FMath::Max(FMath::Abs(Dir.X), 0.001f), (Canvas->ClipY * 0.5f - Margin) / FMath::Max(FMath::Abs(Dir.Y), 0.001f));
	const FVector2D P = Center + Dir * Scale;
	TArray<FCanvasUVTri> Tris;
	FCanvasUVTri& T = Tris.AddDefaulted_GetRef();
	const FVector2D Perp(-Dir.Y, Dir.X);
	T.V0_Pos = P + Dir * S(20.f);
	T.V1_Pos = P - Dir * S(8.f) + Perp * S(14.f);
	T.V2_Pos = P - Dir * S(8.f) - Perp * S(14.f);
	T.V0_Color = T.V1_Color = T.V2_Color = GoldText;
	FCanvasTriangleItem Item(Tris, GWhiteTexture);
	Item.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Item);
	Text(FString::Printf(TEXT("%.0fm"), Dist), P.X - Dir.X * S(34.f), P.Y - Dir.Y * S(34.f), 17.f, Cream, EAlign::Center);
}

void AYJJHUD::DrawPrompt(const AYJJHero& Hero)
{
	const FString Prompt = Hero.GetInteractPrompt();
	if (Prompt.IsEmpty())
	{
		return;
	}
	const float X = Canvas->ClipX * 0.5f + S(140.f);
	const float Y = Canvas->ClipY * 0.5f + S(40.f);
	const FVector2D Size = Measure(Prompt, 24.f);
	Panel(X, Y - S(24.f), Size.X + S(80.f), S(48.f), FLinearColor(0.03f, 0.06f, 0.12f, 0.6f));
	Panel(X + S(8.f), Y - S(17.f), S(34.f), S(34.f), FLinearColor(1.f, 0.95f, 0.85f, 0.95f));
	Text(TEXT("E"), X + S(25.f), Y, 22.f, Ink, EAlign::Center, 0.f);
	Text(Prompt, X + S(56.f), Y, 24.f, Cream);
}

void AYJJHUD::DrawZoneCard(const UYJJAdventureSubsystem& Adv)
{
	float Age;
	const FYJJZone* Zone = Adv.GetZoneCard(Age);
	if (!Zone)
	{
		return;
	}
	// 0.6초 페이드 인, 3초 유지, 1.4초 페이드 아웃
	const float A = FMath::Clamp(Age / 0.6f, 0.f, 1.f) * FMath::Clamp((5.f - Age) / 1.4f, 0.f, 1.f);
	const float Ease = 1.f - FMath::Pow(1.f - FMath::Clamp(Age / 0.9f, 0.f, 1.f), 3.f);
	const float CX = Canvas->ClipX * 0.5f;
	const float CY = Canvas->ClipY * 0.24f;

	const float LineW = S(260.f) * Ease;
	FCanvasTileItem Line(FVector2D(CX - LineW, CY + S(42.f)), FVector2D(LineW * 2.f, S(2.f)), WithAlpha(GoldText, A * 0.8f));
	Line.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Line);
	Diamond(CX - LineW, CY + S(43.f), S(6.f), WithAlpha(GoldText, A));
	Diamond(CX + LineW, CY + S(43.f), S(6.f), WithAlpha(GoldText, A));
	Diamond(CX, CY + S(43.f), S(8.f), WithAlpha(FLinearColor::White, A));

	Text(Zone->Name, CX, CY - S(4.f), 64.f, WithAlpha(Cream, A), EAlign::Center, 3.f, FLinearColor(0.1f, 0.16f, 0.3f, 0.9f));
	Text(Zone->Subtitle, CX, CY + S(78.f), 24.f, WithAlpha(FLinearColor(0.85f, 0.93f, 1.f, 1.f), A), EAlign::Center, 2.f);
}

void AYJJHUD::DrawToasts(const UYJJAdventureSubsystem& Adv)
{
	const TArray<UYJJAdventureSubsystem::FToast>& Toasts = Adv.GetToasts();
	float Y = Canvas->ClipY - S(150.f);
	for (int32 i = Toasts.Num() - 1; i >= 0; --i)
	{
		const UYJJAdventureSubsystem::FToast& T = Toasts[i];
		const float A = FMath::Clamp(T.Age / 0.25f, 0.f, 1.f) * FMath::Clamp((T.Duration - T.Age) / 0.6f, 0.f, 1.f);
		const float Slide = (1.f - FMath::Clamp(T.Age / 0.3f, 0.f, 1.f)) * S(20.f);
		const FVector2D Size = Measure(T.Text, 26.f);
		Panel(Canvas->ClipX * 0.5f - Size.X * 0.5f - S(30.f), Y - S(24.f) + Slide, Size.X + S(60.f), S(48.f), FLinearColor(0.03f, 0.06f, 0.12f, 0.5f * A));
		Text(T.Text, Canvas->ClipX * 0.5f, Y + Slide, 26.f, WithAlpha(T.Color, A), EAlign::Center);
		Y -= S(58.f);
	}
}

void AYJJHUD::DrawIntro(const UYJJAdventureSubsystem& Adv)
{
	// 시작 직후 제목과 조작 안내 (튜토리얼 대신 한 줄)
	const float T = Adv.GetPlayTime();
	if (T > 14.f)
	{
		return;
	}
	const float TitleA = FMath::Clamp((T - 0.5f) / 1.2f, 0.f, 1.f) * FMath::Clamp((6.5f - T) / 1.5f, 0.f, 1.f);
	const float CX = Canvas->ClipX * 0.5f;
	const float CY = Canvas->ClipY * 0.36f;
	if (TitleA > 0.f)
	{
		Text(TEXT("하늘섬의 등불"), CX, CY, 92.f, WithAlpha(Cream, TitleA), EAlign::Center, 4.f, FLinearColor(0.12f, 0.2f, 0.38f, 0.9f));
		Text(TEXT("빛을 잃은 바람의 성소. 다섯 섬의 등불을 다시 밝히자"), CX, CY + S(84.f), 28.f, WithAlpha(FLinearColor(0.88f, 0.95f, 1.f, 1.f), TitleA), EAlign::Center, 2.f);
	}
	const float HintA = FMath::Clamp((T - 2.f) / 1.f, 0.f, 1.f) * FMath::Clamp((14.f - T) / 1.5f, 0.f, 1.f);
	if (HintA > 0.f)
	{
		const FString Hint = TEXT("WASD 이동   Shift 달리기   Space 점프 · 공중에서 활공   좌클릭 공격   E 상호작용");
		const FVector2D Size = Measure(Hint, 21.f);
		Panel(CX - Size.X * 0.5f - S(24.f), Canvas->ClipY - S(70.f), Size.X + S(48.f), S(40.f), FLinearColor(0.03f, 0.06f, 0.12f, 0.45f * HintA));
		Text(Hint, CX, Canvas->ClipY - S(50.f), 21.f, WithAlpha(Cream, HintA), EAlign::Center, 1.5f);
	}
}

void AYJJHUD::DrawEnding(const UYJJAdventureSubsystem& Adv)
{
	const bool bEnded = Adv.GetPhase() == EYJJPhase::Ended;
	const float T = bEnded ? Adv.GetPhaseTime() + 14.f : Adv.GetPhaseTime();
	const float CX = Canvas->ClipX * 0.5f;

	// 영화 같은 위아래 검은 띠
	const float Bar = FMath::Clamp(T / 1.5f, 0.f, 1.f) * FMath::Clamp((24.f - T) / 2.f, 0.f, 1.f) * S(90.f);
	if (Bar > 0.5f)
	{
		FCanvasTileItem Top(FVector2D(0, 0), FVector2D(Canvas->ClipX, Bar), FLinearColor(0, 0, 0, 1));
		Canvas->DrawItem(Top);
		FCanvasTileItem Bottom(FVector2D(0, Canvas->ClipY - Bar), FVector2D(Canvas->ClipX, Bar), FLinearColor(0, 0, 0, 1));
		Canvas->DrawItem(Bottom);
	}

	const float A = FMath::Clamp((T - 7.f) / 1.5f, 0.f, 1.f) * FMath::Clamp((24.f - T) / 2.f, 0.f, 1.f);
	if (A <= 0.f)
	{
		return;
	}
	const float CY = Canvas->ClipY * 0.4f;
	Text(TEXT("하늘섬에 다시 바람이 분다"), CX, CY, 72.f, WithAlpha(Cream, A), EAlign::Center, 4.f, FLinearColor(0.35f, 0.2f, 0.12f, 0.9f));
	const int32 Minutes = FMath::FloorToInt(Adv.GetPlayTime() / 60.f);
	const int32 Seconds = FMath::FloorToInt(Adv.GetPlayTime()) % 60;
	const FString Stats = FString::Printf(TEXT("모험 시간 %d분 %02d초      별가루 %d / %d      보물 상자 %d / %d"), Minutes, Seconds, Adv.GetStardust(), Adv.GetStardustTotal(), Adv.GetChestsOpened(), Adv.GetChestTotal());
	Text(Stats, CX, CY + S(84.f), 28.f, WithAlpha(GoldText, A), EAlign::Center, 2.f);
	Text(TEXT("플레이해 주셔서 고마워요"), CX, CY + S(134.f), 24.f, WithAlpha(FLinearColor(0.95f, 0.95f, 1.f, 1.f), A), EAlign::Center, 2.f);
}

void AYJJHUD::DrawOverlays(const AYJJHero& Hero)
{
	// 다쳤을 때 붉은 테두리
	const float Hurt = Hero.GetHurtFlash();
	if (Hurt > 0.01f)
	{
		const float Edge = S(90.f);
		const FLinearColor Red(0.9f, 0.1f, 0.15f, 0.35f * Hurt);
		for (int32 i = 0; i < 6; ++i)
		{
			const float E = Edge * (1.f - i / 6.f);
			const FLinearColor C(Red.R, Red.G, Red.B, Red.A / 6.f);
			FCanvasTileItem T(FVector2D(0, 0), FVector2D(Canvas->ClipX, E), C);
			T.BlendMode = SE_BLEND_Translucent;
			Canvas->DrawItem(T);
			FCanvasTileItem B(FVector2D(0, Canvas->ClipY - E), FVector2D(Canvas->ClipX, E), C);
			B.BlendMode = SE_BLEND_Translucent;
			Canvas->DrawItem(B);
			FCanvasTileItem L(FVector2D(0, 0), FVector2D(E, Canvas->ClipY), C);
			L.BlendMode = SE_BLEND_Translucent;
			Canvas->DrawItem(L);
			FCanvasTileItem R(FVector2D(Canvas->ClipX - E, 0), FVector2D(E, Canvas->ClipY), C);
			R.BlendMode = SE_BLEND_Translucent;
			Canvas->DrawItem(R);
		}
	}

	// 암전 (낙사·부활)
	const float Fade = Hero.GetScreenFade();
	if (Fade > 0.01f)
	{
		FCanvasTileItem Black(FVector2D(0, 0), FVector2D(Canvas->ClipX, Canvas->ClipY), FLinearColor(0.02f, 0.03f, 0.08f, Fade));
		Black.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Black);
	}
}
