#include "UI/YJJHUD.h"

#include "Data/YJJLoc.h"
#include "UI/YJJHudLayout.h"
#include "Data/YJJData.h"
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

UFont* AYJJHUD::FallbackFont = nullptr;

const UFont* AYJJHUD::BaseFont()
{
	if (!FallbackFont)
	{
		FallbackFont = LoadObject<UFont>(nullptr, YJJAssetPaths::FallbackFont);
		if (FallbackFont)
		{
			FallbackFont->AddToRoot();
		}
	}
	return FallbackFont;
}

FLinearColor AYJJHUD::WithAlpha(FLinearColor C, float A)
{
	C.A *= A;
	return C;
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
	if (!Base || Str.IsEmpty() || Color.A <= YJJHudLayout::Font::InvisibleAlpha)
	{
		return;
	}
	const int32 FontSize = FMath::Max(FMath::RoundToInt(S(Size)), YJJHudLayout::Font::MinSize);
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
	const int32 FontSize = FMath::Max(FMath::RoundToInt(S(Size)), YJJHudLayout::Font::MinSize);
	FSlateFontInfo Font = Composite.IsValid() ? FSlateFontInfo(Composite, FontSize) : FSlateFontInfo(Base, FontSize);
	if (FSlateApplication::IsInitialized() && FSlateApplication::Get().GetRenderer())
	{
		return FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Str, Font);
	}
	return FVector2D(Str.Len() * S(Size) * YJJHudLayout::Font::FallbackCharWidth, S(Size));
}

void AYJJHUD::Panel(float X, float Y, float W, float H, const FLinearColor& Color)
{
	namespace P = YJJHudLayout::Panel;
	// 둥근 모서리 반투명 패널: 겹쳐 그리면 반투명이 진해지므로 테두리 다각형 하나를 부채꼴로 채운다
	const float C = FMath::Min(S(P::CornerRadius), H * 0.5f);
	TArray<FVector2D> Outline;
	const FVector2D Corners[] = { FVector2D(X + W - C, Y + H - C), FVector2D(X + C, Y + H - C), FVector2D(X + C, Y + C), FVector2D(X + W - C, Y + C) };
	for (int32 c = 0; c < UE_ARRAY_COUNT(Corners); ++c)
	{
		for (int32 k = 0; k <= P::CornerSteps; ++k)
		{
			const float A = FMath::DegreesToRadians(c * P::QuarterTurnDeg + k * (P::QuarterTurnDeg / P::CornerSteps));
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
	Circle(X, Y, R, Color, YJJHudLayout::Shape::DiamondSides);
}

void AYJJHUD::Heart(float X, float Y, float R, const FLinearColor& Color)
{
	namespace Sh = YJJHudLayout::Shape;
	// 원 두 개 + 아래 삼각형
	Circle(X - R * 0.5f, Y - R * Sh::HeartLobeUp, R * Sh::HeartLobeRadius, Color, Sh::SmoothSides);
	Circle(X + R * 0.5f, Y - R * Sh::HeartLobeUp, R * Sh::HeartLobeRadius, Color, Sh::SmoothSides);
	TArray<FCanvasUVTri> Tris;
	FCanvasUVTri& T = Tris.AddDefaulted_GetRef();
	T.V0_Pos = FVector2D(X - R * Sh::HeartWide, Y);
	T.V1_Pos = FVector2D(X + R * Sh::HeartWide, Y);
	T.V2_Pos = FVector2D(X, Y + R * Sh::HeartPoint);
	T.V0_Color = T.V1_Color = T.V2_Color = Color;
	FCanvasTriangleItem Item(Tris, GWhiteTexture);
	Item.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Item);
}

FLinearColor AYJJHUD::Hud(const TCHAR* Name, float Alpha)
{
	return WithAlpha(FYJJData::Color(Name), Alpha);
}

// ============================================================

void AYJJHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas)
	{
		return;
	}
	UIScale = FMath::Clamp(Canvas->ClipY / YJJHudLayout::ReferenceHeight, YJJHudLayout::ScaleMin, YJJHudLayout::ScaleMax);

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
	namespace O = YJJHudLayout::Objective;
	const FString Goal = Adv.GetObjectiveText();
	const float X = S(O::Margin);
	const float Y = S(O::Margin);
	const FVector2D Size = Measure(Goal, O::TextSize);
	Panel(X, Y, Size.X + S(O::PanelPad), S(O::PanelHeight), Hud(TEXT("HudPanel"), O::PanelAlpha));
	Diamond(X + S(O::IconX), Y + S(O::IconY), S(O::IconOuter), FYJJData::Color(TEXT("HudGoldText")));
	Diamond(X + S(O::IconX), Y + S(O::IconY), S(O::IconInner), Hud(TEXT("HudWhite"), O::IconInnerAlpha));
	Text(Goal, X + S(O::TextX), Y + S(O::IconY), O::TextSize, FYJJData::Color(TEXT("HudCream")));

	// 등불 진행 점
	const int32 Count = Adv.GetLanternCount();
	const int32 Lit = Adv.GetLanternsLit();
	for (int32 i = 0; i < Count; ++i)
	{
		const float CX = X + S(O::IconX + i * O::DotSpacing);
		const float CY = Y + S(O::DotRowY);
		Circle(CX, CY, S(O::DotBack), Hud(TEXT("HudShadow"), O::DotBackAlpha));
		Circle(CX, CY, S(O::Dot), i < Lit ? FYJJData::Color(TEXT("HudGoldText")) : Hud(TEXT("HudWhite"), O::DotOffAlpha));
	}

	// 별가루
	const FString Dust = FString::Printf(TEXT("%d / %d"), Adv.GetStardust(), Adv.GetStardustTotal());
	const float DY = Y + S(O::DustRowY);
	Circle(X + S(O::IconX), DY, S(O::DustGlow), FYJJData::Color(TEXT("HudDustGlow")));
	Diamond(X + S(O::IconX), DY, S(O::DustIcon), FYJJData::Color(TEXT("HudDustIcon")));
	Text(Dust, X + S(O::CountX), DY, O::CountSize, FYJJData::Color(TEXT("HudDustText")));

	// 보물 상자
	if (Adv.GetChestTotal() > 0)
	{
		const float CY = DY + S(O::ChestRowGap);
		Panel(X + S(O::ChestX), CY - S(O::ChestHeight * 0.5f), S(O::ChestWidth), S(O::ChestHeight), FYJJData::Color(TEXT("HudChestBox")));
		Panel(X + S(O::ChestX), CY - S(O::ChestHeight * 0.5f + O::LidUp), S(O::ChestWidth), S(O::LidHeight), FYJJData::Color(TEXT("HudChestLid")));
		Text(FString::Printf(TEXT("%d / %d"), Adv.GetChestsOpened(), Adv.GetChestTotal()), X + S(O::CountX), CY, O::CountSize, FYJJData::Color(TEXT("HudChestText")));
	}
}

void AYJJHUD::DrawHearts(const AYJJHero& Hero)
{
	namespace Hh = YJJHudLayout::Hearts;
	const float R = S(Hh::Radius);
	const float Gap = S(Hh::Gap);
	const int32 Max = Hero.GetMaxHealth();
	const float X0 = Canvas->ClipX - S(Hh::RightMargin) - (Max - 1) * Gap;
	const float Y = S(Hh::Y);
	const float Pulse = Hero.GetHurtFlash();
	for (int32 i = 0; i < Max; ++i)
	{
		const float X = X0 + i * Gap;
		Heart(X, Y, R * Hh::BackScale, Hud(TEXT("HudShadow"), Hh::BackAlpha));
		const bool bFull = i < Hero.GetHealth();
		const float Bounce = (bFull && i == Hero.GetHealth() - 1) ? Pulse * Hh::HurtBounce : 0.f;
		Heart(X, Y, R * (1.f + Bounce), bFull ? FYJJData::Color(TEXT("HudHeart")) : Hud(TEXT("HudWhite"), Hh::EmptyAlpha));
		if (bFull)
		{
			Circle(X + R * Hh::ShineSpot.X, Y + R * Hh::ShineSpot.Y, R * Hh::ShineRadius, Hud(TEXT("HudWhite"), Hh::ShineAlpha), YJJHudLayout::Shape::DotSides);
		}
	}
}

void AYJJHUD::DrawStamina(const AYJJHero& Hero)
{
	namespace St = YJJHudLayout::Stamina;
	const float Max = Hero.GetMaxStamina();
	const float Frac = Max > 0.f ? Hero.GetStamina() / Max : 1.f;
	const float Dt = GetWorld()->GetDeltaSeconds();
	const bool bFull = Frac >= St::FullAt;
	DisplayStamina = FMath::FInterpTo(DisplayStamina, Frac, Dt, St::FollowSpeed);
	StaminaAlpha = FMath::FInterpTo(StaminaAlpha, bFull ? 0.f : 1.f, Dt, bFull ? St::HideSpeed : St::ShowSpeed);
	if (StaminaAlpha < St::HiddenBelow)
	{
		return;
	}

	// 영웅 오른쪽 옆에 세로 원호
	FVector2D Screen;
	const APlayerController* PC = GetOwningPlayerController();
	if (!PC || !PC->ProjectWorldLocationToScreen(Hero.GetActorLocation() + FVector(0, 0, St::AnchorHeight), Screen))
	{
		return;
	}
	const float CX = Screen.X + S(St::SideOffset);
	const float CY = Screen.Y;
	const float R = S(St::Radius);
	const bool bWarn = Hero.IsExhausted() || Frac < St::WarnBelow;
	const float Blink = bWarn ? St::BlinkBase + St::BlinkRange * FMath::Sin(GetWorld()->GetTimeSeconds() * St::BlinkSpeed) : 1.f;
	const FLinearColor Fill = bWarn ? Hud(TEXT("HudStaminaWarn"), StaminaAlpha * Blink) : Hud(TEXT("HudStamina"), StaminaAlpha);
	const FLinearColor Back = Hud(TEXT("HudShadow"), St::DotBackAlpha * StaminaAlpha);
	for (int32 i = 0; i < St::Dots; ++i)
	{
		const float T = i / static_cast<float>(St::Dots - 1);
		const float A = FMath::DegreesToRadians(-St::ArcDeg * 0.5f + St::ArcDeg * (1.f - T));
		const float X = CX + FMath::Cos(A) * R * St::Squeeze;
		const float Y = CY - FMath::Sin(A) * R;
		Circle(X, Y, S(St::DotBack), Back, YJJHudLayout::Shape::DotSides);
		if (T <= DisplayStamina)
		{
			Circle(X, Y, S(St::Dot), Fill, YJJHudLayout::Shape::DotSides);
		}
	}
}

void AYJJHUD::DrawMarker(const AYJJHero& Hero, const UYJJAdventureSubsystem& Adv)
{
	namespace M = YJJHudLayout::Marker;
	FVector Target;
	if (!Adv.GetObjectiveLocation(Hero.GetActorLocation(), Target))
	{
		return;
	}
	Target += FVector(0, 0, M::TargetLift);
	const APlayerController* PC = GetOwningPlayerController();
	FVector CamLoc;
	FRotator CamRot;
	PC->GetPlayerViewPoint(CamLoc, CamRot);
	const float Dist = FVector::Dist(Hero.GetActorLocation(), Target) / M::CmPerMeter;
	if (Dist < M::HideWithinMeters)
	{
		return;
	}

	FVector2D Screen;
	const bool bInFront = FVector::DotProduct(CamRot.Vector(), (Target - CamLoc).GetSafeNormal()) > 0.f;
	const bool bProjected = PC->ProjectWorldLocationToScreen(Target, Screen);
	const float Margin = S(M::EdgeMargin);
	const float Time = GetWorld()->GetTimeSeconds();
	const float Bob = FMath::Sin(Time * M::BobSpeed) * S(M::Bob);
	const FString Label = FString::Printf(TEXT("%.0fm"), Dist);

	if (bInFront && bProjected && Screen.X > Margin && Screen.X < Canvas->ClipX - Margin && Screen.Y > Margin && Screen.Y < Canvas->ClipY - Margin)
	{
		Diamond(Screen.X, Screen.Y + Bob, S(M::Back), Hud(TEXT("HudShadow"), M::BackAlpha));
		Diamond(Screen.X, Screen.Y + Bob, S(M::Outer), FYJJData::Color(TEXT("HudGoldText")));
		Diamond(Screen.X, Screen.Y + Bob, S(M::Inner), FLinearColor::White);
		Text(Label, Screen.X, Screen.Y + Bob + S(M::LabelGap), M::LabelSize, FYJJData::Color(TEXT("HudCream")), EAlign::Center);
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
	const float Scale = FMath::Min((Canvas->ClipX * 0.5f - Margin) / FMath::Max(FMath::Abs(Dir.X), M::MinAxis), (Canvas->ClipY * 0.5f - Margin) / FMath::Max(FMath::Abs(Dir.Y), M::MinAxis));
	const FVector2D P = Center + Dir * Scale;
	TArray<FCanvasUVTri> Tris;
	FCanvasUVTri& T = Tris.AddDefaulted_GetRef();
	const FVector2D Perp(-Dir.Y, Dir.X);
	T.V0_Pos = P + Dir * S(M::ArrowTip);
	T.V1_Pos = P - Dir * S(M::ArrowBack) + Perp * S(M::ArrowHalfWidth);
	T.V2_Pos = P - Dir * S(M::ArrowBack) - Perp * S(M::ArrowHalfWidth);
	T.V0_Color = T.V1_Color = T.V2_Color = FYJJData::Color(TEXT("HudGoldText"));
	FCanvasTriangleItem Item(Tris, GWhiteTexture);
	Item.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Item);
	Text(Label, P.X - Dir.X * S(M::ArrowLabelGap), P.Y - Dir.Y * S(M::ArrowLabelGap), M::ArrowLabelSize, FYJJData::Color(TEXT("HudCream")), EAlign::Center);
}

void AYJJHUD::DrawPrompt(const AYJJHero& Hero)
{
	namespace Pr = YJJHudLayout::Prompt;
	const FString Prompt = Hero.GetInteractPrompt();
	if (Prompt.IsEmpty())
	{
		return;
	}
	const float X = Canvas->ClipX * 0.5f + S(Pr::OffsetX);
	const float Y = Canvas->ClipY * 0.5f + S(Pr::OffsetY);
	const FVector2D Size = Measure(Prompt, Pr::TextSize);
	Panel(X, Y - S(Pr::PanelHalfHeight), Size.X + S(Pr::PanelPad), S(Pr::PanelHalfHeight * 2.f), Hud(TEXT("HudPanel"), Pr::PanelAlpha));
	Panel(X + S(Pr::KeyX), Y - S(Pr::KeyHalf), S(Pr::KeyHalf * 2.f), S(Pr::KeyHalf * 2.f), FYJJData::Color(TEXT("HudKeyCap")));
	Text(TEXT("E"), X + S(Pr::KeyLabelX), Y, Pr::KeyLabelSize, FYJJData::Color(TEXT("HudInk")), EAlign::Center, 0.f);
	Text(Prompt, X + S(Pr::TextX), Y, Pr::TextSize, FYJJData::Color(TEXT("HudCream")));
}

void AYJJHUD::DrawZoneCard(const UYJJAdventureSubsystem& Adv)
{
	namespace Z = YJJHudLayout::ZoneCard;
	float Age;
	const FYJJZone* Zone = Adv.GetZoneCard(Age);
	if (!Zone)
	{
		return;
	}
	const float Duration = FYJJData::Tune(TEXT("Adventure.ZoneCardDuration"));
	const float A = FMath::Clamp(Age / Z::FadeIn, 0.f, 1.f) * FMath::Clamp((Duration - Age) / Z::FadeOut, 0.f, 1.f);
	const float Ease = 1.f - FMath::Pow(1.f - FMath::Clamp(Age / Z::LineGrowTime, 0.f, 1.f), 3.f);
	const float CX = Canvas->ClipX * 0.5f;
	const float CY = Canvas->ClipY * Z::HeightRatio;
	const FLinearColor Gold = FYJJData::Color(TEXT("HudGoldText"));

	const float LineW = S(Z::LineHalfWidth) * Ease;
	FCanvasTileItem Line(FVector2D(CX - LineW, CY + S(Z::LineY)), FVector2D(LineW * 2.f, S(Z::LineThickness)), WithAlpha(Gold, A * Z::LineAlpha));
	Line.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Line);
	Diamond(CX - LineW, CY + S(Z::GemY), S(Z::SideGem), WithAlpha(Gold, A));
	Diamond(CX + LineW, CY + S(Z::GemY), S(Z::SideGem), WithAlpha(Gold, A));
	Diamond(CX, CY + S(Z::GemY), S(Z::CenterGem), WithAlpha(FLinearColor::White, A));

	Text(Zone->Name, CX, CY - S(Z::NameUp), Z::NameSize, WithAlpha(FYJJData::Color(TEXT("HudCream")), A), EAlign::Center, Z::NameOutline, FYJJData::Color(TEXT("HudZoneOutline")));
	Text(Zone->Subtitle, CX, CY + S(Z::SubY), Z::SubSize, WithAlpha(FYJJData::Color(TEXT("HudSubtitle")), A), EAlign::Center, Z::SubOutline);
}

void AYJJHUD::DrawToasts(const UYJJAdventureSubsystem& Adv)
{
	namespace Ts = YJJHudLayout::Toasts;
	const TArray<UYJJAdventureSubsystem::FToast>& Toasts = Adv.GetToasts();
	float Y = Canvas->ClipY - S(Ts::BottomOffset);
	for (int32 i = Toasts.Num() - 1; i >= 0; --i)
	{
		const UYJJAdventureSubsystem::FToast& T = Toasts[i];
		const float A = FMath::Clamp(T.Age / Ts::FadeIn, 0.f, 1.f) * FMath::Clamp((T.Duration - T.Age) / Ts::FadeOut, 0.f, 1.f);
		const float Slide = (1.f - FMath::Clamp(T.Age / Ts::SlideTime, 0.f, 1.f)) * S(Ts::SlideDistance);
		const FVector2D Size = Measure(T.Text, Ts::TextSize);
		Panel(Canvas->ClipX * 0.5f - Size.X * 0.5f - S(Ts::PadX), Y - S(Ts::HalfHeight) + Slide, Size.X + S(Ts::PadX * 2.f), S(Ts::HalfHeight * 2.f), Hud(TEXT("HudPanel"), Ts::PanelAlpha * A));
		Text(T.Text, Canvas->ClipX * 0.5f, Y + Slide, Ts::TextSize, WithAlpha(T.Color, A), EAlign::Center);
		Y -= S(Ts::LineSpacing);
	}
}

void AYJJHUD::DrawIntro(const UYJJAdventureSubsystem& Adv)
{
	namespace In = YJJHudLayout::Intro;
	// 시작 직후 제목과 조작 안내 (튜토리얼 대신 한 줄)
	const float T = Adv.GetPlayTime();
	if (T > In::Duration)
	{
		return;
	}
	const float TitleA = FMath::Clamp((T - In::TitleIn) / In::TitleInTime, 0.f, 1.f) * FMath::Clamp((In::TitleOut - T) / In::TitleOutTime, 0.f, 1.f);
	const float CX = Canvas->ClipX * 0.5f;
	const float CY = Canvas->ClipY * In::HeightRatio;
	if (TitleA > 0.f)
	{
		Text(FYJJLoc::Get(TEXT("Title.Name")), CX, CY, In::TitleSize, WithAlpha(FYJJData::Color(TEXT("HudCream")), TitleA), EAlign::Center, In::TitleOutline, FYJJData::Color(TEXT("HudTitleOutline")));
		Text(FYJJLoc::Get(TEXT("Title.Sub")), CX, CY + S(In::SubY), In::SubSize, WithAlpha(FYJJData::Color(TEXT("HudTitleSub")), TitleA), EAlign::Center, In::SubOutline);
	}
	const float HintA = FMath::Clamp((T - In::HintIn) / In::HintInTime, 0.f, 1.f) * FMath::Clamp((In::Duration - T) / In::HintOutTime, 0.f, 1.f);
	if (HintA > 0.f)
	{
		const FString Hint = FYJJLoc::Get(TEXT("Title.Controls"));
		const FVector2D Size = Measure(Hint, In::HintSize);
		Panel(CX - Size.X * 0.5f - S(In::HintPadX), Canvas->ClipY - S(In::HintPanelBottom), Size.X + S(In::HintPadX * 2.f), S(In::HintPanelHeight), Hud(TEXT("HudPanel"), In::HintPanelAlpha * HintA));
		Text(Hint, CX, Canvas->ClipY - S(In::HintTextBottom), In::HintSize, WithAlpha(FYJJData::Color(TEXT("HudCream")), HintA), EAlign::Center, In::HintOutline);
	}
}

void AYJJHUD::DrawEnding(const UYJJAdventureSubsystem& Adv)
{
	namespace E = YJJHudLayout::Ending;
	const bool bEnded = Adv.GetPhase() == EYJJPhase::Ended;
	// 엔딩 시작부터 잰 시간 (연출 단계가 끝나면 기록 카드 단계로 이어진다)
	const float T = bEnded ? Adv.GetPhaseTime() + FYJJData::Tune(TEXT("Adventure.FinaleDuration")) : Adv.GetPhaseTime();
	const float CX = Canvas->ClipX * 0.5f;
	const float Out = FMath::Clamp((E::End - T) / E::FadeOut, 0.f, 1.f);

	// 영화 같은 위아래 검은 띠
	const float Bar = FMath::Clamp(T / E::BarIn, 0.f, 1.f) * Out * S(E::BarHeight);
	if (Bar > E::BarVisible)
	{
		const FLinearColor Black = FYJJData::Color(TEXT("HudLetterbox"));
		FCanvasTileItem Top(FVector2D(0, 0), FVector2D(Canvas->ClipX, Bar), Black);
		Canvas->DrawItem(Top);
		FCanvasTileItem Bottom(FVector2D(0, Canvas->ClipY - Bar), FVector2D(Canvas->ClipX, Bar), Black);
		Canvas->DrawItem(Bottom);
	}

	const float A = FMath::Clamp((T - E::CardIn) / E::CardInTime, 0.f, 1.f) * Out;
	if (A <= 0.f)
	{
		return;
	}
	const float CY = Canvas->ClipY * E::HeightRatio;
	Text(FYJJLoc::Get(TEXT("Ending.Title")), CX, CY, E::TitleSize, WithAlpha(FYJJData::Color(TEXT("HudCream")), A), EAlign::Center, E::TitleOutline, FYJJData::Color(TEXT("HudEndingOutline")));
	const int32 Minutes = FMath::FloorToInt(Adv.GetPlayTime() / E::SecondsPerMinute);
	const int32 Seconds = FMath::FloorToInt(FMath::Fmod(Adv.GetPlayTime(), E::SecondsPerMinute));
	const FString Stats = FYJJLoc::Format(TEXT("Ending.Stats"), { Minutes, FString::Printf(TEXT("%02d"), Seconds), Adv.GetStardust(), Adv.GetStardustTotal(), Adv.GetChestsOpened(), Adv.GetChestTotal() });
	Text(Stats, CX, CY + S(E::StatsY), E::StatsSize, WithAlpha(FYJJData::Color(TEXT("HudGoldText")), A), EAlign::Center, E::LineOutline);
	Text(FYJJLoc::Get(TEXT("Ending.Thanks")), CX, CY + S(E::ThanksY), E::ThanksSize, WithAlpha(FYJJData::Color(TEXT("HudThanks")), A), EAlign::Center, E::LineOutline);
}

void AYJJHUD::DrawOverlays(const AYJJHero& Hero)
{
	namespace Ov = YJJHudLayout::Overlay;
	// 다쳤을 때 붉은 테두리 (안쪽으로 갈수록 옅게 여러 겹)
	const float Hurt = Hero.GetHurtFlash();
	if (Hurt > Ov::VisibleAbove)
	{
		const float Edge = S(Ov::HurtEdge);
		const FLinearColor C = Hud(TEXT("HudHurt"), Ov::HurtAlpha * Hurt / Ov::HurtLayers);
		for (int32 i = 0; i < Ov::HurtLayers; ++i)
		{
			const float E = Edge * (1.f - i / static_cast<float>(Ov::HurtLayers));
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
	if (Fade > Ov::VisibleAbove)
	{
		FCanvasTileItem Black(FVector2D(0, 0), FVector2D(Canvas->ClipX, Canvas->ClipY), Hud(TEXT("HudFade"), Fade));
		Black.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Black);
	}
}
