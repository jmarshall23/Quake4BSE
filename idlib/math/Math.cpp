/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#pragma hdrstop
#include "precompiled.h"

const int SMALLEST_NON_DENORMAL					= 1 << IEEE_FLT_MANTISSA_BITS;
const int NAN_VALUE								= 0x7f800000;

const float	idMath::PI				= 3.14159265358979323846f;
const float	idMath::TWO_PI			= 2.0f * PI;
const float	idMath::HALF_PI			= 0.5f * PI;
const float	idMath::ONEFOURTH_PI	= 0.25f * PI;
const float idMath::ONEOVER_PI		= 1.0f / idMath::PI;
const float idMath::ONEOVER_TWOPI	= 1.0f / idMath::TWO_PI;
const float idMath::E				= 2.71828182845904523536f;
const float idMath::SQRT_TWO		= 1.41421356237309504880f;
const float idMath::SQRT_THREE		= 1.73205080756887729352f;
const float	idMath::SQRT_1OVER2		= 0.70710678118654752440f;
const float	idMath::SQRT_1OVER3		= 0.57735026918962576450f;
const float	idMath::M_DEG2RAD		= PI / 180.0f;
const float	idMath::M_RAD2DEG		= 180.0f / PI;
const float	idMath::M_SEC2MS		= 1000.0f;
const float	idMath::M_MS2SEC		= 0.001f;
const float	idMath::INFINITY		= 1e30f;
const float idMath::FLT_EPSILON		= 1.192092896e-07f;
const float idMath::FLT_SMALLEST_NON_DENORMAL	= * reinterpret_cast< const float* >( & SMALLEST_NON_DENORMAL );	// 1.1754944e-038f

#if defined(USE_INTRINSICS)
const __m128 idMath::SIMD_SP_zero				= { 0.0f, 0.0f, 0.0f, 0.0f };
const __m128 idMath::SIMD_SP_255				= { 255.0f, 255.0f, 255.0f, 255.0f };
const __m128 idMath::SIMD_SP_min_char			= { -128.0f, -128.0f, -128.0f, -128.0f };
const __m128 idMath::SIMD_SP_max_char			= { 127.0f, 127.0f, 127.0f, 127.0f };
const __m128 idMath::SIMD_SP_min_short			= { -32768.0f, -32768.0f, -32768.0f, -32768.0f };
const __m128 idMath::SIMD_SP_max_short			= { 32767.0f, 32767.0f, 32767.0f, 32767.0f };
const __m128 idMath::SIMD_SP_smallestNonDenorm	= { FLT_SMALLEST_NON_DENORMAL, FLT_SMALLEST_NON_DENORMAL, FLT_SMALLEST_NON_DENORMAL, FLT_SMALLEST_NON_DENORMAL };
const __m128 idMath::SIMD_SP_tiny				= { 1e-4f, 1e-4f, 1e-4f, 1e-4f };
const __m128 idMath::SIMD_SP_rsqrt_c0			= { 3.0f, 3.0f, 3.0f, 3.0f };
const __m128 idMath::SIMD_SP_rsqrt_c1			= { -0.5f, -0.5f, -0.5f, -0.5f };
#endif

bool		idMath::initialized		= false;
dword		idMath::iSqrt[SQRT_TABLE_SIZE];		// inverse square root lookup table

/*
===============
idMath::Init
===============
*/
void idMath::Init()
{
	union _flint fi, fo;

	for( int i = 0; i < SQRT_TABLE_SIZE; i++ )
	{
		fi.i	 = ( ( EXP_BIAS - 1 ) << EXP_POS ) | ( i << LOOKUP_POS );
		fo.f	 = ( float )( 1.0 / sqrt( fi.f ) );
		iSqrt[i] = ( ( dword )( ( ( fo.i + ( 1 << ( SEED_POS - 2 ) ) ) >> SEED_POS ) & 0xFF ) ) << SEED_POS;
	}

	iSqrt[SQRT_TABLE_SIZE / 2] = ( ( dword )( 0xFF ) ) << ( SEED_POS );

	initialized = true;
}

/*
================
idMath::FloatToBits
================
*/
int idMath::FloatToBits( float f, int exponentBits, int mantissaBits )
{
	int i, sign, exponent, mantissa, value;

	assert( exponentBits >= 2 && exponentBits <= 8 );
	assert( mantissaBits >= 2 && mantissaBits <= 23 );

	int maxBits = ( ( ( 1 << ( exponentBits - 1 ) ) - 1 ) << mantissaBits ) | ( ( 1 << mantissaBits ) - 1 );
	int minBits = ( ( ( 1 <<   exponentBits ) - 2 ) << mantissaBits ) | 1;

	float max = BitsToFloat( maxBits, exponentBits, mantissaBits );
	float min = BitsToFloat( minBits, exponentBits, mantissaBits );

	if( f >= 0.0f )
	{
		if( f >= max )
		{
			return maxBits;
		}
		else if( f <= min )
		{
			return minBits;
		}
	}
	else
	{
		if( f <= -max )
		{
			return ( maxBits | ( 1 << ( exponentBits + mantissaBits ) ) );
		}
		else if( f >= -min )
		{
			return ( minBits | ( 1 << ( exponentBits + mantissaBits ) ) );
		}
	}

	exponentBits--;
	i = *reinterpret_cast<int*>( &f );
	sign = ( i >> IEEE_FLT_SIGN_BIT ) & 1;
	exponent = ( ( i >> IEEE_FLT_MANTISSA_BITS ) & ( ( 1 << IEEE_FLT_EXPONENT_BITS ) - 1 ) ) - IEEE_FLT_EXPONENT_BIAS;
	mantissa = i & ( ( 1 << IEEE_FLT_MANTISSA_BITS ) - 1 );
	value = sign << ( 1 + exponentBits + mantissaBits );
	value |= ( ( INT32_SIGNBITSET( exponent ) << exponentBits ) | ( abs( exponent ) & ( ( 1 << exponentBits ) - 1 ) ) ) << mantissaBits;
	value |= mantissa >> ( IEEE_FLT_MANTISSA_BITS - mantissaBits );
	return value;
}

/*
================
idMath::BitsToFloat
================
*/
float idMath::BitsToFloat( int i, int exponentBits, int mantissaBits )
{
	static int exponentSign[2] = { 1, -1 };
	int sign, exponent, mantissa, value;

	assert( exponentBits >= 2 && exponentBits <= 8 );
	assert( mantissaBits >= 2 && mantissaBits <= 23 );

	exponentBits--;
	sign = i >> ( 1 + exponentBits + mantissaBits );
	exponent = ( ( i >> mantissaBits ) & ( ( 1 << exponentBits ) - 1 ) ) * exponentSign[( i >> ( exponentBits + mantissaBits ) ) & 1];
	mantissa = ( i & ( ( 1 << mantissaBits ) - 1 ) ) << ( IEEE_FLT_MANTISSA_BITS - mantissaBits );
	value = sign << IEEE_FLT_SIGN_BIT | ( exponent + IEEE_FLT_EXPONENT_BIAS ) << IEEE_FLT_MANTISSA_BITS | mantissa;
	return *reinterpret_cast<float*>( &value );
}


// ================================================================================================
// jscott: fast and reliable random routines
// ================================================================================================

unsigned long rvRandom::mSeed;

float rvRandom::flrand(float min, float max)
{
	float	result;

	mSeed = (mSeed * 214013L) + 2531011;
	// Note: the shift and divide cannot be combined as this breaks the routine
	result = (float)(mSeed >> 17);						// 0 - 32767 range
	result = ((result * (max - min)) * (1.0f / 32768.0f)) + min;
	return(result);
}

float rvRandom::flrand() {
	return flrand(0.0f, 1.0f);
}

float rvRandom::flrand(const idVec2& v) {
	return flrand(v[0], v[1]);
}

int rvRandom::irand(int min, int max)
{
	int		result;

	max++;
	mSeed = (mSeed * 214013L) + 2531011;
	result = mSeed >> 17;
	result = ((result * (max - min)) >> 15) + min;
	return(result);
}

// Try to get a seed independent of the random number system

int rvRandom::Init(void)
{
	mSeed *= (unsigned long)sys->Milliseconds();

	return(mSeed);
}

// ================================================================================================
// Barycentric texture coordinate functions
// Get the *SIGNED* area of a triangle required for barycentric
// ================================================================================================
float idMath::BarycentricTriangleArea(const idVec3& normal, const idVec3& a, const idVec3& b, const idVec3& c)
{
	idVec3	v1, v2;
	idVec3	cross;
	float	area;

	v1 = b - a;
	v2 = c - a;
	cross = v1.Cross(v2);
	area = 0.5f * (cross * normal);

	return(area);
}

void idMath::BarycentricEvaluate(idVec2& result, const idVec3& point, const idVec3& normal, const float area, const idVec3 t[3], const idVec2 tc[3])
{
	float	b1, b2, b3;

	float scale = 1.f;

	scale /= area;

	b1 = idMath::BarycentricTriangleArea(normal, point, t[1], t[2]);
	b2 = idMath::BarycentricTriangleArea(normal, t[0], point, t[2]);
	b3 = idMath::BarycentricTriangleArea(normal, t[0], t[1], point);

	result[0] = ((b1 * tc[0][0]) + (b2 * tc[1][0]) + (b3 * tc[2][0])) * scale;
	result[1] = ((b1 * tc[0][1]) + (b2 * tc[1][1]) + (b3 * tc[2][1])) * scale;
}

void idMath::BarycentricEvaluate(idVec2& result, const idVec3& point, const idVec3& normal, const float area, const idVec3 t[3], const short tc[3][2], float scale) {
	float	b1, b2, b3;

	scale /= area;

	b1 = idMath::BarycentricTriangleArea(normal, point, t[1], t[2]);
	b2 = idMath::BarycentricTriangleArea(normal, t[0], point, t[2]);
	b3 = idMath::BarycentricTriangleArea(normal, t[0], t[1], point);

	result[0] = ((b1 * tc[0][0]) + (b2 * tc[1][0]) + (b3 * tc[2][0])) * scale;
	result[1] = ((b1 * tc[0][1]) + (b2 * tc[1][1]) + (b3 * tc[2][1])) * scale;
}
