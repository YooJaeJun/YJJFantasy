#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "HAL/IConsoleManager.h"
#include "YJJData.generated.h"

/**
 * 게임 데이터는 코드가 아니라 Content/YJJFantasy/Data/*.csv 에 있다.
 * 게임이 처음 데이터를 찾을 때 CSV를 엔진 DataTable로 읽고, 콘솔 yjj.ReloadData 로 다시 읽는다.
 *
 *   Tuning.csv         조작감·전투·몬스터·수집 수치 (이름, 값)
 *   Colors.csv         색상표 (이름, "R|G|B" 선형 색)
 *   Islands.csv        섬: 위치·크기·색·꽃·구역 이름
 *   Placements.csv     섬 위에 놓는 모든 것: 나무·바위·등불·상자·몬스터·이동 장치·별가루·지형 조각
 *   VistaIslands.csv   닿을 수 없는 먼 섬
 *   Tour.csv           투어 스크린샷 지점
 *
 * 글자는 Localization.csv (FYJJLoc).
 */

/** Tuning.csv 한 줄 */
USTRUCT()
struct FYJJTuningRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	float Value = 0.f;

	UPROPERTY(EditAnywhere)
	FString Note;
};

/** Colors.csv 한 줄 */
USTRUCT()
struct FYJJColorRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 선형 색 "R|G|B" 또는 "R|G|B|A" */
	UPROPERTY(EditAnywhere)
	FString Linear;

	UPROPERTY(EditAnywhere)
	FString Note;
};

/** Islands.csv 한 줄 (행 이름 = 섬 키: Hub, Meadow ...) */
USTRUCT()
struct FYJJIslandRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 섬 가운데 (BaseZ 기준 섬 좌표) */
	UPROPERTY(EditAnywhere) float X = 0.f;
	UPROPERTY(EditAnywhere) float Y = 0.f;
	UPROPERTY(EditAnywhere) float Z = 0.f;
	UPROPERTY(EditAnywhere) float Radius = 2000.f;

	/** Colors.csv 이름 또는 "R|G|B" */
	UPROPERTY(EditAnywhere) FString Grass;
	UPROPERTY(EditAnywhere) FString Rock;
	UPROPERTY(EditAnywhere) int32 Seed = 0;

	/** 꽃 송이 수와 색들 ("A;B;C" 꼴로 Colors.csv 이름) */
	UPROPERTY(EditAnywhere) int32 FlowerCount = 0;
	UPROPERTY(EditAnywhere) FString FlowerColors;

	/** 들어가면 이름 카드가 뜨는 반지름 */
	UPROPERTY(EditAnywhere) float ZoneRadius = 2000.f;
	UPROPERTY(EditAnywhere) FString NameKey;
	UPROPERTY(EditAnywhere) FString SubtitleKey;

	UPROPERTY(EditAnywhere) FString Note;
};

/**
 * Placements.csv 한 줄. Type 에 따라 쓰는 열이 다르다 (CSV 머리말 아래 Note 참고).
 * Island 가 섬 키면 그 섬 가운데 기준, World 면 섬 좌표(BaseZ 기준) 그대로.
 * Ground = 1 이면 (X, Y) 아래 땅을 찾아 Lift 만큼 띄운다.
 */
USTRUCT()
struct FYJJPlacementRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) FString Island;
	UPROPERTY(EditAnywhere) FString Type;
	UPROPERTY(EditAnywhere) float X = 0.f;
	UPROPERTY(EditAnywhere) float Y = 0.f;
	UPROPERTY(EditAnywhere) float Z = 0.f;
	UPROPERTY(EditAnywhere) int32 Ground = 0;
	UPROPERTY(EditAnywhere) float Lift = 0.f;
	UPROPERTY(EditAnywhere) float Yaw = 0.f;
	UPROPERTY(EditAnywhere) float Pitch = 0.f;
	UPROPERTY(EditAnywhere) float Roll = 0.f;
	/** 크기 (종류마다 뜻이 다름: 바위 크기, 수정 높이, 징검돌 반지름 ...) */
	UPROPERTY(EditAnywhere) float Size = 0.f;
	UPROPERTY(EditAnywhere) float Scale = 1.f;
	/** 지형 조각(Shape) 크기 cm */
	UPROPERTY(EditAnywhere) float SX = 0.f;
	UPROPERTY(EditAnywhere) float SY = 0.f;
	UPROPERTY(EditAnywhere) float SZ = 0.f;
	UPROPERTY(EditAnywhere) int32 Seed = 0;
	UPROPERTY(EditAnywhere) FString Color;
	UPROPERTY(EditAnywhere) FString Color2;
	/** 지형 조각 재질: Surface, Solid, Water */
	UPROPERTY(EditAnywhere) FString Mat;
	UPROPERTY(EditAnywhere) float Emissive = 0.f;
	UPROPERTY(EditAnywhere) float Rough = 0.85f;
	UPROPERTY(EditAnywhere) int32 Collide = 0;
	UPROPERTY(EditAnywhere) float A = 0.f;
	UPROPERTY(EditAnywhere) float B = 0.f;
	/** 선의 끝점 / 움직이는 발판의 이동 벡터 */
	UPROPERTY(EditAnywhere) float X2 = 0.f;
	UPROPERTY(EditAnywhere) float Y2 = 0.f;
	UPROPERTY(EditAnywhere) float Z2 = 0.f;
	UPROPERTY(EditAnywhere) int32 Count = 0;
	/** Localization.csv 키, 도형 이름, 빠진 칸 목록 등 */
	UPROPERTY(EditAnywhere) FString Text;
	UPROPERTY(EditAnywhere) FString Note;
};

/** VistaIslands.csv 한 줄 */
USTRUCT()
struct FYJJVistaIslandRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) float X = 0.f;
	UPROPERTY(EditAnywhere) float Y = 0.f;
	UPROPERTY(EditAnywhere) float Z = 0.f;
	UPROPERTY(EditAnywhere) float Radius = 4000.f;
	UPROPERTY(EditAnywhere) int32 Seed = 0;
	UPROPERTY(EditAnywhere) int32 Trees = 0;
	UPROPERTY(EditAnywhere) FString Note;
};

/** Tour.csv 한 줄 (행 이름 = 스크린샷 파일 이름) */
USTRUCT()
struct FYJJTourRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) float X = 0.f;
	UPROPERTY(EditAnywhere) float Y = 0.f;
	UPROPERTY(EditAnywhere) float Z = 0.f;
	UPROPERTY(EditAnywhere) float Yaw = 0.f;
	UPROPERTY(EditAnywhere) float Pitch = 0.f;
	UPROPERTY(EditAnywhere) FString Note;
};

/** 게임 데이터 창구. 모든 함수는 정적이고, 처음 부를 때 CSV를 읽는다 */
class YJJFANTASY_API FYJJData
{
public:
	/** Tuning.csv 값. 없는 이름이면 경고를 한 번 찍고 Fallback */
	static float Tune(const TCHAR* Name, float Fallback = 0.f);

	/** Colors.csv 이름 또는 "R|G|B" 문자열 -> 선형 색. 모르는 이름은 분홍(눈에 띄게) */
	static FLinearColor Color(const FString& NameOrLinear);

	/** "A;B;C" -> 색 목록 */
	static TArray<FLinearColor> Colors(const FString& List);

	static const FYJJIslandRow* Island(const FName& Key);
	/** Islands.csv 줄 순서 */
	static TArray<FName> IslandKeys();

	/** CSV 줄 순서대로 */
	static TArray<const FYJJPlacementRow*> Placements();
	static TArray<const FYJJVistaIslandRow*> VistaIslands();
	static TArray<TPair<FName, const FYJJTourRow*>> Tour();

	/** CSV를 다시 읽는다 (콘솔 yjj.ReloadData) */
	static void Reload();

	/** CSV 폴더 (Content 기준) */
	static constexpr const TCHAR* DataFolder = TEXT("YJJFantasy/Data");

private:
	struct FStore
	{
		UDataTable* Tuning = nullptr;
		UDataTable* Colors = nullptr;
		UDataTable* Islands = nullptr;
		UDataTable* Placements = nullptr;
		UDataTable* Vista = nullptr;
		UDataTable* Tour = nullptr;
		TSet<FString> Missing;
	};

	static FStore Instance;
	static FStore& Store();
	static void Load(FStore& S);
	static UDataTable* LoadTable(const TCHAR* File, UScriptStruct* RowStruct);
	static FLinearColor ParseLinear(const FString& Text, bool& bOk);

	template <typename RowType>
	static TArray<const RowType*> Rows(UDataTable* Table)
	{
		TArray<const RowType*> Out;
		if (Table)
		{
			Table->ForeachRow<RowType>(TEXT("FYJJData"), [&Out](const FName&, const RowType& Row) { Out.Add(&Row); });
		}
		return Out;
	}

	static FAutoConsoleCommand ReloadCommand;
};
