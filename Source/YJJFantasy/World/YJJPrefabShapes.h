#pragma once

#include "CoreMinimal.h"

/**
 * 월드 빌더 소품의 모양 비율. 배치(어디에 몇 개)는 Content/YJJFantasy/Data/Placements.csv,
 * 색은 Colors.csv, 여기는 "한 개를 어떤 모양으로 조립하나"만 담는다.
 * 대부분 소품 크기(반지름, 높이, Scale)에 곱하는 비율이고, 단위가 cm 인 것은 이름에 적었다.
 */
namespace YJJPrefab
{
	/** 결정적 난수 씨앗을 섞는 수 (바꾸면 배치가 통째로 달라진다) */
	namespace Hash
	{
		constexpr int32 IslandBlobAngle = 11;
		constexpr int32 IslandBlobDist = 13;
		constexpr int32 IslandBlobSize = 17;
		constexpr int32 IslandRimAngle = 19;
		constexpr int32 IslandRimSize = 23;
		constexpr int32 GrassAngle = 101;
		constexpr int32 GrassDist = 103;
		constexpr int32 GrassSize = 107;
		constexpr int32 FlowerSeed = 211;
		constexpr int32 FlowerColor = 977;
		constexpr int32 FlowerClusterSize = 6;
		constexpr int32 FlowerCluster = 7;
		constexpr int32 FlowerJitter = 3;
		constexpr int32 FlowerSize = 5;
		constexpr int32 PebbleAngle = 307;
		constexpr int32 PebbleDist = 311;
		constexpr int32 PebbleSize = 313;
		constexpr int32 TreeCrown = 3;
		constexpr int32 TreeBump = 11;
		constexpr int32 CrystalHeight = 5;
		constexpr int32 MoteOffset = 3;
		constexpr int32 MoteDrift = 5;
		constexpr int32 MotePhase = 7;
	}

	/** 케이크 섬: 잔디 크림 + 코코아색 흙층 + 둥글둥글한 바닥 */
	namespace Island
	{
		constexpr float TopThickness = 140.f;
		constexpr float GroundSideShade = 0.8f;
		constexpr float IcingTopShade = 1.08f;
		constexpr float IcingSideShade = 0.86f;
		constexpr float UnderSideShade = 0.88f;
		constexpr float DirtCenterZ = -215.f;
		constexpr float DirtThickness = 190.f;
		constexpr float DirtWidth = 0.985f;
		/** 바닥 공의 꼭대기를 흙층 안에 숨기는 높이 (cm). 윗면을 뚫지 않게 */
		constexpr float BottomHideZ = -150.f;
		constexpr float Bottom1Depth = 0.25f;
		constexpr float Bottom1Width = 0.96f;
		constexpr float Bottom2Depth = 0.36f;
		constexpr float Bottom2Width = 0.62f;
		constexpr float Bottom2Overlap = 0.9f;
		constexpr int32 BlobCount = 4;
		constexpr float BlobDistMin = 0.3f;
		constexpr float BlobDistRange = 0.25f;
		constexpr float BlobSizeMin = 0.28f;
		constexpr float BlobSizeRange = 0.14f;
		constexpr float BlobDrop = 1.5f;
		constexpr float BlobSink = 0.3f;
		constexpr float BlobFlat = 0.85f;
		/** 가장자리 잔디 크림 방울 하나가 덮는 둘레 (cm) */
		constexpr float RimSpacing = 110.f;
		constexpr int32 RimMin = 10;
		constexpr int32 RimMax = 40;
		constexpr float RimJitter = 0.5f;
		constexpr float RimSizeMin = 150.f;
		constexpr float RimSizeRange = 130.f;
		constexpr float RimEdge = 0.99f;
		constexpr float RimTopZ = -60.f;
		constexpr float RimSink = 0.4f;
		inline const FVector RimShape = FVector(1.3f, 1.1f, 0.75f);
		/** 꾸미기 기본 꽃 수 = 반지름 / 이 값 */
		constexpr float DefaultFlowerSpacing = 18.f;
	}

	/** 납작한 구를 반쯤 묻은 언덕: 높이 = 반지름 x 이 비율이라 경사 20도 이하로 걸어서 오른다 */
	namespace Hill
	{
		constexpr float Flatness = 0.25f;
	}

	/** 풀숲·꽃·돌멩이 */
	namespace Decor
	{
		/** 땅을 찾는 범위 (cm) */
		constexpr float GroundSearch = 1500.f;
		/** 풀숲 수 = 반지름² / 이 값 */
		constexpr float GrassArea = 26000.f;
		constexpr float GrassSpread = 0.92f;
		constexpr float GrassTopShadeA = 1.15f;
		constexpr float GrassSideShadeA = 0.85f;
		constexpr float GrassTopShadeB = 1.3f;
		constexpr float GrassSizeMin = 0.6f;
		constexpr float GrassSizeRange = 0.8f;
		constexpr int32 GrassBlobs = 3;
		constexpr float GrassBlobTurn = 2.1f;
		constexpr float GrassSideBlob = 0.7f;
		constexpr float GrassBlobOffset = 26.f;
		constexpr float GrassBlobLift = 6.f;
		inline const FVector GrassBlobShape = FVector(0.55f, 0.48f, 0.36f);
		constexpr float FlowerEmissive = 0.25f;
		constexpr float FlowerRoughness = 0.6f;
		constexpr float FlowerSpread = 0.85f;
		constexpr float FlowerClusterSize = 260.f;
		constexpr float FlowerSizeMin = 0.7f;
		constexpr float FlowerSizeRange = 0.6f;
		constexpr float FlowerLift = 16.f;
		inline const FVector FlowerShape = FVector(0.2f, 0.2f, 0.12f);
		/** 돌멩이 수 = 반지름 / 이 값 */
		constexpr float PebbleSpacing = 60.f;
		constexpr float PebbleSpread = 0.9f;
		constexpr float PebbleSizeMin = 0.3f;
		constexpr float PebbleSizeRange = 0.5f;
		constexpr float PebbleLift = 5.f;
		inline const FVector PebbleShape = FVector(0.6f, 0.45f, 0.25f);
	}

	/** 막대사탕 나무 (Scale 1 기준 cm) */
	namespace Tree
	{
		constexpr float TrunkWidth = 50.f;
		constexpr float TrunkHeight = 340.f;
		inline const FVector RootFlare = FVector(90.f, 90.f, 40.f);
		constexpr float RootFlareZ = 10.f;
		constexpr float CrownMin = 360.f;
		constexpr float CrownRange = 60.f;
		constexpr float CrownZ = 440.f;
		constexpr float CrownFlat = 0.9f;
		constexpr float LeafTopShade = 1.12f;
		constexpr float LeafSideShade = 0.8f;
		constexpr float LeafEmissive = 0.04f;
		constexpr float LeafRoughness = 0.9f;
		constexpr int32 Bumps = 2;
		constexpr float BumpTurn = 2.6f;
		constexpr float BumpMin = 0.42f;
		constexpr float BumpRange = 0.1f;
		constexpr float BumpOut = 0.42f;
		constexpr float BumpUpMin = 0.12f;
		constexpr float BumpUpStep = 0.15f;
		/** 이 수마다 하나는 얼굴 */
		constexpr int32 FaceEvery = 3;
		constexpr float FaceOut = 0.49f;
		constexpr float FaceDown = 0.05f;
		constexpr float FaceWidth = 0.38f;
	}

	/** 소프트아이스크림 같은 둥근 층 나무 (Scale 1 기준 cm) */
	namespace Pine
	{
		constexpr float TrunkWidth = 44.f;
		constexpr float TrunkHeight = 180.f;
		constexpr int32 Tiers = 3;
		constexpr float TierWidth = 300.f;
		constexpr float TierShrink = 80.f;
		constexpr float TierZ = 230.f;
		constexpr float TierStep = 120.f;
		constexpr float TierFlat = 0.55f;
		constexpr float TipSize = 70.f;
		constexpr float TipZ = 590.f;
		constexpr float LeafTopShade = 1.3f;
		constexpr float LeafSideShade = 0.95f;
	}

	/** 둥근 조약돌 바위 (Size 에 곱함) */
	namespace Rock
	{
		constexpr float TopShade = 1.25f;
		constexpr float MainZ = 0.3f;
		inline const FVector MainShape = FVector(1.3f, 1.f, 0.9f);
		inline const FVector SideOffset = FVector(-0.45f, 0.35f, 0.1f);
		inline const FVector SideShape = FVector(0.6f, 0.55f, 0.45f);
		constexpr int32 FaceEvery = 2;
		inline const FVector FaceSpot = FVector(0.62f, 0.f, 0.38f);
		constexpr float FaceWidth = 0.55f;
		constexpr int32 FaceMood = 1;
	}

	/** 사탕 수정 무리 (Size = 가장 큰 수정 높이) */
	namespace Crystal
	{
		constexpr float TopShade = 1.35f;
		constexpr float SideShade = 1.05f;
		constexpr float Emissive = 0.45f;
		constexpr float Roughness = 0.4f;
		constexpr int32 CountMin = 3;
		constexpr int32 CountVariety = 3;
		constexpr float SideHeightMin = 0.45f;
		constexpr float SideHeightRange = 0.4f;
		constexpr float Width = 0.48f;
		constexpr float SideOffset = 0.22f;
		constexpr float TiltMin = 25.f;
		constexpr float TiltRange = 15.f;
		constexpr float CenterRise = 0.4f;
		constexpr float BaseCone = 0.25f;
		constexpr int32 FaceEvery = 2;
		constexpr float FaceOut = 0.33f;
		constexpr float FaceUp = 0.25f;
		constexpr float FaceWidth = 0.32f;
		constexpr int32 FaceMood = 2;
		constexpr float LightZ = 0.6f;
		constexpr float LightIntensity = 120.f;
		constexpr float LightRadius = 4.f;
	}

	/** 기둥 (높이·반지름은 배치 값, cm 은 이름에) */
	namespace Pillar
	{
		constexpr float SideShade = 0.82f;
		constexpr float BaseHeightCm = 50.f;
		constexpr float BaseWidth = 2.8f;
		constexpr int32 Flutes = 6;
		constexpr float FluteWidthCm = 10.f;
		constexpr float FluteLength = 0.92f;
		constexpr float CapWidth = 2.9f;
		constexpr float CapHeightCm = 60.f;
		constexpr float BrokenTipHeightCm = 60.f;
		constexpr float BrokenTipLiftCm = 20.f;
		constexpr float BrokenTipRoll = 20.f;
	}

	/** 아치 (폭·높이는 배치 값) */
	namespace Arch
	{
		constexpr float SideShade = 0.82f;
		constexpr float PostWidthCm = 120.f;
		inline const FVector LintelCm = FVector(150.f, 200.f, 100.f);
		constexpr float CurveWidth = 0.95f;
		constexpr float CurveDepthCm = 110.f;
		constexpr float CurveDrop = 10.f;
		constexpr float GemSizeCm = 50.f;
		inline const FVector GemSpotCm = FVector(70.f, 0.f, 30.f);
		constexpr float GemGlow = 2.5f;
	}

	/** 폭포 (폭·낙차는 배치 값) */
	namespace Waterfall
	{
		constexpr float FlowSpeed = 1.6f;
		constexpr float Opacity = 0.78f;
		constexpr float LipOutCm = 40.f;
		constexpr float LipDrop = 0.12f;
		constexpr float LipLength = 0.26f;
		constexpr float LipPitch = -25.f;
		constexpr float SheetThicknessCm = 60.f;
		constexpr float FallOutCm = 110.f;
		constexpr float FallCenter = 0.55f;
		constexpr float FallLength = 0.7f;
		constexpr float FallWidth = 1.05f;
		constexpr float MistThicknessCm = 120.f;
		constexpr float MistWidth = 1.2f;
		constexpr float MistGlow = 0.25f;
		constexpr float SprayDrop = 0.92f;
		constexpr float SprayWidth = 1.4f;
		constexpr float SprayHeight = 0.8f;
		constexpr float SprayGlow = 0.6f;
		constexpr float SprayCore = 0.2f;
	}

	/** 떠 있는 징검돌 (반지름은 배치 값) */
	namespace Stone
	{
		constexpr float SlabThicknessCm = 60.f;
		constexpr float BottomWidth = 0.95f;
		constexpr float BottomDepth = 1.3f;
		constexpr float BottomSink = 0.62f;
		constexpr float SideShade = 0.85f;
		constexpr float BumpOut = 0.4f;
		inline const FVector BumpShape = FVector(0.7f, 0.6f, 0.f);
		constexpr float BumpHeightCm = 70.f;
		constexpr float BumpZCm = -20.f;
		constexpr float BumpTurnPerSeed = 47.f;
	}

	/** 주인공 주변을 떠다니는 빛 먼지 */
	namespace Motes
	{
		constexpr int32 Count = 70;
		inline const FVector Box = FVector(2200.f, 2200.f, 900.f);
		constexpr float Glow = 4.f;
		constexpr float Rim = 0.6f;
		constexpr float DriftSpeed = 60.f;
		constexpr float DriftUpMin = 0.3f;
		constexpr float DriftUpRange = 0.4f;
		constexpr float PhaseSpread = 10.f;
		constexpr float StartScale = 0.06f;
		constexpr float SwayX = 0.7f;
		constexpr float SwayY = 0.5f;
		constexpr float SwayAmount = 40.f;
		constexpr float TwinkleSpeed = 2.f;
		constexpr float TwinklePhase = 4.f;
		constexpr float ScaleMin = 0.04f;
		constexpr float ScaleTwinkle = 0.05f;
	}

	/** 짓는 동안 땅 찾기 */
	namespace Ground
	{
		/** 도형 중심에서 이만큼(크기 배율) 넘게 떨어진 도형은 검사하지 않는다 */
		constexpr float CullSlack = 1.75f;
		/** 이보다 가파르면 땅이 아니다 (법선 z) */
		constexpr float WalkableNormalZ = 0.6f;
		constexpr float DefaultSearch = 6000.f;
		constexpr float OnGroundSearch = 3000.f;
	}
}
