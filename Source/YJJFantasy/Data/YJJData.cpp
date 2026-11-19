#include "Data/YJJData.h"

#include "Data/YJJLoc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "YJJFantasy.h"

FAutoConsoleCommand FYJJData::ReloadCommand(TEXT("yjj.ReloadData"), TEXT("Content/YJJFantasy/Data/*.csv 다시 읽기 (섬은 다음 시작부터)"),
	FConsoleCommandDelegate::CreateStatic(&FYJJData::Reload));

UDataTable* FYJJData::LoadTable(const TCHAR* File, UScriptStruct* RowStruct)
{
	UDataTable* Table = NewObject<UDataTable>(GetTransientPackage(), NAME_None, RF_Transient);
	Table->RowStruct = RowStruct;
	Table->AddToRoot();
	const FString Path = FPaths::ProjectContentDir() / DataFolder / File;
	FString Csv;
	if (!FFileHelper::LoadFileToString(Csv, *Path))
	{
		UE_LOG(LogYJJFantasy, Error, TEXT("[Data] %s 를 읽지 못했다 (%s)"), File, *Path);
		return Table;
	}
	for (const FString& Problem : Table->CreateTableFromCSVString(Csv))
	{
		UE_LOG(LogYJJFantasy, Warning, TEXT("[Data] %s: %s"), File, *Problem);
	}
	UE_LOG(LogYJJFantasy, Display, TEXT("[Data] %s: %d rows"), File, Table->GetRowMap().Num());
	return Table;
}

void FYJJData::Load(FStore& S)
{
	for (UDataTable* Old : { S.Tuning, S.Colors, S.Islands, S.Placements, S.Vista, S.Tour })
	{
		if (Old)
		{
			Old->RemoveFromRoot();
		}
	}
	S.Tuning = LoadTable(TEXT("Tuning.csv"), FYJJTuningRow::StaticStruct());
	S.Colors = LoadTable(TEXT("Colors.csv"), FYJJColorRow::StaticStruct());
	S.Islands = LoadTable(TEXT("Islands.csv"), FYJJIslandRow::StaticStruct());
	S.Placements = LoadTable(TEXT("Placements.csv"), FYJJPlacementRow::StaticStruct());
	S.Vista = LoadTable(TEXT("VistaIslands.csv"), FYJJVistaIslandRow::StaticStruct());
	S.Tour = LoadTable(TEXT("Tour.csv"), FYJJTourRow::StaticStruct());
	S.Missing.Reset();
}

FYJJData::FStore FYJJData::Instance;

FYJJData::FStore& FYJJData::Store()
{
	if (!Instance.Tuning)
	{
		Load(Instance);
	}
	return Instance;
}

float FYJJData::Tune(const TCHAR* Name, float Fallback)
{
	FStore& S = Store();
	if (const FYJJTuningRow* Row = S.Tuning->FindRow<FYJJTuningRow>(FName(Name), TEXT("FYJJData"), false))
	{
		return Row->Value;
	}
	if (!S.Missing.Contains(Name))
	{
		S.Missing.Add(Name);
		UE_LOG(LogYJJFantasy, Warning, TEXT("[Data] Tuning.csv 에 %s 가 없다 (기본값 %g)"), Name, Fallback);
	}
	return Fallback;
}

FLinearColor FYJJData::ParseLinear(const FString& Text, bool& bOk)
{
	TArray<FString> Parts;
	Text.ParseIntoArray(Parts, TEXT("|"));
	bOk = Parts.Num() == 3 || Parts.Num() == 4;
	if (!bOk)
	{
		return FLinearColor::White;
	}
	return FLinearColor(FCString::Atof(*Parts[0]), FCString::Atof(*Parts[1]), FCString::Atof(*Parts[2]), Parts.Num() == 4 ? FCString::Atof(*Parts[3]) : 1.f);
}

FLinearColor FYJJData::Color(const FString& NameOrLinear)
{
	bool bOk = false;
	if (NameOrLinear.Contains(TEXT("|")))
	{
		const FLinearColor C = ParseLinear(NameOrLinear, bOk);
		if (bOk)
		{
			return C;
		}
	}
	FStore& S = Store();
	if (const FYJJColorRow* Row = S.Colors->FindRow<FYJJColorRow>(FName(*NameOrLinear), TEXT("FYJJData"), false))
	{
		const FLinearColor C = ParseLinear(Row->Linear, bOk);
		if (bOk)
		{
			return C;
		}
	}
	if (!S.Missing.Contains(NameOrLinear))
	{
		S.Missing.Add(NameOrLinear);
		UE_LOG(LogYJJFantasy, Warning, TEXT("[Data] Colors.csv 에 '%s' 가 없다"), *NameOrLinear);
	}
	return FLinearColor(1.f, 0.f, 1.f);
}

TArray<FLinearColor> FYJJData::Colors(const FString& List)
{
	TArray<FString> Names;
	List.ParseIntoArray(Names, TEXT(";"));
	TArray<FLinearColor> Out;
	for (const FString& Name : Names)
	{
		Out.Add(Color(Name.TrimStartAndEnd()));
	}
	return Out;
}

const FYJJIslandRow* FYJJData::Island(const FName& Key)
{
	const FYJJIslandRow* Row = Store().Islands->FindRow<FYJJIslandRow>(Key, TEXT("FYJJData"), false);
	if (!Row)
	{
		UE_LOG(LogYJJFantasy, Error, TEXT("[Data] Islands.csv 에 %s 줄이 없다"), *Key.ToString());
	}
	return Row;
}

TArray<FName> FYJJData::IslandKeys()
{
	TArray<FName> Keys;
	Store().Islands->ForeachRow<FYJJIslandRow>(TEXT("FYJJData"), [&Keys](const FName& Key, const FYJJIslandRow&) { Keys.Add(Key); });
	return Keys;
}

TArray<const FYJJPlacementRow*> FYJJData::Placements()
{
	return Rows<FYJJPlacementRow>(Store().Placements);
}

TArray<const FYJJVistaIslandRow*> FYJJData::VistaIslands()
{
	return Rows<FYJJVistaIslandRow>(Store().Vista);
}

TArray<TPair<FName, const FYJJTourRow*>> FYJJData::Tour()
{
	TArray<TPair<FName, const FYJJTourRow*>> Out;
	Store().Tour->ForeachRow<FYJJTourRow>(TEXT("FYJJData"), [&Out](const FName& Key, const FYJJTourRow& Row) { Out.Emplace(Key, &Row); });
	return Out;
}

void FYJJData::Reload()
{
	Load(Store());
	FYJJLoc::Reload();
	UE_LOG(LogYJJFantasy, Display, TEXT("[Data] reloaded"));
}
