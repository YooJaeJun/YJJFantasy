#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "YJJLoc.generated.h"

/** Localization.csv 한 줄 (행 이름 = 키) */
USTRUCT()
struct FYJJLocRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FString ko;

	UPROPERTY(EditAnywhere)
	FString en;

	UPROPERTY(EditAnywhere)
	FString Note;
};

/**
 * 화면에 나오는 모든 글자는 Content/YJJFantasy/Data/Localization.csv 에 있다.
 *   열: Key, ko, en, Note  (언어를 늘리려면 FYJJLocRow 에 열을 하나 더 붙인다)
 * 언어: 명령줄 -yjjlang=en, 없으면 ko. 그 언어 칸이 비어 있으면 ko, 그것도 없으면 [Key] (빠진 글자가 눈에 띄게).
 * 서식 자리는 {0} {1} ... (FString::Format)
 */
class YJJFANTASY_API FYJJLoc
{
public:
	static FString Get(const TCHAR* Key);
	static FString Format(const TCHAR* Key, const FStringFormatOrderedArguments& Args);

	/** 지금 언어 (ko, en) */
	static const FString& Language();

	/** CSV를 다시 읽는다 (yjj.ReloadData 가 함께 부른다) */
	static void Reload();

	static constexpr const TCHAR* FileName = TEXT("Localization.csv");
	static constexpr const TCHAR* DefaultLanguage = TEXT("ko");

private:
	static void Load();

	static UDataTable* Table;
	static FString CurrentLanguage;
	/** 찾은 글자 캐시 (키 -> 지금 언어의 글자) */
	static TMap<FName, FString> Cache;
};
