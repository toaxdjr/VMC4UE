// Fill out your copyright notice in the Description page of Project Settings.

#include "../Include/VMC4UEBlueprintFunctionLibrary.h"
#include "../Include/VMC4UEStreamingData.h"
#include "VMC4UE/Include/VMC4UEOSCManager.h"
#include "UEOSC/Include/UEOSCElement.h"
#include "UEOSC/Include/UEOSCReceiver.h"
#include "Kismet/KismetMathLibrary.h"
#include "GenericPlatform/GenericPlatformMath.h"

void UVMC4UEBlueprintFunctionLibrary::OnReceivedVMC(UVMC4UEStreamingSkeletalMeshTransform *SkeletalMeshTransform, const FName &Address, const TArray<FUEOSCElement> &Data, const FString &SenderIp)
{
	if (SkeletalMeshTransform == nullptr)
	{
		return;
	}

    const FString AddressString = Address.ToString();
	float scale = SkeletalMeshTransform->Scale;

	if (AddressString == TEXT("/VMC/Ext/Root/Pos"))
	{
		if (Data.Num() < 7)
		{
			return;
		}
		int32 Index = 0;

		FRWScopeLock RWScopeLock(SkeletalMeshTransform->RWLock, FRWScopeLockType::SLT_Write);
		
		++Index;
		const auto UnityLocationX = Data[Index++].FloatValue;
		const auto UnityLocationY = Data[Index++].FloatValue;
		const auto UnityLocationZ = Data[Index++].FloatValue;
		const auto UnityRotationX = Data[Index++].FloatValue;
		const auto UnityRotationY = Data[Index++].FloatValue;
		const auto UnityRotationZ = Data[Index++].FloatValue;
		const auto UnityRotationW = Data[Index++].FloatValue;

		auto& VMCBone = SkeletalMeshTransform->Root;
		if (0.0f != scale)
		{
			VMCBone.Location.X = -UnityLocationX * scale;
			VMCBone.Location.Y = UnityLocationZ * scale;
			VMCBone.Location.Z = UnityLocationY * scale;
		}
		else {
		}
		VMCBone.Rotation.X = -UnityRotationX;
		VMCBone.Rotation.Y = UnityRotationZ;
		VMCBone.Rotation.Z = UnityRotationY;
		VMCBone.Rotation.W = UnityRotationW;

		//VMCBone.Rotation = FQuat::Identity;
		
		//UE_LOG(LogTemp, Log, TEXT("VMCBone.Location = %3.3f, %3.3f, %3.3f"), VMCBone.Location.X, VMCBone.Location.Y, VMCBone.Location.Z);
	}
	else if (AddressString == TEXT("/VMC/Ext/Bone/Pos"))
    {
        if (Data.Num() < 8)
        {
            return;
        }
        int32 Index = 0;

		FRWScopeLock RWScopeLock(SkeletalMeshTransform->RWLock, FRWScopeLockType::SLT_Write);
		
        const auto BoneName = Data[Index++].StringValue;
        const auto UnityLocationX = Data[Index++].FloatValue;
        const auto UnityLocationY = Data[Index++].FloatValue;
        const auto UnityLocationZ = Data[Index++].FloatValue;
        const auto UnityRotationX = Data[Index++].FloatValue;
        const auto UnityRotationY = Data[Index++].FloatValue;
        const auto UnityRotationZ = Data[Index++].FloatValue;
        const auto UnityRotationW = Data[Index++].FloatValue;

		auto& VMCBone = SkeletalMeshTransform->Bones.FindOrAdd(BoneName);
		if (0.0f != scale)
		{
			VMCBone.Location.X = -UnityLocationX * scale;
			VMCBone.Location.Y = UnityLocationZ * scale;
			VMCBone.Location.Z = UnityLocationY * scale;
		}
		VMCBone.Rotation.X = -UnityRotationX;
		VMCBone.Rotation.Y = UnityRotationZ;
		VMCBone.Rotation.Z = UnityRotationY;
		VMCBone.Rotation.W = UnityRotationW;

		//VMCBone.Rotation = FQuat::Identity;
	}
	else if (AddressString == TEXT("/VMC/Ext/Blend/Val"))
	{

		if (Data.Num() < 2)
		{
			//UE_LOG(LogTemp, Log, TEXT("No blendshapes"));
			return;
		}
		int32 Index = 0;
		FString pop = AddressString;

		//UE_LOG(LogTemp, Log, TEXT("yes blendshapes %s : %d"), pop,);
		const auto Name = Data[Index++].StringValue;
		const auto Value = Data[Index++].FloatValue;
		//UE_LOG(LogTemp, Log, TEXT("yes blendshapes  %f"), Value);
		auto& TargetBlendShape = SkeletalMeshTransform->FutureBlendShapes.FindOrAdd(Name);

		TargetBlendShape = Value;
		
	}
	else if (AddressString == TEXT("/VMC/Ext/Blend/Apply"))
	{
		FRWScopeLock RWScopeLock(SkeletalMeshTransform->RWLock, FRWScopeLockType::SLT_Write);
		
		SkeletalMeshTransform->CurrentBlendShapes.Empty();
		for (auto& FutureBlendShape : SkeletalMeshTransform->FutureBlendShapes)
		{   /*
			if (FutureBlendShape.Key == "eyeBlinkLeft") {
				UE_LOG(LogTemp, Log, TEXT("Value for left eye %f"), FutureBlendShape.Value);
			}*/
			SkeletalMeshTransform->CurrentBlendShapes.FindOrAdd(FutureBlendShape.Key);
			SkeletalMeshTransform->CurrentBlendShapes[FutureBlendShape.Key] = FutureBlendShape.Value;
		}
		SkeletalMeshTransform->FutureBlendShapes.Empty();
	}
}

TWeakObjectPtr<UVMC4UEStreamingSkeletalMeshTransform> UVMC4UEBlueprintFunctionLibrary::GetStreamingSkeletalMeshTransform(int32 Port)
{
	UVMC4UEOSCManager* OSCManager = UVMC4UEOSCManager::GetInstance();
	if (OSCManager == nullptr)
	{
		return nullptr;
	}
	
	{
		// Get
		FRWScopeLock RWScopeLock(OSCManager->RWLock, FRWScopeLockType::SLT_ReadOnly);
		auto StreamingSkeletalMeshTransform = OSCManager->StreamingSkeletalMeshTransformMap.Find(Port);
		if (StreamingSkeletalMeshTransform != nullptr)
		{
			return *StreamingSkeletalMeshTransform;
		}
	}
	{
		// Create
		FRWScopeLock RWScopeLock(OSCManager->RWLock, FRWScopeLockType::SLT_Write);
		auto StreamingSkeletalMeshTransform = OSCManager->StreamingSkeletalMeshTransformMap.Find(Port);
		if (StreamingSkeletalMeshTransform != nullptr)
		{
			return *StreamingSkeletalMeshTransform;
		}
		UVMC4UEStreamingSkeletalMeshTransform* NewStreamingSkeletalMeshTransform = NewObject<UVMC4UEStreamingSkeletalMeshTransform>();
		//FRWScopeLock RWScopeLock2(NewStreamingSkeletalMeshTransform->RWLock, FRWScopeLockType::SLT_Write);
		OSCManager->StreamingSkeletalMeshTransformMap.Emplace(Port, NewStreamingSkeletalMeshTransform);

		// Bind Port
		UUEOSCReceiver* OscReceiver = NewObject<UUEOSCReceiver>();
		OscReceiver->OSCReceiveEventDelegate.AddDynamic(NewStreamingSkeletalMeshTransform, &UVMC4UEStreamingSkeletalMeshTransform::OnReceived);
		OscReceiver->Connect(Port);

		OSCManager->OscReceivers.Add(OscReceiver);

		return NewStreamingSkeletalMeshTransform;
	}
	return nullptr;
}

void UVMC4UEBlueprintFunctionLibrary::RefreshConnection(float Seconds)
{
	UVMC4UEOSCManager* OSCManager = UVMC4UEOSCManager::GetInstance();
	if (OSCManager == nullptr)
	{
		return;
	}

	Seconds = FGenericPlatformMath::Min(Seconds, 1.0f);


	// Reconnect
	{
		FRWScopeLock RWScopeLock(OSCManager->RWLock, FRWScopeLockType::SLT_ReadOnly);

		auto Now = UKismetMathLibrary::Now();
		OSCManager->ResetReceiverCallbacks();
		
		for (auto& OscReceiver : OSCManager->OscReceivers)
		{
			UE_LOG(LogTemp, Log, TEXT("Port: %d health check\n"), OscReceiver->GetPort());
			auto Span = Now - OscReceiver->GetLastUpdateTime();
			UE_LOG(LogTemp, Log, TEXT("Seconds since last msg: %ld\n"), Span.GetTotalSeconds());
			if (Span.GetTotalSeconds() > (double)Seconds)
			{
				
				UE_LOG(LogTemp, Log, TEXT("Trying to reconnect: %d\n"), OscReceiver->GetPort());
				OscReceiver->Reconnect();
			}
			if (Span.GetTotalSeconds() < (double)0.0f) {
				UE_LOG(LogTemp, Log, TEXT("Trying to reconnect messages from the future?: %d\n"), OscReceiver->GetPort());
				OscReceiver->Reconnect();
			}
		}
	}
}
