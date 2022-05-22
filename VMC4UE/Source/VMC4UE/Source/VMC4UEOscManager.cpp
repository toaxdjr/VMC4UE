// Fill out your copyright notice in the Description page of Project Settings.

#include "VMC4UE/Include/VMC4UEOscManager.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

UVMC4UEOSCManager* UVMC4UEOSCManager::Instance = nullptr;

UVMC4UEOSCManager* UVMC4UEOSCManager::GetInstance()
{
	if (Instance == nullptr)
	{
		Instance = NewObject< UVMC4UEOSCManager >();
		check(Instance)
		Instance->AddToRoot();
	}
	return Instance;
}

//added function to restart hanging OSC threads
void UVMC4UEOSCManager::ResetReceiverCallbacks()
{
	if (Instance == nullptr)
	{
		Instance = NewObject< UVMC4UEOSCManager >();
		check(Instance)
		Instance->AddToRoot();	
	}
	
	for (auto& OscReceiver : Instance->OscReceivers) {
			for (auto& ISSMTM : Instance->StreamingSkeletalMeshTransformMap) {
				if (ISSMTM.Key == OscReceiver->GetPort() && !OscReceiver->OSCReceiveEventDelegate.IsBound()) {
					OscReceiver->OSCReceiveEventDelegate.AddDynamic(ISSMTM.Value, &UVMC4UEStreamingSkeletalMeshTransform::OnReceived);
				}
			}
	}
}
