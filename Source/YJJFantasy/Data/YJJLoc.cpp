#include "Data/YJJLoc.h"

#include "Data/YJJData.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "YJJFantasy.h"

UDataTable* FYJJLoc::Table = nullptr;
FString FYJJLoc::CurrentLanguage;
TMap<FName, FString> FYJJLoc::Cache;

void FYJJLoc::Load()
{
	if (Table)
	{
		Table->RemoveFromRoot();
	}
	Cache.Reset();
	FString Lang;
	CurrentLanguage = FParse::Value(FCommandLine::Get(), TEXT("yjjlang="), Lang) ? Lang : FString(DefaultLanguage);

	Table = NewObject<UDataTable>(GetTransientPackage(), NAME_None, RF_Transient);
	Table->RowStruct = FYJJLocRow::StaticStruct();
	Table->AddToRoot();
	const FString Path = FPaths::ProjectContentDir() / FYJJData::DataFolder / FileName;
	FString Csv;
	if (!FFileHelper::LoadFileToString(Csv, *Path))
	{
		UE_LOG(LogYJJFantasy, Error, TEXT("[Loc] %s 를 읽지 못했다"), *Path);
		return;
	}
	for (const FString& Problem : Table->CreateTableFromCSVString(Csv))
	{
		UE_LOG(LogYJJFantasy, Warning, TEXT("[Loc] %s"), *Problem);
	}
	UE_LOG(LogYJJFantasy, Display, TEXT("[Loc] %s: %d keys, language %s"), FileName, Table->GetRowMap().Num(), *CurrentLanguage);
}

FString FYJJLoc::Get(const TCHAR* Key)
{
	if (!Table)
	{
		Load();
	}
	const FName Name(Key);
	if (const FString* Found = Cache.Find(Name))
	{
		return *Found;
	}
	FString Text = FString::Printf(TEXT("[%s]"), Key);
	if (const FYJJLocRow* Row = Table->FindRow<FYJJLocRow>(Name, TEXT("FYJJLoc"), false))
	{
		const FString& Wanted = CurrentLanguage == TEXT("en") ? Row->en : Row->ko;
		Text = Wanted.IsEmpty() ? Row->ko : Wanted;
	}
	else
	{
		UE_LOG(LogYJJFantasy, Warning, TEXT("[Loc] %s 에 '%s' 가 없다"), FileName, Key);
	}
	return Cache.Add(Name, MoveTemp(Text));
}

FString FYJJLoc::Format(const TCHAR* Key, const FStringFormatOrderedArguments& Args)
{
	const FString Pattern = Get(Key);
	return FString::Format(*Pattern, Args);
}

const FString& FYJJLoc::Language()
{
	if (!Table)
	{
		Load();
	}
	return CurrentLanguage;
}

void FYJJLoc::Reload()
{
	Load();
}
