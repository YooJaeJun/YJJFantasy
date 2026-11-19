#pragma once

#include "CoreMinimal.h"

/**
 * HUD 배치·크기·타이밍. 크기는 1080p 기준 픽셀 (AYJJHUD::S 가 화면 높이에 맞춰 늘이고 줄인다),
 * 글자 크기는 포인트, 시간은 초. 색은 Content/YJJFantasy/Data/Colors.csv 의 Hud* 줄.
 */
namespace YJJHudLayout
{
	/** 기준 화면 높이와 배율 범위 */
	constexpr float ReferenceHeight = 1080.f;
	constexpr float ScaleMin = 0.5f;
	constexpr float ScaleMax = 2.f;

	/** 글자 */
	namespace Font
	{
		constexpr int32 MinSize = 6;
		constexpr float InvisibleAlpha = 0.01f;
		/** 글꼴 측정 서비스가 없을 때 한 글자 폭 추정 */
		constexpr float FallbackCharWidth = 0.6f;
	}

	/** 둥근 모서리 패널 */
	namespace Panel
	{
		constexpr float CornerRadius = 12.f;
		constexpr int32 CornerSteps = 5;
		constexpr float QuarterTurnDeg = 90.f;
	}

	namespace Shape
	{
		constexpr int32 DiamondSides = 4;
		constexpr int32 SmoothSides = 16;
		constexpr int32 DotSides = 8;
		/** 하트 = 원 두 개 + 아래 삼각형 */
		constexpr float HeartLobeUp = 0.15f;
		constexpr float HeartLobeRadius = 0.56f;
		constexpr float HeartWide = 1.03f;
		constexpr float HeartPoint = 1.05f;
	}

	/** 왼쪽 위 목표 패널 + 등불 점 + 별가루·상자 수 */
	namespace Objective
	{
		constexpr float Margin = 36.f;
		constexpr float TextSize = 26.f;
		constexpr float PanelPad = 74.f;
		constexpr float PanelHeight = 52.f;
		constexpr float PanelAlpha = 0.45f;
		constexpr float IconX = 28.f;
		constexpr float IconY = 26.f;
		constexpr float IconOuter = 11.f;
		constexpr float IconInner = 5.f;
		constexpr float IconInnerAlpha = 0.9f;
		constexpr float TextX = 50.f;
		constexpr float DotSpacing = 26.f;
		constexpr float DotRowY = 76.f;
		constexpr float DotBack = 9.f;
		constexpr float DotBackAlpha = 0.35f;
		constexpr float Dot = 6.5f;
		constexpr float DotOffAlpha = 0.25f;
		constexpr float DustRowY = 112.f;
		constexpr float DustGlow = 10.f;
		constexpr float DustIcon = 8.f;
		constexpr float CountX = 48.f;
		constexpr float CountSize = 22.f;
		constexpr float ChestRowGap = 34.f;
		constexpr float ChestX = 18.f;
		constexpr float ChestWidth = 20.f;
		constexpr float ChestHeight = 18.f;
		constexpr float LidHeight = 6.f;
		constexpr float LidUp = 1.f;
	}

	/** 오른쪽 위 하트 */
	namespace Hearts
	{
		constexpr float Radius = 15.f;
		constexpr float Gap = 40.f;
		constexpr float RightMargin = 40.f;
		constexpr float Y = 58.f;
		constexpr float BackScale = 1.22f;
		constexpr float BackAlpha = 0.4f;
		constexpr float HurtBounce = 0.3f;
		constexpr float EmptyAlpha = 0.18f;
		inline const FVector2D ShineSpot = FVector2D(-0.45f, -0.35f);
		constexpr float ShineRadius = 0.2f;
		constexpr float ShineAlpha = 0.7f;
	}

	/** 주인공 옆 세로 원호 기력 */
	namespace Stamina
	{
		constexpr float FollowSpeed = 10.f;
		constexpr float ShowSpeed = 10.f;
		constexpr float HideSpeed = 2.f;
		constexpr float FullAt = 0.999f;
		constexpr float HiddenBelow = 0.02f;
		constexpr float AnchorHeight = 30.f;
		constexpr float SideOffset = 70.f;
		constexpr float Radius = 46.f;
		constexpr int32 Dots = 26;
		constexpr float WarnBelow = 0.25f;
		constexpr float BlinkBase = 0.6f;
		constexpr float BlinkRange = 0.4f;
		constexpr float BlinkSpeed = 14.f;
		/** 원호가 걸치는 각 (도): -Arc/2 ~ +Arc/2 */
		constexpr float ArcDeg = 140.f;
		constexpr float Squeeze = 0.45f;
		constexpr float DotBack = 4.6f;
		constexpr float DotBackAlpha = 0.45f;
		constexpr float Dot = 3.4f;
	}

	/** 다음 목표 표시 (화면 안 마름모 / 화면 밖 가장자리 화살표) */
	namespace Marker
	{
		constexpr float TargetLift = 520.f;
		constexpr float CmPerMeter = 100.f;
		constexpr float HideWithinMeters = 6.f;
		constexpr float EdgeMargin = 60.f;
		constexpr float BobSpeed = 3.f;
		constexpr float Bob = 4.f;
		constexpr float Back = 16.f;
		constexpr float BackAlpha = 0.35f;
		constexpr float Outer = 12.f;
		constexpr float Inner = 5.f;
		constexpr float LabelGap = 28.f;
		constexpr float LabelSize = 18.f;
		constexpr float MinAxis = 0.001f;
		constexpr float ArrowTip = 20.f;
		constexpr float ArrowBack = 8.f;
		constexpr float ArrowHalfWidth = 14.f;
		constexpr float ArrowLabelGap = 34.f;
		constexpr float ArrowLabelSize = 17.f;
	}

	/** 화면 가운데 오른쪽 E 안내 */
	namespace Prompt
	{
		constexpr float OffsetX = 140.f;
		constexpr float OffsetY = 40.f;
		constexpr float TextSize = 24.f;
		constexpr float PanelHalfHeight = 24.f;
		constexpr float PanelPad = 80.f;
		constexpr float PanelAlpha = 0.6f;
		constexpr float KeyX = 8.f;
		constexpr float KeyHalf = 17.f;
		constexpr float KeyLabelX = 25.f;
		constexpr float KeyLabelSize = 22.f;
		constexpr float TextX = 56.f;
	}

	/** 섬에 들어가면 뜨는 이름 카드 (전체 길이는 Tuning.csv Adventure.ZoneCardDuration) */
	namespace ZoneCard
	{
		constexpr float FadeIn = 0.6f;
		constexpr float FadeOut = 1.4f;
		constexpr float LineGrowTime = 0.9f;
		constexpr float HeightRatio = 0.24f;
		constexpr float LineHalfWidth = 260.f;
		constexpr float LineY = 42.f;
		constexpr float LineThickness = 2.f;
		constexpr float LineAlpha = 0.8f;
		constexpr float GemY = 43.f;
		constexpr float SideGem = 6.f;
		constexpr float CenterGem = 8.f;
		constexpr float NameUp = 4.f;
		constexpr float NameSize = 64.f;
		constexpr float NameOutline = 3.f;
		constexpr float SubY = 78.f;
		constexpr float SubSize = 24.f;
		constexpr float SubOutline = 2.f;
	}

	/** 아래쪽 알림 */
	namespace Toasts
	{
		constexpr float BottomOffset = 150.f;
		constexpr float FadeIn = 0.25f;
		constexpr float FadeOut = 0.6f;
		constexpr float SlideTime = 0.3f;
		constexpr float SlideDistance = 20.f;
		constexpr float TextSize = 26.f;
		constexpr float PadX = 30.f;
		constexpr float HalfHeight = 24.f;
		constexpr float PanelAlpha = 0.5f;
		constexpr float LineSpacing = 58.f;
	}

	/** 시작 제목과 조작 안내 */
	namespace Intro
	{
		constexpr float Duration = 14.f;
		constexpr float TitleIn = 0.5f;
		constexpr float TitleInTime = 1.2f;
		constexpr float TitleOut = 6.5f;
		constexpr float TitleOutTime = 1.5f;
		constexpr float HeightRatio = 0.36f;
		constexpr float TitleSize = 92.f;
		constexpr float TitleOutline = 4.f;
		constexpr float SubY = 84.f;
		constexpr float SubSize = 28.f;
		constexpr float SubOutline = 2.f;
		constexpr float HintIn = 2.f;
		constexpr float HintInTime = 1.f;
		constexpr float HintOutTime = 1.5f;
		constexpr float HintSize = 21.f;
		constexpr float HintPadX = 24.f;
		constexpr float HintPanelBottom = 70.f;
		constexpr float HintPanelHeight = 40.f;
		constexpr float HintPanelAlpha = 0.45f;
		constexpr float HintTextBottom = 50.f;
		constexpr float HintOutline = 1.5f;
	}

	/** 엔딩 영화 띠 + 기록 카드 (시간은 엔딩 시작부터) */
	namespace Ending
	{
		constexpr float BarIn = 1.5f;
		constexpr float End = 24.f;
		constexpr float FadeOut = 2.f;
		constexpr float BarHeight = 90.f;
		constexpr float BarVisible = 0.5f;
		constexpr float CardIn = 7.f;
		constexpr float CardInTime = 1.5f;
		constexpr float HeightRatio = 0.4f;
		constexpr float TitleSize = 72.f;
		constexpr float TitleOutline = 4.f;
		constexpr float StatsY = 84.f;
		constexpr float StatsSize = 28.f;
		constexpr float ThanksY = 134.f;
		constexpr float ThanksSize = 24.f;
		constexpr float LineOutline = 2.f;
		constexpr float SecondsPerMinute = 60.f;
	}

	/** 다쳤을 때 붉은 테두리, 낙사 암전 */
	namespace Overlay
	{
		constexpr float VisibleAbove = 0.01f;
		constexpr float HurtEdge = 90.f;
		constexpr float HurtAlpha = 0.35f;
		constexpr int32 HurtLayers = 6;
	}
}
