// Fill out your copyright notice in the Description page of Project Settings.

#include "WGameInstance.h"
#include "MoviePlayer.h"
#include "GameMode/WSurvivalGameMode.h"

void UWGameInstance::Init()
{
    Super::Init();

    FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &ThisClass::OnPreLoadMap);
    FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::OnDestinationWorldLoaded);
}

void UWGameInstance::OnPreLoadMap(const FString& MapName)
{
    // 退出到主菜单前，先保存当前真实进度（捕捉退出那一刻的血量/怒气等），供续关精确还原
    if (UWorld* CurrentWorld = GetWorld())
    {
        if (AWSurvivalGameMode* SurvivalGM = CurrentWorld->GetAuthGameMode<AWSurvivalGameMode>())
        {
            SurvivalGM->SaveCurrentProgress();
        }
    }

    FLoadingScreenAttributes LoadingScreenAttributes;
    LoadingScreenAttributes.bAutoCompleteWhenLoadingCompletes = true;
    LoadingScreenAttributes.MinimumLoadingScreenDisplayTime = 2.f;
    LoadingScreenAttributes.WidgetLoadingScreen = FLoadingScreenAttributes::NewTestLoadingScreenWidget();

    GetMoviePlayer()->SetupLoadingScreen(LoadingScreenAttributes);

}

void UWGameInstance::OnDestinationWorldLoaded(UWorld* LoadedWorld)
{
    GetMoviePlayer()->StopMovie();
}

TSoftObjectPtr<UWorld> UWGameInstance::GetGameLevelByTag(FGameplayTag InTag) const
{
	for (const FWGameLevelSet& GameLevelSet : GameLevelSets)
	{
		if (!GameLevelSet.IsValid()) continue;

        if (GameLevelSet.LevelTag == InTag)
        {
            return GameLevelSet.Level;
        }
	}

    return TSoftObjectPtr<UWorld>();
}