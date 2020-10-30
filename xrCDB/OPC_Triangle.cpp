///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains a handy triangle class.
 *	\file		IceTriangle.cpp
 *	\author		Pierre Terdiman
 *	\date		January, 17, 2000
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Precompiled Header
#include "stdafx.h"
#pragma hdrstop

using namespace Meshmerizer;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains an indexed triangle class.
 *
 *	\class		Triangle
 *	\author		Pierre Terdiman
 *	\version	1.0
 *	\date		08.15.98
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Checks whether the triangle is degenerate or not. A degenerate triangle has two common vertex references. This is a zero-area triangle.
 *	\return		true if the triangle is degenerate
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool IndexedTriangle::IsDegenerate() const
{
	if (mVRef[0] == mVRef[1])
		return true;
	if (mVRef[1] == mVRef[2])
		return true;
	if (mVRef[2] == mVRef[0])
		return true;
	return false;
}
