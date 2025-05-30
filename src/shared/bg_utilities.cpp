/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2014 Unvanquished Development

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Daemon GPL Source is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Unvanquished is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#include "common/Common.h"
#include "bg_public.h"

/**
 * @brief Given an origin, a target point, an initial velocity v0, and a gravity g,
 *        calculate the two possible pitch angles and corresponding directions for
 *        reaching the target on a parabolic trajectory.
 * @return true when angles contains the two possible pitch angles and dir1 and dir2
 *         contain the corresponding directions. false when the target is out of reach.
 */
bool BG_GetTrajectoryPitch( vec3_t origin, vec3_t target, float v0, float g,
                                vec2_t angles, vec3_t dir1, vec3_t dir2 )
{
	vec3_t t3;  // target relative to origin in 3D space
	vec2_t t2;  // relative target in 2D space (ignoring yaw angle)
	float  tmp; // temporary variable
	float  v02; // v0 * v0

	VectorSubtract( target, origin, t3 );

	// abort if the distance is very small
	if ( VectorLength( t3 ) < 0.1f )
	{
		return false;
	}

	v02 = v0 * v0;

	// create a 2D representation of the problem by cutting out the plane that contains
	// the trajectory
	t2[ 1 ] = t3[ 2 ];
	t2[ 0 ] = sqrtf( t3[ 0 ] * t3[ 0 ] + t3[ 1 ] * t3[ 1 ] );

	// calculate the angles
	tmp = v02 * v02 - g * ( g * t2[ 0 ] * t2[ 0 ] + 2.0f * t2[ 1 ] * v02 );

	if ( tmp < 0.0f )
	{
		// target not reachable
		return false;
	}

	tmp = sqrtf( tmp );

	angles[ 0 ] = atanf( ( v02 + tmp ) / ( g * t2[ 0 ] ) );
	angles[ 1 ] = atanf( ( v02 - tmp ) / ( g * t2[ 0 ] ) );

	// calculate the corresponding directions for both angles
	dir1[ 0 ] = dir2[ 0 ] = t3[ 0 ];
	dir1[ 1 ] = dir2[ 1 ] = t3[ 1 ];

	if ( fabs( t3[ 0 ] ) < 0.01f && fabs( t3[ 1 ] ) < 0.01f ) // epsilon to prevent DBZ
	{
		// if target is below or above origin, we already know the direction
		dir1[ 2 ] = dir2[ 2 ] = t3[ 2 ];
	}
	else
	{
		// normalize direction so it can be rotated more easily
		dir1[ 2 ] = dir2[ 2 ] = 0.0f;
		VectorNormalize( dir1 );
		VectorNormalize( dir2 );

		// angles are in ]-pi/2,pi/2[, so cos != 0
		dir1[ 2 ] = sqrtf( 1.0f / cosf( angles[ 0 ] ) - 1.0f );
		dir2[ 2 ] = sqrtf( 1.0f / cosf( angles[ 1 ] ) - 1.0f );

		if ( angles[ 0 ] < 0.0f )
		{
			dir1[ 2 ] = -dir1[ 2 ];
		}

		if ( angles[ 1 ] < 0.0f )
		{
			dir2[ 2 ] = -dir2[ 2 ];
		}
	}

	VectorNormalize( dir1 );
	VectorNormalize( dir2 );

	return true;
}

void BG_BuildEntityDescription( char *str, size_t size, entityState_t *es )
{
	Q_snprintf(str, size, "%s #%i", Com_EntityTypeName( static_cast<entityType_t>( es->eType ) ), es->number );
	str[ size -1 ] = '\0';
}

bool BG_IsMainStructure( buildable_t buildable )
{
	switch ( buildable )
	{
		case BA_A_OVERMIND:
		case BA_H_REACTOR:
			return true;

		default:
			return false;
	}
}


bool BG_IsMainStructure( entityState_t *es )
{
	if ( es->eType != entityType_t::ET_BUILDABLE ) return false;

	return BG_IsMainStructure( (buildable_t)es->modelindex );
}

/**
 * @brief Moves a point from the origin of a bounding box to its center.
 * @param point The origin, will be modified.
 * @param mins BBOX mins.
 * @param maxs BBOX maxs.
 */
void BG_MoveOriginToBBOXCenter( vec3_t point, const vec3_t mins, const vec3_t maxs )
{
	point[ 0 ] = point[ 0 ] + ( maxs[ 0 ] + mins[ 0 ] ) * 0.5f;
	point[ 1 ] = point[ 1 ] + ( maxs[ 1 ] + mins[ 1 ] ) * 0.5f;
	point[ 2 ] = point[ 2 ] + ( maxs[ 2 ] + mins[ 2 ] ) * 0.5f;
}

void BG_MoveOriginToBBOXCenter( glm::vec3& point, glm::vec3 const& mins, glm::vec3 const& maxs )
{
	point = point + ( maxs + mins ) * 0.5f;
}

void ModifyFlag(int &flags, int flag, bool value) {
	if (value) {
		flags |= flag;
	} else {
		flags &= ~flag;
	}
}

void AddFlag(int &flags, int flag) {
	ModifyFlag(flags, flag, true);
}

void RemoveFlag(int &flags, int flag) {
	ModifyFlag(flags, flag, false);
}

void ToggleFlag(int &flags, int flag) {
	flags ^= flag;
}

// imported from daemon.
// Given 3 degree angles (yaw, pitch and roll) computes three vectors that
// point forwards, rightwards and upwards.
//
// The vectors are orthogonal and normalized.
//
// NOTES:
// * it would likely be more efficient to work with a quaternion here.
// * working with a quaternion would also allow to return a value
void AngleVectors( const glm::vec3 &angles, glm::vec3 *forward, glm::vec3 *right, glm::vec3 *up )
{
	float        angle;
	static float sr, sp, sy, cr, cp, cy;

	// static to help MS compiler fp bugs

	angle = angles[ YAW ] * ( M_PI * 2 / 360 );
	sy = sin( angle );
	cy = cos( angle );

	angle = angles[ PITCH ] * ( M_PI * 2 / 360 );
	sp = sin( angle );
	cp = cos( angle );

	angle = angles[ ROLL ] * ( M_PI * 2 / 360 );
	sr = sin( angle );
	cr = cos( angle );

	if ( forward )
	{
		(*forward)[0] = cp * cy;
		(*forward)[1] = cp * sy;
		(*forward)[2] = -sp;
	}

	if ( right )
	{
		(*right)[0] = ( -1 * sr * sp * cy + -1 * cr * -sy );
		(*right)[1] = ( -1 * sr * sp * sy + -1 * cr * cy );
		(*right)[2] = -1 * sr * cp;
	}

	if ( up )
	{
		(*up)[0] = ( cr * sp * cy + -sr * -sy );
		(*up)[1] = ( cr * sp * sy + -sr * cy );
		(*up)[2] = cr * cp;
	}
}

// This is a drop in replacement for AnglesToAxis
WARN_UNUSED_RESULT glm::mat3 RotationMatrix( const glm::vec3 &angles ) {
	glm::mat3 m;
	AngleVectors( angles, &m[0], &m[1], &m[2] );
	m[1] *= -1.0f;
	return m;
}

glm::vec3 ProjectPointOntoVectorBounded(const glm::vec3 &point, const glm::vec3 &vStart, const glm::vec3 &vEnd)
{
	glm::vec3 pVec = point - vStart;
	glm::vec3 vec = vEnd - vStart;
	VectorNormalize( vec );
	// project onto the directional vector for this segment
	glm::vec3 vProj = vStart + glm::dot(pVec, vec) * vec;

	// check bounds
	int j;
	for (j = 0; j < 3; j++)
	{
		if ((vProj[j] > vStart[j] && vProj[j] > vEnd[j] ) || ( vProj[j] < vStart[j] && vProj[j] < vEnd[j]))
		{
			break;
		}
	}

	if (j < 3)
	{
		if (Q_fabs(vProj[j] - vStart[j]) < Q_fabs(vProj[j] - vEnd[j]))
		{
			return vStart;
		}

		else
		{
			return vEnd;
		}
	}

	return vProj;
}

glm::vec3 ProjectPointOnPlane(const glm::vec3 &point, const glm::vec3 &normal)
{
	return point - glm::dot(point, normal) * normal;
}

// Assumes "vec" is normalized.
glm::vec3 PerpendicularVector(const glm::vec3 &vec)
{
	int pos, i;
	float minelem = 1.0f;

	/*
	 * * find the smallest magnitude axially aligned vector
	 */
	for (pos = 0, i = 0; i < 3; i++)
	{
		if (Q_fabs(vec[i]) < minelem)
		{
			pos = i;
			minelem = Q_fabs(vec[i]);
		}
	}

	glm::vec3 tempvec = {0.0f, 0.0f, 0.0f};
	tempvec[pos] = 1.0f;

	glm::vec3 result = ProjectPointOnPlane(tempvec, vec);
	VectorNormalize(result);
	return result;
}

glm::vec3 vectoangles(const glm::vec3 &vec)
{
	float yaw, pitch;

	if (vec[1] == 0.0f && vec[0] == 0.0f)
	{
		yaw = 0.0f;

		if (vec[2] > 0.0f)
		{
			pitch = 90.0f;
		}

		else
		{
			pitch = 270.0f;
		}
	}

	else
	{
		if (vec[0] != 0.0f)
		{
			yaw = RAD2DEG(atan2f(vec[1], vec[0]));
		}
		else if (vec[1] > 0.0f)
		{
			yaw = 90.0f;
		}
		else
		{
			yaw = 270.0f;
		}

		if (yaw < 0.0f)
		{
			yaw += 360.0f;
		}

		float forward = sqrtf(vec[0] * vec[0] + vec[1] * vec[1]);
		pitch = RAD2DEG(atan2f(vec[2], forward));

		if (pitch < 0.0f)
		{
			pitch += 360.0f;
		}
	}

	return {-pitch, yaw, 0.0f};
}

glm::vec3 AnglesSubtract(const glm::vec3 &u, const glm::vec3 &v)
{
	return {
		AngleSubtract(u[0], v[0]),
		AngleSubtract(u[1], v[1]),
		AngleSubtract(u[2], v[2]),
	};
}
