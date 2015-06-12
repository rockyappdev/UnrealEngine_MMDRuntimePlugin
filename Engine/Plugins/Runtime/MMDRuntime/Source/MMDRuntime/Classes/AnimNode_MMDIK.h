// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Animation/BoneControllers/AnimNode_SkeletalControlBase.h"
#include "Plugins/Developer/IM4U/Source/IM4U/Classes/MMDExtendAsset.h"

#include "AnimNode_MMDIK.generated.h"

/**
 * MMD Bone IK Controller.
 */

USTRUCT()
struct MMDRuntime_API FAnimNode_MMDIK : public FAnimNode_SkeletalControlBase
{
	GENERATED_USTRUCT_BODY()

	/** IM4U plugin MMDExtend data **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MMDExend)
	TAssetPtr<UMMDExtendAsset> MMDExtendAssetRef;

	FAnimNode_MMDIK();

	// FAnimNode_SkeletalControlBase interface
	virtual void EvaluateBoneTransforms(USkeletalMeshComponent* SkelComp, const FBoneContainer& RequiredBones, FA2CSPose& MeshBases, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface

private:
	// FAnimNode_SkeletalControlBase interface
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface
};
