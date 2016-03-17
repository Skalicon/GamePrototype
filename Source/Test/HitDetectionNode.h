// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

/**
 * 
 */
class TEST_API HitDetectionNode
{
public:
	HitDetectionNode();
	HitDetectionNode(int, FVector);
	~HitDetectionNode();

	int socketIndex;
	FVector socketLastTickLocation;

};
