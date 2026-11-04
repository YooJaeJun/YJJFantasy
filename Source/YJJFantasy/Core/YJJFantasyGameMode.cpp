#include "Core/YJJFantasyGameMode.h"
#include "Player/YJJHero.h"

AYJJFantasyGameMode::AYJJFantasyGameMode()
{
	DefaultPawnClass = AYJJHero::StaticClass();
}
