// Fill out your copyright notice in the Description page of Project Settings.

#include "Interfaces/PawnUIInterface.h"

UHeroUIComponent* IPawnUIInterface::GetHeroUIComponent() const
{
	return nullptr;
}

UEnemyUIComponent* IPawnUIInterface::GetEnemyUIComponent() const
{
	return nullptr;
}