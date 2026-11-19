#pragma once

#include "CoreMinimal.h"

/**
 * 월드 빌더가 코드로 만드는 먼 풍경·월드 전체 값. 섬 위 배치는 Content/YJJFantasy/Data/Placements.csv.
 */
namespace YJJWorldLayout
{
	/** 환경(해·하늘·안개) 액터를 섬 아래 이 높이에 둔다 */
	constexpr float EnvironmentZ = -2000.f;

	// ---- 먼 섬 (VistaIslands.csv) 위의 실루엣 나무와 폭포
	constexpr int32 FarTreeCount = 5;
	constexpr float FarTreeSpread = 0.7f;
	constexpr float FarTreeScaleMin = 3.f;
	constexpr float FarTreeScaleRange = 2.f;
	/** Seed 가 이 수의 배수인 먼 섬엔 폭포 */
	constexpr int32 FarWaterfallEvery = 3;
	constexpr float FarWaterfallEdge = 0.95f;
	constexpr float FarWaterfallWidth = 900.f;
	constexpr float FarWaterfallDrop = 9000.f;

	// ---- 고리 유적 가운데 빛
	constexpr float RingCoreSize = 900.f;
	constexpr float RingCoreIntensity = 1.5f;
	constexpr float RingCoreCore = 0.6f;
	constexpr float RingCoreRim = 1.f;

	// ---- 떠다니는 작은 바위 무리
	constexpr int32 FloatingRockCount = 40;
	constexpr float FloatingRockDistMin = 9000.f;
	constexpr float FloatingRockDistRange = 9000.f;
	constexpr float FloatingRockZMin = -2000.f;
	constexpr float FloatingRockZRange = 9000.f;
	constexpr float FloatingRockTiltMax = 40.f;
	constexpr float FloatingRockSizeMin = 150.f;
	constexpr float FloatingRockSizeRange = 500.f;

	// ---- 뭉게구름 (셋 중 하나는 얼굴)
	constexpr int32 CloudCount = 46;
	constexpr float CloudDistMin = 3500.f;
	constexpr float CloudDistRange = 22000.f;
	/** 짝수 번째 구름은 섬 아래 */
	constexpr float CloudLowZ = -2500.f;
	constexpr float CloudLowRange = 3000.f;
	constexpr float CloudHighZ = -1500.f;
	constexpr float CloudHighRange = 9000.f;
	/** 시작 섬과 이만큼 가까우면 건너뛴다 */
	constexpr float CloudKeepOut = 3200.f;
	constexpr float CloudSizeMin = 500.f;
	constexpr float CloudSizeRange = 1300.f;
	constexpr int32 CloudPuffsMin = 4;
	constexpr int32 CloudPuffsVariety = 4;
	constexpr int32 CloudFaceEvery = 3;

	// ---- 발밑 만화 구름 바다
	constexpr float CloudSeaZ = -3600.f;
	constexpr int32 CloudSeaCount = 220;
	constexpr float CloudSeaDistMin = 2000.f;
	constexpr float CloudSeaDistRange = 42000.f;
	constexpr float CloudSeaSizeMin = 2600.f;
	constexpr float CloudSeaSizeRange = 3800.f;
	constexpr float CloudSeaBumpZ = 900.f;
	constexpr float CloudSeaFlatY = 0.8f;
	constexpr float CloudSeaFlatZ = 0.32f;
	constexpr float CloudEmissive = 0.3f;
	constexpr float CloudPuffEmissive = 0.35f;

	/** 뭉게구름 한 덩어리의 모양 (구름 크기 S 에 곱하는 비율) */
	struct FCloudPuffShape
	{
		float Spacing;
		float SideMin;
		float SideRange;
		float Jitter;
		float Lift;
		float Depth;
		float Height;
		float FaceOut;
		float FaceUp;
		float FaceWidth;
		float BaseDrop;
		float BaseDepth;
		float BaseHeight;
	};
	constexpr FCloudPuffShape CloudPuff = { 0.55f, 0.6f, 0.35f, 0.4f, 0.15f, 0.85f, 0.65f, 0.415f, 0.2f, 0.32f, 0.05f, 0.7f, 0.12f };

	/** 떠다니는 바위 = 둥근 몸 + 잔디 모자 (바위 크기 S 에 곱하는 위치·크기) */
	struct FBlobShape
	{
		FVector Offset;
		FVector Size;
	};
	inline const FBlobShape FloatingRockBody = { FVector(0.f, 0.f, -0.2f), FVector(1.9f, 1.9f, 1.4f) };
	inline const FBlobShape FloatingRockCap = { FVector(0.f, 0.f, 0.32f), FVector(1.7f, 1.7f, 0.6f) };
	constexpr float FloatingRockSideShade = 0.85f;

	/** AI 성소 섬 바닥 바위 방울: (아래로 내리는 비율, 폭 비율, 높이 비율) x 섬 폭 */
	inline const FVector AIIslandBottomBlobs[] = { FVector(0.08f, 0.8f, 0.4f), FVector(0.3f, 0.42f, 0.3f) };
	constexpr float AIIslandSideShade = 0.88f;

	/** AI 소품 밑동만 밟히게 막는 보이지 않는 원기둥 */
	inline const FVector WindmillColliderCenter(0.f, 0.f, 250.f);
	inline const FVector WindmillColliderSize(420.f, 420.f, 500.f);
	inline const FVector AICrystalColliderCenter(0.f, 0.f, 150.f);
	inline const FVector AICrystalColliderSize(380.f, 380.f, 300.f);

	/** 고리 유적 블록은 원의 접선 방향으로 놓는다 */
	constexpr float RingBlockTurn = 90.f;
	constexpr float FullTurn = 360.f;
	constexpr float QuarterTurn = 90.f;

	// ---- 결정적 난수(YJJShapes::Hash) 씨앗. 바꾸면 배치가 통째로 달라진다
	constexpr int32 HashFarTreeAngle = 7;
	constexpr int32 HashFarTreeDist = 9;
	constexpr int32 HashRockAngle = 17;
	constexpr int32 HashRockDist = 19;
	constexpr int32 HashRockZ = 23;
	constexpr int32 HashRockSize = 29;
	constexpr int32 HashCloudAngle = 41;
	constexpr int32 HashCloudAngleSalt = 3;
	constexpr int32 HashCloudDist = 43;
	constexpr int32 HashCloudDistSalt = 5;
	constexpr int32 HashCloudLow = 47;
	constexpr int32 HashCloudHigh = 53;
	constexpr int32 HashCloudYaw = 59;
	constexpr int32 HashCloudSize = 61;
	constexpr int32 HashCloudPuff = 67;
	constexpr int32 HashCloudPuffY = 71;
	constexpr int32 HashSeaAngle = 131;
	constexpr int32 HashSeaAngleSalt = 7;
	constexpr int32 HashSeaDist = 137;
	constexpr int32 HashSeaDistSalt = 9;
	constexpr int32 HashSeaSize = 139;
	constexpr int32 HashSeaZ = 149;
}
