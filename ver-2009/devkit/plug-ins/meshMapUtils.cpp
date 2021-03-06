//-
// ==========================================================================
// Copyright 1995,2006,2008 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk
// license agreement provided at the time of installation or download,
// or which otherwise accompanies this software in either electronic
// or hard copy form.
// ==========================================================================
//+

////////////////////////////////////////////////////////////////////////
// 
// meshMapUtils.cpp
// 
// Description:
//    Utility functions for the meshRemap and meshReorder commands
//       moveToolContext
// 
////////////////////////////////////////////////////////////////////////

#include <assert.h>

#include "meshMapUtils.h"

#include <maya/MIntArray.h>
#include <maya/MFnMesh.h>
#include <maya/MItMeshEdge.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MGlobal.h>
#include <maya/MFloatPointArray.h>
#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MObjectArray.h>
#include <maya/MItMeshVertex.h>


//
// Modulating index, will return 0..l-1, for any integer value
#define IDX(i, l) ( ( i + l ) % l )

//
// Intersect the current face list with the one provided, keep only the common elements
// 
void meshMapUtils::intersectArrays( MIntArray& selection, MIntArray& moreFaces )
{
	int j = selection.length() - 1;
	while ( j >= 0 )
	{
		bool found = false;;

		// For each item in moreFaces, try find it in fSelectedFaces
		for( unsigned int i = 0 ; i < moreFaces.length(); i++ )
		{
			if( selection[j] == moreFaces[i] )
			{
				found = true;
				break;
			}
		}	

		if( !found )
		{
			selection.remove(j);
		}

		j--;
	}
}


//
//  Recursively traverse a mesh by processing each face, and the neighbouring faces along it's edges.
//  The initial invocation of this routine provides the basis for the new first vertex/edge
//  and faces.  
//
//  The result of this routine is an array of values that map the old CV indices to the new ones. Along
//  the a new list of reindexed CVs is built, along with a list of poly counts and connetions. These
//  can be used to build a new mesh with the reordering specfied by the seed face and vertices.
//
//
//  Inputs: 
//		path				: Path to the object being traversed
//		faceIdx				: Current face being traversed
//		v0, v1				: Veretices that define the direction of travel along the face
//		faceTraversal		: An array booleans to track which faces have been 
//							: traversed, controls the recursion  
//		origVertices		: The vertices from the original mesh. The could be obtained
//							: from the path, but are passed in for efficiency
//
// 	Outputs:
//		cvMapping			: Mapping of the existing vertices to their new indices
//							: the fist values in the final array will be the intial v0, v1
//		cvMappingInverse	: The inverse of the cvMapping
//							: the value of items v0 and v1 will be 0 and 1 respectively
//		newPolygonCounts	: Vertex counts for each of the new faces
//		newPolygonConnects  : Connections, specified in terms of new CV indices
//		newVertices			: The orginal vertices resorted based on the reindexing
//
//
MStatus meshMapUtils::traverseFace( 
	MDagPath&	path,
	int faceIdx, 
	int v0, 
	int v1, 
	MIntArray& faceTraversal,
	MIntArray& cvMapping,
	MIntArray& cvMappingInverse,
	MIntArray& newPolygonCounts,
	MIntArray& newPolygonConnects,
	MFloatPointArray& origVertices,
	MFloatPointArray& newVertices
)
{
	int vtxCnt = -1;
	int dir = 0;
	int dummy;		// For setIndex calls

	MStatus stat = MStatus::kSuccess;

	MFnMesh theMesh( path, &stat );
	MItMeshPolygon polyIt( path );
	MItMeshEdge edgeIt( path );

	if( stat != MStatus::kSuccess )
	{
		MGlobal::displayError( " theMesh.getPoint failed");
		return stat;
	}


	//
	// Skip over any faces already processed, this is not a failure
	// 
	if( faceTraversal[faceIdx] )
	{
		return MStatus::kSuccess;
	}

	//
	// get the vertex/edge information and sort it based on the user seed
	//
	MIntArray vtxOrig;
	MIntArray edgeOrig;

	polyIt.setIndex( faceIdx, dummy );
	polyIt.getEdges( edgeOrig );	   
	polyIt.getVertices( vtxOrig );	   

	vtxCnt = vtxOrig.length();

	// 
	// the sorted V/E info
	// 
	MIntArray vtxSorted( vtxCnt );
	MIntArray edgeSorted( vtxCnt );

	//
	// Build a new array ordered with v0, then v1, first figure out the 
	// starting point, and direction 
	//
	int v0Idx =	-1;
	int i;
	for( i = 0; i < vtxCnt; i++ )
	{
		if( vtxOrig[i] == v0 )
		{
			// We've found v0, now find in what direction we need to travel to find v1
					
			v0Idx =	i;
			
			if( vtxOrig[IDX(i+1, vtxCnt)] == v1 )
			{
				dir = 1;
			}
			else if( vtxOrig[IDX(i-1, vtxCnt)] == v1 )
			{
				dir = -1;
			}
			else
			{
				assert( dir != 0 );
			}	
			break;
		}
	}

	// Now sort the vertex/edge arrays
	for( i = 0; i < vtxCnt; i++ )
	{
		vtxSorted[i] = vtxOrig[IDX( v0Idx + i * dir, vtxCnt )];
		if( dir == 1 )
		{
			edgeSorted[i] = edgeOrig[IDX( v0Idx + i * dir, vtxCnt )];
		}
		else
		{
			edgeSorted[i] = edgeOrig[IDX( v0Idx - 1 + i * dir, vtxCnt )];
		}
	}


	// Add any new CVs to the vertex array being constructed
	for ( i = 0; i < vtxCnt; i++ )
	{
		MPoint pos;
		int index = vtxSorted[i];

		if( cvMapping[index] == -1  )
		{
			if( stat != MStatus::kSuccess )
			{
				MGlobal::displayError( " theMesh.getPoint failed");
				return stat;
			}

			// Added the new CV, and mark it as transferred
			newVertices.append( origVertices[index] );

			// Store the mapping from the old CV indices to the new ones
			cvMapping[index] = newVertices.length()-1;
			cvMappingInverse[newVertices.length()-1] = index;
		}

	}

	//
	//  Add the new face count 
	//
	newPolygonCounts.append( vtxCnt );

	//
	//  Add the new polyConnects
	
	for ( i = 0; i < vtxCnt; i++ )
	{
		newPolygonConnects.append( cvMapping[vtxSorted[i]] );
	}

	// Mark this face as complete 
	faceTraversal[faceIdx] = true;

	//
	//  Now recurse over the edges of this face
	// 
	for( i = 0; i < (int)edgeSorted.length(); i++ )
	{
		int nextEdge = edgeSorted[i];

		int2 nextEdgeVtx;
		stat = theMesh.getEdgeVertices(nextEdge, nextEdgeVtx );

		//
		// Find the vertex, in the sorted array, that starts the next edge
		int baseIdx = -1;
		bool swap = false;
		int j;
		for( j = 0; j < (int)vtxSorted.length(); j++ )
		{
			if( vtxSorted[j] == nextEdgeVtx[0] )
			{
				baseIdx = j;
				break;
			}
		}

		assert( baseIdx != -1 );
	
		// 
		// Now look forward and backward in the vertex array to find the
		// edge's other point, this indicates the edges direction. This
		// is needed to guide the next recursion level, and keep the
		// normals pointed consistenly
		//
		if( vtxSorted[IDX(j+1, vtxCnt)] == nextEdgeVtx[1] )
		{
			// Nothing
		}
		else if ( vtxSorted[IDX(j-1, vtxCnt)] == nextEdgeVtx[1] )
		{
			swap = true;
		}


		MIntArray connectedFaces;
		edgeIt.setIndex( nextEdge, dummy );
		edgeIt.getConnectedFaces( connectedFaces );

		// A single face is simply the current one. Recurse over the others
		if( connectedFaces.length() > 1 )
		{
			int nextFace;
			if( connectedFaces[0] == faceIdx )
			{
			   	nextFace = connectedFaces[1];
			}
			else
			{
			   	nextFace = connectedFaces[0];
			}

			int nextVtx0 = -1;
			int nextVtx1 = -1;
			if ( !swap )
			{
				nextVtx0 = nextEdgeVtx[1];
				nextVtx1 = nextEdgeVtx[0];
			}
			else
			{
				nextVtx0 = nextEdgeVtx[0];
				nextVtx1 = nextEdgeVtx[1];
			}

			stat = traverseFace( path, nextFace, nextVtx0, nextVtx1, faceTraversal,
					cvMapping, cvMappingInverse, 
					newPolygonCounts, newPolygonConnects, 
					origVertices, newVertices );

			// Break out of edge loop on error
			if( stat != MStatus::kSuccess )
			{
				break;
			}	
		}
	}

	return stat;
}

//
//  Given a list of 3 paths and compoents find the face they define and store their indices
//
MStatus meshMapUtils::validateFaceSelection( MDagPathArray& paths, MObjectArray& components, int *faceIdx, MIntArray *seedVtx )
{
	MStatus stat = MStatus::kSuccess;
	MIntArray finalFaceList;

	if ( paths.length() != 3 && components.length() != 3 )
	{
		return MStatus::kFailure;
	}

	seedVtx->clear();

	for (unsigned int i = 0; i < 3; i++)
	{
		MItMeshVertex fIt ( paths[i], components[i], &stat );
		if( stat != MStatus::kSuccess || fIt.isDone() )
		{
			MGlobal::displayError( " MItMeshVertex failed");
			return MStatus::kFailure;
		}

		seedVtx->append( fIt.index() ); // The  cv's location

		MIntArray  faceList;
		stat = fIt.getConnectedFaces ( faceList );
		if( stat != MStatus::kSuccess )
		{
			MGlobal::displayError( " getConnectedFaces failed");
			return MStatus::kFailure;
		}

		if( i == 0 )
		{
			finalFaceList = faceList;
		}
		else
		{
			meshMapUtils::intersectArrays( finalFaceList, faceList );
		}
	}

	if( finalFaceList.length() != 1 )
	{
		return MStatus::kFailure;
	}
	else
	{
		*faceIdx = finalFaceList[0];
	}

	MDagPath p0( paths[0] );
	MDagPath p1( paths[1] );
	MDagPath p2( paths[2] );

	if( !(p0 == p1 ) || !(p0 == p2 ))
	{
		return MStatus::kFailure;
	}

	return MStatus::kSuccess;
}
