// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MMDRuntimePrivatePCH.h"

#include "Runtime/Engine/Public/AnimationRuntime.h"
#include "AnimNode_MMDIK.h"

//DEFINE_LOG_CATEGORY(LogAnimation);

/////////////////////////////////////////////////////
// FAnimNode_MMDIK

FAnimNode_MMDIK::FAnimNode_MMDIK()
{
}

void FAnimNode_MMDIK::EvaluateBoneTransforms(USkeletalMeshComponent* SkelComp, const FBoneContainer& RequiredBones, FA2CSPose& MeshBases, TArray<FBoneTransform>& OutBoneTransforms)
{

	FVector EffectorLocation(FVector::ZeroVector);
	FVector JointTargetLocation(FVector::ZeroVector);
	TEnumAsByte<enum EBoneControlSpace> EffectorLocationSpace(BCS_BoneSpace);
	TEnumAsByte<enum EBoneControlSpace> JointTargetLocationSpace(BCS_ParentBoneSpace);

	FTransform UpperLimbCSTransform;
	FTransform LowerLimbCSTransform;
	FTransform EndBoneCSTransform;
	FTransform JointTargetTransform;
	const float BlendWeight = FMath::Clamp<float>(1.0f, 0.f, 1.f);

	check(OutBoneTransforms.Num() == 0);

	const FStringAssetReference& AssetRef = MMDExtendAssetRef.ToStringReference();

	UMMDExtendAsset* MMDExtendAssetPtr = MMDExtendAssetRef.Get();

	if (MMDExtendAssetPtr == nullptr)
	{
		UE_LOG(LogAnimation, Warning, TEXT("FAnimNode_MMDIK::EvaluateBoneTransforms: MMExtendPtr is nullptr!"));
		return;
	}

	for (int32 indexIK = 0; indexIK < MMDExtendAssetPtr->IkInfoList.Num(); indexIK++)
	{
		JointTargetLocationSpace = BCS_ParentBoneSpace;

		// Get indices of the lower and upper limb bones and check validity.
		bool bInvalidLimb = false;

		// IKBoneIndex
		const FName EffectorSpaceBoneName = MMDExtendAssetPtr->IkInfoList[indexIK].IKBoneName;
		const int32 EffectorSpaceBoneIndex = MMDExtendAssetPtr->IkInfoList[indexIK].IKBoneIndex;

		const FName EndBoneName = MMDExtendAssetPtr->IkInfoList[indexIK].TargetBoneName;
		const int32 EndBoneIndex = MMDExtendAssetPtr->IkInfoList[indexIK].TargetBoneIndex;

		if (EffectorSpaceBoneName.IsEqual(TEXT("¶‚Â‚Üæ‚h‚j")) || EffectorSpaceBoneName.IsEqual(TEXT("‰E‚Â‚Üæ‚h‚j")))
		{
			JointTargetLocationSpace = BCS_BoneSpace;
		}

		const int32 LowerLimbIndex = RequiredBones.GetParentBoneIndex(EndBoneIndex);
		if (LowerLimbIndex == INDEX_NONE)
		{
			bInvalidLimb = true;
		}

		int32 UpperLimbIndex = INDEX_NONE;

		if (!bInvalidLimb)
		{
			UpperLimbIndex = RequiredBones.GetParentBoneIndex(LowerLimbIndex);
			if (UpperLimbIndex == INDEX_NONE)
			{
				bInvalidLimb = true;
			}

		}

		if (!bInvalidLimb)
		{
			int32 JointTargetSpaceBoneIndex = INDEX_NONE;

			if (MMDExtendAssetPtr->IkInfoList[indexIK].ikLinkList.Num() > 0)
			{
				JointTargetSpaceBoneIndex = MMDExtendAssetPtr->IkInfoList[indexIK].ikLinkList[0].BoneIndex;
			}

			UpperLimbCSTransform = MeshBases.GetComponentSpaceTransform(UpperLimbIndex);
			LowerLimbCSTransform = MeshBases.GetComponentSpaceTransform(LowerLimbIndex);

			EndBoneCSTransform = MeshBases.GetComponentSpaceTransform(EndBoneIndex);

			FTransform JointTargetTransform(JointTargetLocation);
			FAnimationRuntime::ConvertBoneSpaceTransformToCS(SkelComp, MeshBases, JointTargetTransform, JointTargetSpaceBoneIndex, JointTargetLocationSpace);

			const FVector RootPos = UpperLimbCSTransform.GetTranslation();
			const FVector InitialJointPos = LowerLimbCSTransform.GetTranslation();
			const FVector InitialEndPos = EndBoneCSTransform.GetTranslation();

			FTransform EffectorTransform(EffectorLocation);
			FAnimationRuntime::ConvertBoneSpaceTransformToCS(SkelComp, MeshBases, EffectorTransform, EffectorSpaceBoneIndex, EffectorLocationSpace);

			FVector DesiredPos = EffectorTransform.GetTranslation();
			FVector DesiredDelta = DesiredPos - RootPos;
			float DesiredLength = DesiredDelta.Size();

			// Check to handle case where DesiredPos is the same as RootPos.
			FVector	DesiredDir;
			if (DesiredLength < (float)KINDA_SMALL_NUMBER)
			{
				DesiredLength = (float)KINDA_SMALL_NUMBER;
				DesiredDir = FVector(1, 0, 0);
			}
			else
			{
				DesiredDir = DesiredDelta / DesiredLength;
			}

			FVector	JointTargetPos = JointTargetTransform.GetTranslation();
			FVector JointTargetDelta = JointTargetPos - RootPos;
			float JointTargetLength = JointTargetDelta.Size();

			// Same check as above, to cover case when JointTarget position is the same as RootPos.
			FVector JointPlaneNormal, JointBendDir;
			if (JointTargetLength < (float)KINDA_SMALL_NUMBER)
			{
				JointBendDir = FVector(0, 1, 0);
				JointPlaneNormal = FVector(0, 0, 1);
			}
			else
			{
				JointPlaneNormal = DesiredDir ^ JointTargetDelta;

				// If we are trying to point the limb in the same direction that we are supposed to displace the joint in,
				// we have to just pick 2 random vector perp to DesiredDir and each other.
				if (JointPlaneNormal.Size() < (float)KINDA_SMALL_NUMBER)
				{
					DesiredDir.FindBestAxisVectors(JointPlaneNormal, JointBendDir);
				}
				else
				{
					JointPlaneNormal.Normalize();

					// Find the final member of the reference frame by removing any component of JointTargetDelta along DesiredDir.
					// This should never leave a zero vector, because we've checked DesiredDir and JointTargetDelta are not parallel.
					JointBendDir = JointTargetDelta - ((JointTargetDelta | DesiredDir) * DesiredDir);
					JointBendDir.Normalize();
				}
			}

			// Find lengths of upper and lower limb in the ref skeleton.
			// Use actual sizes instead of ref skeleton, so we take into account translation and scaling from other bone controllers.
			float LowerLimbLength = (InitialEndPos - InitialJointPos).Size();
			float UpperLimbLength = (InitialJointPos - RootPos).Size();
			float MaxLimbLength = LowerLimbLength + UpperLimbLength;

			FVector OutEndPos = DesiredPos;
			FVector OutJointPos = InitialJointPos;

			// If we are trying to reach a goal beyond the length of the limb, clamp it to something solvable and extend limb fully.
			if (DesiredLength > MaxLimbLength)
			{
				OutEndPos = RootPos + (MaxLimbLength * DesiredDir);
				OutJointPos = RootPos + (UpperLimbLength * DesiredDir);
			}
			else
			{
				// So we have a triangle we know the side lengths of. We can work out the angle between DesiredDir and the direction of the upper limb
				// using the sin rule:
				const float TwoAB = 2.f * UpperLimbLength * DesiredLength;

				const float CosAngle = (TwoAB != 0.f) ? ((UpperLimbLength*UpperLimbLength) + (DesiredLength*DesiredLength) - (LowerLimbLength*LowerLimbLength)) / TwoAB : 0.f;

				// If CosAngle is less than 0, the upper arm actually points the opposite way to DesiredDir, so we handle that.
				const bool bReverseUpperBone = (CosAngle < 0.f);

				// If CosAngle is greater than 1.f, the triangle could not be made - we cannot reach the target.
				// We just have the two limbs double back on themselves, and EndPos will not equal the desired EffectorLocation.
				if ((CosAngle > 1.f) || (CosAngle < -1.f))
				{
					// Because we want the effector to be a positive distance down DesiredDir, we go back by the smaller section.
					if (UpperLimbLength > LowerLimbLength)
					{
						OutJointPos = RootPos + (UpperLimbLength * DesiredDir);
						OutEndPos = OutJointPos - (LowerLimbLength * DesiredDir);
					}
					else
					{
						OutJointPos = RootPos - (UpperLimbLength * DesiredDir);
						OutEndPos = OutJointPos + (LowerLimbLength * DesiredDir);
					}
				}
				else
				{
					// Angle between upper limb and DesiredDir
					const float Angle = FMath::Acos(CosAngle);

					// Now we calculate the distance of the joint from the root -> effector line.
					// This forms a right-angle triangle, with the upper limb as the hypotenuse.
					const float JointLineDist = UpperLimbLength * FMath::Sin(Angle);

					// And the final side of that triangle - distance along DesiredDir of perpendicular.
					// ProjJointDistSqr can't be neg, because JointLineDist must be <= UpperLimbLength because appSin(Angle) is <= 1.
					const float ProjJointDistSqr = (UpperLimbLength*UpperLimbLength) - (JointLineDist*JointLineDist);
					// although this shouldn't be ever negative, sometimes Xbox release produces -0.f, causing ProjJointDist to be NaN
					// so now I branch it.
					float ProjJointDist = (ProjJointDistSqr>0.f) ? FMath::Sqrt(ProjJointDistSqr) : 0.f;
					if (bReverseUpperBone)
					{
						ProjJointDist *= -1.f;
					}

					// So now we can work out where to put the joint!
					OutJointPos = RootPos + (ProjJointDist * DesiredDir) + (JointLineDist * JointBendDir);
				}
			}

			// Update transform for upper bone.
			{
				// Get difference in direction for old and new joint orientations
				FVector const OldDir = (InitialJointPos - RootPos).GetSafeNormal();
				FVector const NewDir = (OutJointPos - RootPos).GetSafeNormal();
				// Find Delta Rotation take takes us from Old to New dir
				FQuat const DeltaRotation = FQuat::FindBetween(OldDir, NewDir);
				// Rotate our Joint quaternion by this delta rotation
				UpperLimbCSTransform.SetRotation(DeltaRotation * UpperLimbCSTransform.GetRotation());
				// And put joint where it should be.
				UpperLimbCSTransform.SetTranslation(RootPos);

				// Order important. First bone is upper limb.
				OutBoneTransforms.Add(FBoneTransform(UpperLimbIndex, UpperLimbCSTransform));
			}

			// Update transform for lower bone.
			{
				// Get difference in direction for old and new joint orientations
				FVector const OldDir = (InitialEndPos - InitialJointPos).GetSafeNormal();
				FVector const NewDir = (OutEndPos - OutJointPos).GetSafeNormal();

				// Find Delta Rotation take takes us from Old to New dir
				FQuat const DeltaRotation = FQuat::FindBetween(OldDir, NewDir);
				// Rotate our Joint quaternion by this delta rotation
				LowerLimbCSTransform.SetRotation(DeltaRotation * LowerLimbCSTransform.GetRotation());
				// And put joint where it should be.
				LowerLimbCSTransform.SetTranslation(OutJointPos);

				// Order important. Second bone is lower limb.
				OutBoneTransforms.Add(FBoneTransform(LowerLimbIndex, LowerLimbCSTransform));

			}

			// Update transform for end bone.
			{

				// Set correct location for end bone.
				EndBoneCSTransform.SetTranslation(OutEndPos);

				// Order important. Third bone is End Bone.
				OutBoneTransforms.Add(FBoneTransform(EndBoneIndex, EndBoneCSTransform));
			}

			OutBoneTransforms.Sort([](const FBoneTransform& A, const FBoneTransform& B)
			{
				return A.BoneIndex < B.BoneIndex;
			});

			if (OutBoneTransforms.Num() > 0)
			{
				MeshBases.LocalBlendCSBoneTransforms(OutBoneTransforms, BlendWeight);
				OutBoneTransforms.Empty();
			}

		}

	}

}

bool FAnimNode_MMDIK::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	// if both bones are valid
	//return (IKBone.IsValid(RequiredBones));
	return true;
}

void FAnimNode_MMDIK::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	const FStringAssetReference& AssetRef = MMDExtendAssetRef.ToStringReference();

	UE_LOG(LogAnimation, Warning, TEXT("FAnimNode_MMDIK::InitializeBoneReferences: AssetRef[%s]."), *AssetRef.ToString());

	MMDExtendAssetRef = Cast<UMMDExtendAsset>(StaticLoadObject(UMMDExtendAsset::StaticClass(), NULL, *AssetRef.ToString()));

}
