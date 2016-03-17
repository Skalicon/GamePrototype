// Fill out your copyright notice in the Description page of Project Settings.

#include "Test.h"
#include "HitDetectionNode.h"

HitDetectionNode::HitDetectionNode()
{
}

HitDetectionNode::HitDetectionNode(int socketIndex_import, FVector socketLastTickLocation_import)
{
	socketIndex = socketIndex_import;
	socketLastTickLocation = socketLastTickLocation_import;
}

HitDetectionNode::~HitDetectionNode()
{
}
