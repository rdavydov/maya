#ifndef _MDagPathArray
#define _MDagPathArray
//-
// ==========================================================================
// Copyright (C) 1995 - 2006 Autodesk, Inc., and/or its licensors.  All
// rights reserved.
//
// The coded instructions, statements, computer programs, and/or related
// material (collectively the "Data") in these files contain unpublished
// information proprietary to Autodesk, Inc. ("Autodesk") and/or its
// licensors,  which is protected by U.S. and Canadian federal copyright law
// and by international treaties.
//
// The Data may not be disclosed or distributed to third parties or be
// copied or duplicated, in whole or in part, without the prior written
// consent of Autodesk.
//
// The copyright notices in the Software and this entire statement,
// including the above license grant, this restriction and the following
// disclaimer, must be included in all copies of the Software, in whole
// or in part, and all derivative works of the Software, unless such copies
// or derivative works are solely in the form of machine-executable object
// code generated by a source language processor.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.
// AUTODESK DOES NOT MAKE AND HEREBY DISCLAIMS ANY EXPRESS OR IMPLIED
// WARRANTIES INCLUDING, BUT NOT LIMITED TO, THE WARRANTIES OF
// NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE,
// OR ARISING FROM A COURSE OF DEALING, USAGE, OR TRADE PRACTICE. IN NO
// EVENT WILL AUTODESK AND/OR ITS LICENSORS BE LIABLE FOR ANY LOST
// REVENUES, DATA, OR PROFITS, OR SPECIAL, DIRECT, INDIRECT, OR
// CONSEQUENTIAL DAMAGES, EVEN IF AUTODESK AND/OR ITS LICENSORS HAS
// BEEN ADVISED OF THE POSSIBILITY OR PROBABILITY OF SUCH DAMAGES.
// ==========================================================================
//+
//
// CLASS:    MDagPathArray
//
// ****************************************************************************
//
// CLASS DESCRIPTION (MDagPathArray)
//
//	Methods are provided for obtaining the DAG Path elements by index, clearing
//	the array, determining the length of the array, and removing, inserting and
//	appending array elements.
//
// ****************************************************************************

#if defined __cplusplus

// ****************************************************************************
// INCLUDED HEADER FILES


#include <maya/MTypes.h>
#include <maya/MStatus.h>

// ****************************************************************************
// DECLARATIONS

class MDagPath;

// ****************************************************************************
// CLASS DECLARATION (MDagPathArray)

//! \ingroup OpenMaya
//! \brief Indexable Array of DAG Paths. 
/*!
Provides methods for manipulating arrays of DAG Paths.

Arrays of DAG Paths are useful for storing and manipluating multiple Paths
to a particular DAG Node.  The DAG Path method MDagPath::getAllPathsTo()
and DAG Node Function Set method MFnDagNode::getAllPaths() implicitly
return an array of DAG Paths.

These arrays may also be used to manage Paths for a number of different
Nodes.

DAG Path arrays are used in conjunction with DAG Paths (MDagPath) and
individual elements of the arrays can be parameters to some methods of the
DAG Node Function Set (MFnDagNode).

Use this DAG Path Array Class to create and manipulate arrays of DAG Paths,
for either a particular DAG Node or a number of different DAG Nodes.

The length of the array adjusts automatically.
*/
class OPENMAYA_EXPORT MDagPathArray
{

public:
					MDagPathArray();
					MDagPathArray(const MDagPathArray& other);
					~MDagPathArray();
	const MDagPath&	operator[]( unsigned int index ) const;
	MDagPathArray& operator=(const MDagPathArray& other );
	MStatus			setLength( unsigned int length );
	unsigned int		length() const;
	MStatus			remove( unsigned int index );
	MStatus			insert( const MDagPath & element, unsigned int index );
	MStatus			append( const MDagPath & element );
	MStatus			clear();

BEGIN_NO_SCRIPT_SUPPORT:

	MDagPath &		operator[]( unsigned int index );

	//!	NO SCRIPT SUPPORT
	friend OPENMAYA_EXPORT std::ostream &operator<<(std::ostream &os,
											   const MDagPathArray &array);

END_NO_SCRIPT_SUPPORT:

protected:
// No protected members

private:
	MDagPathArray ( void * );
	void * arr;
	bool   own;
	static const char* className();
};

#endif /* __cplusplus */
#endif /* _MDagPathArray */
