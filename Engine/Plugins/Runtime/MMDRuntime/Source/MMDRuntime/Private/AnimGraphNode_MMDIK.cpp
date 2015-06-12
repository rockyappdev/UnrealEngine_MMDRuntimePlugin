// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MMDRuntimePrivatePCH.h"

#include "AnimGraphNode_MMDIK.h"


#define LOCTEXT_NAMESPACE "A3Nodes"

/////////////////////////////////////////////////////
// UAnimGraphNode_MMDIK


UAnimGraphNode_MMDIK::UAnimGraphNode_MMDIK(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FText UAnimGraphNode_MMDIK::GetControllerDescription() const
{
	return LOCTEXT("MMDIK", "MMDIK");
}

FText UAnimGraphNode_MMDIK::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_MMDIK_Tooltip", "MMD IK control applies an inverse kinematic (IK) solver for MMD imported Skeleton.");
}

FText UAnimGraphNode_MMDIK::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return GetControllerDescription();
}

#undef LOCTEXT_NAMESPACE
