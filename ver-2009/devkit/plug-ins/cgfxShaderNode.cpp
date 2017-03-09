//
// Copyright (C) 2002-2004 NVIDIA 
// 
// File: cgfxShaderNode.cpp
//
// Dependency Graph Node: cgfxShader
//
// Author: Jim Atkinson
//
// Changes:
//  10/2003  Kurt Harriman - www.octopusgraphics.com +1-415-893-1023
//           - Multiple UV sets; user-specified texcoord assignment.
//           - "tcs/texCoordSource", a new static attribute, is a 
//             string array of up to 32 elements.  Set it to specify
//             the source of each TEXCOORD vertex parameter as one of:
//             a UV set name; "tangent"; "binormal"; "normal"; an empty
//             string; or up to 4 float values "x y z w".  Default is
//             {"map1","tangent","binormal"}.
//           - "-mtc/maxTexCoords" flag of cgfxShader command returns an
//             upper bound on the number of texcoord inputs per vertex
//             (GL_MAX_TEXTURE_UNITS) that can be passed from Maya thru
//             OpenGL to vertex shaders on the current workstation.
//           - The MEL command `pluginInfo -q -version cgfxShader` 
//             returns the plug-in version and cgfxShaderNode.cpp
//             compile date.
//           - Improved error handling.
//  12/2003  Kurt Harriman - www.octopusgraphics.com +1-415-893-1023
//           - To load or reload an effect, use the cgfxShader command
//             "-fx/fxFile <filename>" flag.  Setting the cgfxShader
//             node's "s/shader" attribute no longer loads the effect.
//           - To choose a technique, set the "t/technique" 
//             attribute of the cgfxShader node.  The effect is not 
//             reloaded.  There is no longer a message box requiring
//             the user to choose a technique when loading an effect.
//           - The techniques defined by the current effect are returned
//             by the cgfxShader command "-lt/-listTechniques" flag.
//           - Fixed incorrect transformation of direction/position 
//             parameters to spaces other than world space.
//           - Dangling references to deleted dynamic attributes
//             caused exceptions in MObject destructor, terminating
//             the Maya process.  This has been fixed.
//           - Improved error handling.
//
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
#ifndef CGFXSHADER_VERSION  
#define CGFXSHADER_VERSION  "4.4"
#endif

#include "cgfxShaderNode.h"
#include "cgfxFindImage.h"

#include <maya/MDagPath.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MEventMessage.h>
#include <maya/MFloatVector.h>
#include <maya/MFnMesh.h>
#include <maya/MFnStringArrayData.h>
#include <maya/MFnStringData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MPlug.h>
#include <maya/MPoint.h>
#include <maya/MVector.h>
#include <maya/MDGModifier.h>
#include <maya/MFileIO.h>
#include <maya/MNodeMessage.h>
#include <maya/MFileObject.h>

#if defined(_SWATCH_RENDERING_SUPPORTED_)
	// For swatch rendering
	#include <maya/MHardwareRenderer.h>
	#include <maya/MGeometryData.h>
	#include <maya/MHWShaderSwatchGenerator.h>
#endif
	#include <maya/MImage.h>

#include "nv_dds.h"


#ifdef _WIN32
#else
#	include <sys/timeb.h>
#	include <string.h>
#
#   define stricmp strcasecmp
#   define strnicmp strncasecmp
#endif


//
// Statics and globals...
//

PFNGLCLIENTACTIVETEXTUREARBPROC glStateCache::glClientActiveTexture = 0;
PFNGLVERTEXATTRIBPOINTERARBPROC glStateCache::glVertexAttribPointer = 0;
PFNGLENABLEVERTEXATTRIBARRAYARBPROC glStateCache::glEnableVertexAttribArray = 0;
PFNGLDISABLEVERTEXATTRIBARRAYARBPROC glStateCache::glDisableVertexAttribArray = 0;
PFNGLVERTEXATTRIB4FARBPROC glStateCache::glVertexAttrib4f = 0;
PFNGLSECONDARYCOLORPOINTEREXTPROC glStateCache::glSecondaryColorPointer = 0;
PFNGLSECONDARYCOLOR3FEXTPROC glStateCache::glSecondaryColor3f = 0;
PFNGLMULTITEXCOORD4FARBPROC glStateCache::glMultiTexCoord4fARB = 0;

int glStateCache::sMaxTextureUnits = 0;

glStateCache::glStateCache()
{ 
	reset(); 
}


glStateCache glStateCache::gInstance;

void glStateCache::activeTexture( int i) 
{ 
	if( i != fActiveTextureUnit) 
	{ 
		fActiveTextureUnit = i; 
		if( glStateCache::glClientActiveTexture) 
			glStateCache::glClientActiveTexture( GL_TEXTURE0_ARB + i );
		}
	}

void glStateCache::enableVertexAttrib( int i)
{
	if( !(fEnabledRegisters & (1 << (glRegister::kVertexAttrib + i)))) 
	{ 
		if( glStateCache::glEnableVertexAttribArray) 
			glStateCache::glEnableVertexAttribArray( i);
		fEnabledRegisters |= (1 << (glRegister::kVertexAttrib + i)); 
	} 
	fRequiredRegisters |= (1 << (glRegister::kVertexAttrib + i));
}


void glStateCache::flushState() 
{ 
	// Work out which registers are enabled, but no longer required
	long redundantRegisters = fEnabledRegisters & ~fRequiredRegisters;
	//printf( "State requires %d, enabled %d, redundant %d\n", fRequiredRegisters, fEnabledRegisters, redundantRegisters);

	// Disable them
	if( redundantRegisters & (1 << glRegister::kPosition)) 
		glDisableClientState(GL_VERTEX_ARRAY); 
	if( redundantRegisters & (1 << glRegister::kNormal)) 
		glDisableClientState(GL_NORMAL_ARRAY); 
	if( redundantRegisters & (1 << glRegister::kColor)) 
		glDisableClientState(GL_COLOR_ARRAY); 
	if( redundantRegisters & (1 << glRegister::kSecondaryColor)) 
		glDisableClientState(GL_SECONDARY_COLOR_ARRAY_EXT); 
	for( int i = glRegister::kTexCoord; i <= glRegister::kLastTexCoord; i++)
	{
		if( redundantRegisters & (1 << i)) 
		{
			activeTexture( i - glRegister::kTexCoord); 
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}
	}
	for( int i = glRegister::kVertexAttrib; i <= glRegister::kLastVertexAttrib; i++)
	{
		if( redundantRegisters & (1 << i)) 
		{
			if( glStateCache::glDisableVertexAttribArray) 
				glStateCache::glDisableVertexAttribArray( i - glRegister::kVertexAttrib);
		}
	}
	fEnabledRegisters = fRequiredRegisters;
	fRequiredRegisters = 0;
}


// This typeid must be unique across the universe of Maya plug-ins.  
//
// TODO: Get a unique ID from NVIDIA if they have them or from A|W
// if they do not.
//

#ifdef _WIN32
MTypeId     cgfxShaderNode::sId( 4084862000 );
#else
MTypeId     cgfxShaderNode::sId( 0xF37A0C30 );
#endif

CGcontext		cgfxShaderNode::sCgContext;

// Attribute declarations
// 
MObject     cgfxShaderNode::sShader;
MObject     cgfxShaderNode::sTechnique;
MObject			cgfxShaderNode::sAttributeList;
MObject		cgfxShaderNode::sVertexAttributeList;
MObject     cgfxShaderNode::sVertexAttributeSource;
MObject     cgfxShaderNode::sTexCoordSource;
MObject     cgfxShaderNode::sColorSource;
MObject     cgfxShaderNode::sTexturesByName;


// Codes used in ftexCoordList array
enum ETexCoord 
{       
	etcNull      = -1, 
	etcConstant  = -2, 
	etcNormal    = -3,
	etcTangent   = -4,
	etcBinormal  = -5,
	etcDataSet	 = -6,
};



//--------------------------------------------------------------------//
// Constructor:
//
cgfxShaderNode::cgfxShaderNode()
:	fEffect(0)
,	fAttrDefList(0)
,	fVertexAttributes( NULL)
#ifdef TEXTURES_BY_NAME
,	fTexturesByName( true )
#else
,	fTexturesByName( false )
#endif
,   fNormalsPerVertex( 3 )
,   fConstructed(false)
,   fErrorCount( 0 )
,   fErrorLimit( 8 )
,	fTechniqueHasBlending( false )
,	fShaderFxFile()
,	fShaderFxFileChanged( false )
{
	// Set texCoordSource attribute to its default value.
	MStringArray sa;
	sa.append( "map1" );      
	sa.append( "tangent" );
	sa.append( "binormal" );
	MStringArray sa2;
	sa2.append( "colorSet1" );
	setDataSources( &sa, &sa2 );
}


// Post-constructor
void
cgfxShaderNode::postConstructor()
{
	fConstructed = true;               // ok to call MPxNode member functions 
}                                      // cgfxShaderNode::postConstructor


// Destructor:
//
cgfxShaderNode::~cgfxShaderNode()
{
#ifdef KH_DEBUG
	MString ss = "  .. ~node ";
	if ( fConstructed )
	{
		MFnDependencyNode fnNode( thisMObject() );
		ss += fnNode.name();
	}
	ss += "\n";
	::OutputDebugString( ss.asChar() );
#endif

	// Remove all the callbacks that we registered.
	MMessage::removeCallbacks( fCallbackIds );
	fCallbackIds.clear();

	if (fAttrDefList)
	{
		fAttrDefList->release();
		fAttrDefList = 0;
	}

	// Make sure that any loaded shader is unloaded and freed
	// before going away.
	//
	if (fEffect)
	{
		cgDestroyEffect(fEffect);
		fEffect = 0;
	}
}

//
//	Description:
//		This method computes the value of the given output plug based
//		on the values of the input attributes.
//
//	Arguments:
//		plug - the plug to compute
//		data - object that provides access to the attributes for this node
//
MStatus cgfxShaderNode::compute( const MPlug& plug, MDataBlock& data )
{
	MStatus returnStatus;

	// Compute a color, so that Hypershade swatches do not render black.
	if ((plug == outColor) || (plug.parent() == outColor))
	{
		MFloatVector color(.07f, .8f, .07f);
		MDataHandle outputHandle = data.outputValue( outColor );
		outputHandle.asFloatVector() = color;
		outputHandle.setClean();
		return MS::kSuccess;
	}

	return MS::kUnknownParameter;
}

// ========== cgfxShaderNode::creator ==========
//
//	Description:
//		this method exists to give Maya a way to create new objects
//      of this type.
//
//	Return Value:
//		a new object of this type.
//
/* static */
void* cgfxShaderNode::creator()
{
	return new cgfxShaderNode();
}


// ========== cgfxShaderNode::initialize ==========
//
//	Description:
//		This method is called to create and initialize all of the attributes
//      and attribute dependencies for this node type.  This is only called 
//		once when the node type is registered with Maya.
//
//	Return Values:
//		MS::kSuccess
//		MS::kFailure
//		
/* static */
MStatus
cgfxShaderNode::initialize()
{
	MStatus ms;

	try
	{
		initializeNodeAttrs();
	}
	catch ( cgfxShaderCommon::InternalError* e )   // internal error
	{
		size_t ee = (size_t)e;
		MString es = "cgfxShaderNode internal error ";
		es += (int)ee;
		MGlobal::displayError( es );
		ms = MS::kFailure;
	}
	catch ( ... )
	{
		MString es = "cgfxShaderNode internal error: Unhandled exception in initialize";
		MGlobal::displayError( es );
		ms = MS::kFailure;
	}

	return ms;
}


// Create all the attributes. 
/* static */
void
cgfxShaderNode::initializeNodeAttrs()
{
	MFnTypedAttribute	typedAttr;
	MFnNumericAttribute	numericAttr;
	MFnStringData		stringData;
	MFnStringArrayData	stringArrayData;
	MStatus				stat, stat2;

	// The shader attribute holds the name of the .fx file that defines
	// the shader
	//
	sShader = typedAttr.create("shader", "s", MFnData::kString, stringData.create(&stat2), &stat);
	M_CHECK( stat2 );
	M_CHECK( stat );

	// Attribute is keyable and will show up in the channel box
	//
	stat = typedAttr.setKeyable(true);
	M_CHECK( stat );

	// Mark it as internal so we can track changes to it and know when to
	// reload the .fx file
	//
	stat = typedAttr.setInternal(true);
	M_CHECK( stat );

	// Add the attribute we have created to the node
	//
	stat = addAttribute(sShader);
	M_CHECK( stat );

	//
	// technique
	//

	sTechnique = typedAttr.create("technique", "t", MFnData::kString,
		stringData.create(&stat2), &stat);
	M_CHECK( stat2 );
	M_CHECK( stat );

	typedAttr.setInternal(true);
	typedAttr.setKeyable(true);

	stat = addAttribute(sTechnique);
	M_CHECK( stat );

	//
	// attributeList (uniform parameters)
	//

	sAttributeList = typedAttr.create("attributeList", "al", MFnData::kStringArray,
		stringArrayData.create(&stat2), &stat);
	M_CHECK( stat2 );
	M_CHECK( stat );

	// Attribute is NOT keyable and will NOT show up in the channel box
	//
	stat = typedAttr.setKeyable(false);
	M_CHECK( stat );

	// Attribute is NOT connectable
	//
	stat = typedAttr.setConnectable(false);
	M_CHECK( stat );

	// This attribute is an NOT an array.  (It is a single valued attribute
	// whose value is a single MStringArray object.)
	//
	stat = typedAttr.setArray(false);
	M_CHECK( stat );

	// Mark it as internal so we can track changes to it and know when to
	// reload the .fx file
	//
	stat = typedAttr.setInternal(true);
	M_CHECK( stat );

	// This attribute is a hidden.
	//
	stat = typedAttr.setHidden(true);
	M_CHECK( stat );

	// Add the attribute we have created to the node
	//
	stat = addAttribute(sAttributeList);
	M_CHECK( stat );

	//
	// vertexAttributeList (varying parameters)
	//

	sVertexAttributeList = typedAttr.create("vertexAttributeList", "val", MFnData::kStringArray,
		stringArrayData.create(&stat2), &stat);
	M_CHECK( stat2 );
	M_CHECK( stat );

	// Attribute is NOT keyable and will NOT show up in the channel box
	//
	stat = typedAttr.setKeyable(false);
	M_CHECK( stat );

	// Attribute is NOT connectable
	//
	stat = typedAttr.setConnectable(false);
	M_CHECK( stat );

	// This attribute is an NOT an array.  (It is a single valued attribute
	// whose value is a single MStringArray object.)
	//
	stat = typedAttr.setArray(false);
	M_CHECK( stat );

	// Mark it as internal so we can track changes to it and know when to
	// reload the .fx file
	//
	stat = typedAttr.setInternal(true);
	M_CHECK( stat );

	// This attribute is a hidden.
	//
	stat = typedAttr.setHidden(true);
	M_CHECK( stat );

	// Add the attribute we have created to the node
	//
	stat = addAttribute(sVertexAttributeList);
	M_CHECK( stat );

	//
	// vertexAttributeSource
	//
	sVertexAttributeSource = typedAttr.create( "vertexAttributeSource", "vas", MFnData::kStringArray, 
		MObject::kNullObj, &stat );
	M_CHECK( stat );
	stat = typedAttr.setInternal( true );
	M_CHECK( stat );
	stat = addAttribute( sVertexAttributeSource );
	M_CHECK( stat );


	//
	// texCoordSource
	//
	sTexCoordSource = typedAttr.create( "texCoordSource", "tcs", MFnData::kStringArray, 
		MObject::kNullObj, &stat );
	M_CHECK( stat );
	stat = typedAttr.setInternal( true );
	M_CHECK( stat );
	stat = addAttribute( sTexCoordSource );
	M_CHECK( stat );


	//
	// colorSource
	//
	sColorSource = typedAttr.create( "colorSource", "cs", MFnData::kStringArray, 
		MObject::kNullObj, &stat );
	M_CHECK( stat );
	stat = typedAttr.setInternal( true );
	M_CHECK( stat );
	stat = addAttribute( sColorSource );
	M_CHECK( stat );


	//
	// texturesByName
	//
	sTexturesByName = numericAttr.create( "texturesByName", "tbn", MFnNumericData::kBoolean,
		0, &stat );
	M_CHECK( stat );
	stat = numericAttr.setInternal(true);
	M_CHECK( stat );
	// Hide this switch - TDs can recompile this to default to
	// different options, but we don't want to encourage users
	// to switch some shading nodes to use node textures, and
	// others named textures (and we definitely don't want to
	// try and handle converting configured shaders from one to
	// the other)
	//
	stat = numericAttr.setHidden(true);
	M_CHECK( stat );
	numericAttr.setKeyable(false);
	stat = addAttribute( sTexturesByName );
	M_CHECK( stat );

}                                      // cgfxShaderNode::initializeNodeAttrs


/* virtual */
void
cgfxShaderNode::copyInternalData( MPxNode* pSrc )
{
	const cgfxShaderNode& src = *(cgfxShaderNode*)pSrc;
	setTexturesByName( src.getTexturesByName() );
	setShaderFxFile( src.shaderFxFile() );
	setShaderFxFileChanged( true );
	setTechnique( src.getTechnique() );	
	setDataSources( &src.getTexCoordSource(), &src.getColorSource() );

	// Rebuild the shader from the fx file.
	//
	MString fileName = cgfxFindFile(shaderFxFile());
	bool hasFile = (fileName.asChar() != NULL) && strcmp(fileName.asChar(), "");
	if ( hasFile )
	{
		// Create the effect for this node.
		//
		MStringArray fileOptions; 
		cgfxGetFxIncludePath( fileName, fileOptions );
		const char *opts[_CGFX_PLUGIN_MAX_COMPILER_ARGS_];
		unsigned int numOpts = fileOptions.length();		
		if (numOpts)
		{
			numOpts = (numOpts > _CGFX_PLUGIN_MAX_COMPILER_ARGS_) ? _CGFX_PLUGIN_MAX_COMPILER_ARGS_ : numOpts;
			for (unsigned int i=0; i<numOpts; i++)
				opts[i] = fileOptions[i].asChar();
			opts[numOpts] = NULL;
		}
		CGeffect cgEffect = cgCreateEffectFromFile(sCgContext, fileName.asChar(), opts);

		if (cgEffect) 
		{
			cgfxAttrDefList* effectList = NULL;
			MStringArray attributeList;
			MDGModifier dagMod;

			// Update the node.
			//
			cgfxAttrDef::updateNode(cgEffect, this, &dagMod, effectList, attributeList);
			MStatus status = dagMod.doIt();
			assert(status == MS::kSuccess);

			setAttrDefList(effectList);
			setAttributeList(attributeList);
			setEffect(cgEffect);

			// The node now owns this list, and we don't need it anymore. So release
			if( effectList ) 
				effectList->release();
		}
	}
}
// cgfxShaderNode::copyInternalData

bool cgfxShaderNode::setInternalValueInContext( const MPlug& plug,
											  const MDataHandle& handle,
											  MDGContext&)
{
	bool retVal = true;

	try
	{
#ifdef KH_DEBUG
		MString ss = "  .. seti ";
		ss += plug.partialName( true, true, true, false, false, true );
		if (plug == sShader ||
			plug == sTechnique)
		{
			ss += " \"";
			ss += handle.asString();
			ss += "\"";
		}
		ss += "\n";
		::OutputDebugString( ss.asChar() );
#endif
		if (plug == sShader)
		{
			setShaderFxFile(handle.asString());
		}
		else if (plug == sTechnique)
		{
			setTechnique(handle.asString());
		}
		else if (plug == sAttributeList)
		{
			MDataHandle nonConstHandle(handle);
			MObject saData = nonConstHandle.data();
			MFnStringArrayData fnSaData(saData);
			setAttributeList(fnSaData.array());
		}
		else if (plug == sVertexAttributeList)
		{
			MDataHandle nonConstHandle(handle);
			MObject saData = nonConstHandle.data();
			MFnStringArrayData fnSaData(saData);
			const MStringArray& attributeList = fnSaData.array();

			cgfxVertexAttribute* attributes = NULL;
			cgfxVertexAttribute** nextAttribute = &attributes;
			int numAttributes = attributeList.length() / 4;
			for( int i = 0; i < numAttributes; i++)
			{
				cgfxVertexAttribute* attribute = new cgfxVertexAttribute();
				attribute->fName = attributeList[ i * 4 + 0];
				attribute->fType = attributeList[ i * 4 + 1];
				attribute->fUIName = attributeList[ i * 4 + 2];
				attribute->fSemantic = attributeList[ i * 4 + 3];
				*nextAttribute = attribute;
				nextAttribute = &attribute->fNext;
			}
			setVertexAttributes( attributes);
		}
		else if ( plug == sVertexAttributeSource )
		{
			MDataHandle nonConstHandle( handle );
			MObject     saData = nonConstHandle.data();
			MFnStringArrayData fnSaData( saData );
			MStringArray values = fnSaData.array();
			setVertexAttributeSource( values);
		}
		else if ( plug == sTexCoordSource )
		{
			MDataHandle nonConstHandle( handle );
			MObject     saData = nonConstHandle.data();
			MFnStringArrayData fnSaData( saData );
			MStringArray values = fnSaData.array();
			setDataSources( &values, NULL );
		}
		else if ( plug == sColorSource )
		{
			MDataHandle nonConstHandle( handle );
			MObject     saData = nonConstHandle.data();
			MFnStringArrayData fnSaData( saData );
			MStringArray values = fnSaData.array();
			setDataSources( NULL, &values );
		}
		else if ( plug == sTexturesByName )
		{
			setTexturesByName( handle.asBool(), !MFileIO::isReadingFile());
		}
		else
		{
			retVal = MPxHwShaderNode::setInternalValue(plug, handle);
		}
	}
	catch ( cgfxShaderCommon::InternalError* e )   
	{
		reportInternalError( __FILE__, (size_t)e );
		retVal = false;
	}
	catch ( ... )
	{
		reportInternalError( __FILE__, __LINE__ );
		retVal = false;
	}

	return retVal;
}


/* virtual */
bool cgfxShaderNode::getInternalValueInContext( const MPlug& plug,
											  MDataHandle& handle,
											  MDGContext&)
{
	bool retVal = true;

	try
	{
#ifdef KH_DEBUG
		MString ss = "  .. geti ";
		ss += plug.partialName( true, true, true, false, false, true );
		if ( plug == sShader )
			ss += " \"" + fShaderFxFile + "\"";
		else if (plug == sTechnique)
			ss += " \"" + fTechnique + "\"";
		ss += "\n";
		::OutputDebugString( ss.asChar() );
#endif
		if (plug == sShader)
		{
			handle.set(fShaderFxFile);
		}
		else if (plug == sTechnique)
		{
			handle.set(fTechnique);
		}
		else if (plug == sAttributeList)
		{
			MFnStringArrayData saData;
			handle.set(saData.create(fAttributeListArray));
		}
		else if (plug == sVertexAttributeList)
		{

			MStringArray attributeList;
			cgfxVertexAttribute* attribute = fVertexAttributes;
			while( attribute)
			{
				attributeList.append( attribute->fName);
				attributeList.append( attribute->fType);
				attributeList.append( attribute->fUIName);
				attributeList.append( attribute->fSemantic);
				attribute = attribute->fNext;
			}

			MFnStringArrayData saData;
			handle.set(saData.create( attributeList));
		}
		else if ( plug == sVertexAttributeSource )
		{
			MStringArray attributeSources;
			cgfxVertexAttribute* attribute = fVertexAttributes;
			while( attribute)
			{
				attributeSources.append( attribute->fSourceName);
				attribute = attribute->fNext;
			}

			MFnStringArrayData saData;
			handle.set( saData.create( attributeSources ) );
		}
		else if ( plug == sTexCoordSource )
		{
			MFnStringArrayData saData;
			handle.set( saData.create( fTexCoordSource ) );
		}
		else if ( plug == sColorSource )
		{
			MFnStringArrayData saData;
			handle.set( saData.create( fColorSource ) );
		}
		else if (plug == sTexturesByName)
		{
			handle.set(fTexturesByName);
		}
		else
		{
			retVal = MPxHwShaderNode::getInternalValue(plug, handle);
		}
	}
	catch ( cgfxShaderCommon::InternalError* e )   
	{
		reportInternalError( __FILE__, (size_t)e );
		retVal = false;
	}
	catch ( ... )
	{
		reportInternalError( __FILE__, __LINE__ );
		retVal = false;
	}

	return retVal;
}


static void checkGlErrors(const char* msg)
{
#if defined(CGFX_DEBUG)
#define MYERR(n)	case n: OutputDebugStrings("    ", #n); break

	GLenum err;
	bool errors = false;

	while ((err = glGetError()) != GL_NO_ERROR)
	{
		if (!errors)
		{
			// Print this the first time through the loop
			//
			OutputDebugStrings("OpenGl errors: ", msg);
		}

		errors = true;

		switch (err)
		{
			MYERR(GL_INVALID_ENUM);
			MYERR(GL_INVALID_VALUE);
			MYERR(GL_INVALID_OPERATION);
			MYERR(GL_STACK_OVERFLOW);
			MYERR(GL_STACK_UNDERFLOW);
			MYERR(GL_OUT_OF_MEMORY);
		default:
			{
				char tmp[32];
				sprintf(tmp, "%d", err);
				OutputDebugStrings("    GL Error #", tmp);
			}
		}
	}
#undef MYERR
#endif /* CGFX_DEBUG */
}


// Handle a change in a connected texture
//
void textureChangedCallback( MNodeMessage::AttributeMessage msg, MPlug & plug, MPlug & otherPlug, void* aDef)
{
	// Whenever there is a change in our texture's attributes (which 
	// could also be texture node deleted or disconnected), remove 
	// our callback to signify that this texture needs to be refreshed.
	// We don't release the GL texture here because there may not be
	// a valid GL context around when the DG is being updated. The
	// texture will get flushed at the next draw time when the bind
	// code determines there is a node but no callback.
	((cgfxAttrDef*)aDef)->releaseCallback();
}


#if defined(_SWATCH_RENDERING_SUPPORTED_)
/* virtual */
MStatus cgfxShaderNode::renderSwatchImage( MImage & outImage )
{
	MStatus status = MStatus::kFailure;

	if( sCgContext == 0 )	return status;

	// Get the hardware renderer utility class
	MHardwareRenderer *pRenderer = MHardwareRenderer::theRenderer();
	if (pRenderer)
	{
		const MString& backEndStr = pRenderer->backEndString();

		// Get geometry
		// ============
		unsigned int* pIndexing = 0;
		unsigned int  numberOfData = 0;
		unsigned int  indexCount = 0;

		MHardwareRenderer::GeometricShape gshape = MHardwareRenderer::kDefaultSphere;
		MGeometryData* pGeomData =
			pRenderer->referenceDefaultGeometry( gshape, numberOfData, pIndexing, indexCount );
		if( !pGeomData )
		{
			return MStatus::kFailure;
		}

		// Make the swatch context current
		// ===============================
		//
		unsigned int width, height;
		outImage.getSize( width, height );
		unsigned int origWidth = width;
		unsigned int origHeight = height;

		MStatus status2 = pRenderer->makeSwatchContextCurrent( backEndStr, width, height );

		if( status2 != MS::kSuccess )
		{
			pRenderer->dereferenceGeometry( pGeomData, numberOfData );
		}

		// Get the light direction from the API, and use it
		// =============================================
		{
			float light_pos[4];
			pRenderer->getSwatchLightDirection( light_pos[0], light_pos[1], light_pos[2], light_pos[3] );
		}

		// Get camera
		// ==========
		{
			// Get the camera frustum from the API
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			double l, r, b, t, n, f;
			pRenderer->getSwatchPerspectiveCameraSetting( l, r, b, t, n, f );
			glFrustum( l, r, b, t, n, f );

			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			float x, y, z, w;
			pRenderer->getSwatchPerspectiveCameraTranslation( x, y, z, w );
			glTranslatef( x, y, z );
		}

		// Get the default background color and clear the background
		//
		float r, g, b, a;
		MHWShaderSwatchGenerator::getSwatchBackgroundColor( r, g, b, a );
		glClearColor( r, g, b, a );
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glShadeModel(GL_SMOOTH);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

		// Draw The Swatch
		// ===============
		//drawTheSwatch( pGeomData, pIndexing, numberOfData, indexCount );
		MDagPath dummyPath;
		glBind( dummyPath );

		float *vertexData = (float *)( pGeomData[0].data() );
		float *normalData = (float *)( pGeomData[1].data() );
		float *uvData = (float *)( pGeomData[2].data() );
		float *tangentData = (float *)( pGeomData[3].data() );
		float *binormalData = (float *)( pGeomData[4].data() );

		// Stick uvs into ptr array
		int uvCount = fTexCoordSource.length();
		float ** texCoordArrays = uvCount ? new float * [ uvCount] : NULL;
		for( int uv = 0; uv < uvCount; uv++)
		{
			texCoordArrays[ uv] = uvData;
		}

		// Stick normal, tangent, binormals into ptr array
		int normalCount = uvCount > 0 ? uvCount : 1;
		float ** normalArrays = new float * [ fNormalsPerVertex * normalCount];
		for( int n = 0; n < normalCount; n++)
		{
			if( fNormalsPerVertex > 0)
			{
				normalArrays[ n * fNormalsPerVertex + 0] = normalData;
				if( fNormalsPerVertex > 1)
				{
					normalArrays[ n * fNormalsPerVertex + 1] = tangentData;
					if( fNormalsPerVertex > 2)
					{
						normalArrays[ n * fNormalsPerVertex + 2] = binormalData;
					}
				}
			}
		}

		glGeometry( dummyPath,
					GL_TRIANGLES,
					false,
					indexCount,
					pIndexing,
					pGeomData[0].elementCount(), 
					NULL, /* no vertex ids */
					vertexData,
					fNormalsPerVertex,
					(const float **) normalArrays,
					0, 
					NULL, /* no colours */
					uvCount,
					(const float **) texCoordArrays);


		glUnbind( dummyPath );

		if( normalArrays) delete[] normalArrays;
		if( texCoordArrays) delete[] texCoordArrays;

		// Read pixels back from swatch context to MImage
		// ==============================================
		pRenderer->readSwatchContextPixels( backEndStr, outImage );

		// Double check the outing going image size as image resizing
		// was required to properly read from the swatch context
		outImage.getSize( width, height );
		if (width != origWidth || height != origHeight)
		{
			status = MStatus::kFailure;
		}
		else
		{
			status = MStatus::kSuccess;
		}
	}
	return status;
}
#endif

// Tell Maya that Cg effects can be batched
//
bool cgfxShaderNode::supportsBatching() const
{
	return true;
}


// Tell Maya to invert texture coordinates for this shader
//
bool cgfxShaderNode::invertTexCoords() const
{
	return true;
}


// Try and create a missing effect (e.g. once a GL context is available)
//
bool cgfxShaderNode::createEffect()
{
	// Attempt to read the effect from the file. But only when it has
	// changed file name. In the case where the file cannot be found
	// we will not continuously search for the same file while refreshing.
	// The user will need to manually "refresh" the file name, or change
	// it to force a new attempt to load the file here.
	//
	bool rc = false;
	if (shaderFxFileChanged())
	{
		MString fileName = cgfxFindFile(shaderFxFile());

		if(fileName.asChar() != NULL && strcmp(fileName.asChar(), ""))
		{
			// Compile and create the effect.
			MStringArray fileOptions; 
			cgfxGetFxIncludePath( fileName, fileOptions );
			const char *opts[_CGFX_PLUGIN_MAX_COMPILER_ARGS_];
			unsigned int numOpts = fileOptions.length();		
			if (numOpts)
			{
				numOpts = (numOpts > _CGFX_PLUGIN_MAX_COMPILER_ARGS_) ? _CGFX_PLUGIN_MAX_COMPILER_ARGS_ : numOpts;
				for (unsigned int i=0; i<numOpts; i++)
					opts[i] = fileOptions[i].asChar();
				opts[numOpts] = NULL;
			}
			CGeffect cgEffect = cgCreateEffectFromFile(sCgContext, fileName.asChar(), opts);

			if (cgEffect) 
			{
				cgfxAttrDefList* effectList = NULL;
				MStringArray attributeList;
				MDGModifier dagMod;
				// updateNode does a fair amount of work.  It determines which
				// attributes need to be added and which need to be deleted and
				// fills in all the changes in the MDagModifier.  Then it builds
				// a new value for the attributeList attribute.  Finally, it
				// builds a new value for the attrDefList internal value.  All
				// these values are returned here where we can set them into the
				// node.
				cgfxAttrDef::updateNode(cgEffect, this, &dagMod, effectList, attributeList);
				MStatus status = dagMod.doIt();
				assert(status == MS::kSuccess);

				// Actually update the node.
				setAttrDefList(effectList);
				setAttributeList(attributeList);
				setEffect(cgEffect);
				setTechnique( fTechnique);
				rc = true;

				// The node now owns this, and we don't need it anymore, so release it
				if( effectList ) 
					effectList->release();
			}
		}
		setShaderFxFileChanged( false );
	}
	return rc;
}


/* virtual */
MStatus cgfxShaderNode::glBind(const MDagPath& shapePath)
{	
	// This is the routine where you would do all the expensive,
	// one-time kind of work.  Create vertex programs, load
	// textures, etc.
	//
	glStateCache::instance().reset();

	// Since we have no idea what the effect may set, we have
	// to push everything.

//	glPushAttrib(GL_ALL_ATTRIB_BITS);
//	glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);

	// In this case, we will evaluate all the attributes and store the
	// parameter values.  In theory, there could be multiple calls to
	// geometry in between single calls to bind and unbind.  Since we
	// only need to get the attribute values once per frame, get them
	// in bind.

	MStatus stat;     

#ifdef KH_DEBUG
	MString ss = "  .. bind ";
	if ( this && fConstructed )
		ss += name();
	ss += " ";
	ss += request.multiPath().fullPathName();
	ss += "\n";
	::OutputDebugString( ss.asChar() );
#endif

	try
	{
		// One-time OpenGL initialization...
		if ( glStateCache::sMaxTextureUnits <= 0 )
		{
			// Before this point, we never had a good OpenGL context.  Now
			// we can check for extensions and set up pointers to the
			// extension procs.
#ifdef _WIN32
#define RESOLVE_GL_EXTENSION( fn, ext) wglGetProcAddress( #fn #ext)
#elif defined LINUX
#define RESOLVE_GL_EXTENSION( fn, ext) &fn ## ext
#else
#define RESOLVE_GL_EXTENSION( fn, ext) &fn
#endif

			glStateCache::glClientActiveTexture = (PFNGLCLIENTACTIVETEXTUREARBPROC) RESOLVE_GL_EXTENSION( glClientActiveTexture, ARB);
			glStateCache::glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERARBPROC) RESOLVE_GL_EXTENSION( glVertexAttribPointer, ARB);
			glStateCache::glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYARBPROC) RESOLVE_GL_EXTENSION( glEnableVertexAttribArray, ARB);
			glStateCache::glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYARBPROC) RESOLVE_GL_EXTENSION( glDisableVertexAttribArray, ARB);
			glStateCache::glVertexAttrib4f = (PFNGLVERTEXATTRIB4FARBPROC) RESOLVE_GL_EXTENSION( glVertexAttrib4f, ARB);
			glStateCache::glSecondaryColorPointer = (PFNGLSECONDARYCOLORPOINTEREXTPROC) RESOLVE_GL_EXTENSION( glSecondaryColorPointer, EXT);
			glStateCache::glSecondaryColor3f = (PFNGLSECONDARYCOLOR3FEXTPROC) RESOLVE_GL_EXTENSION( glSecondaryColor3f, EXT);
			glStateCache::glMultiTexCoord4fARB = (PFNGLMULTITEXCOORD4FARBPROC) RESOLVE_GL_EXTENSION( glMultiTexCoord4f, ARB);

			// Don't use GL_MAX_TEXTURE_UNITS as this does not provide a proper
			// count when the # of image or texcoord inputs differs
			// from the conventional (older) notion of texture unit. 
			//
			// Instead take the minimum of GL_MAX_TEXTURE_COORDS_ARB and
			// GL_MAX_TEXUTRE_IMAGE_UNITS_ARB according to the 
			// ARB_FRAGMENT_PROGRAM specification.
			//
			GLint tval;
			glGetIntegerv( GL_MAX_TEXTURE_COORDS_ARB, &tval );
			GLint mic = 0;
			glGetIntegerv( GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &mic );
			if (mic < tval)
				tval = mic;

			// Don't use this...
			//glGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB, &tval );
			glStateCache::sMaxTextureUnits = tval;
			if (!glStateCache::glClientActiveTexture || glStateCache::sMaxTextureUnits < 1)
				glStateCache::sMaxTextureUnits = 1;
			else if (glStateCache::sMaxTextureUnits > CGFXSHADERNODE_GL_TEXTURE_MAX)
				glStateCache::sMaxTextureUnits = CGFXSHADERNODE_GL_TEXTURE_MAX;
		}

		// Try and grab the first pass of our effect
		if( fNewEffect.fTechnique && fNewEffect.fTechnique->fPasses)
		{
			// Set up the uniform attribute values for the effect.
			bindAttrValues();

			// Set depth function properly in case we have multi-pass
			if (fTechniqueHasBlending)
				glPushAttrib( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
			glGetBooleanv( GL_DEPTH_TEST, &fDepthEnableState);
			glGetIntegerv( GL_DEPTH_FUNC, &fDepthFunc);
			glGetIntegerv( GL_BLEND_SRC, &fBlendSourceFactor);
			glGetIntegerv( GL_BLEND_DST, &fBlendDestFactor);
			glDepthFunc(GL_LEQUAL);
			if( !fNewEffect.fTechnique->fMultiPass && fNewEffect.fTechnique->fPasses)
				cgSetPassState( fNewEffect.fTechnique->fPasses->fPass);
			}
		else
		{
			// There is no effect.  Either they never set one or the one provided
			// failed to compile.  Just use this default material which is sort
			// of a shiny salmon-pink color.  It looks like nothing that Maya
			// creates by default but still lets you see your geometry.
			//
			glPushAttrib( GL_LIGHTING);
			static float diffuse_color[4]  = {1.0, 0.5, 0.5, 1.0};
			static float specular_color[4] = {1.0, 1.0, 1.0, 1.0};

			glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
			glEnable(GL_COLOR_MATERIAL);
			glColor4fv(diffuse_color);

			// Set up the specular color
			//
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular_color);

			// Set up a default shininess
			//
			glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 100.0);
		}

		checkGlErrors("cgfxShaderNode::glBind");
	}
	catch ( cgfxShaderCommon::InternalError* e )   
	{
		reportInternalError( __FILE__, (size_t)e );
		stat = MS::kFailure;
	}
	catch ( ... )
	{
		reportInternalError( __FILE__, __LINE__ );
		stat = MS::kFailure;
	}

	return stat;
}                                      // cgfxShaderNode::bind


static bool textureInitPowerOfTwo(unsigned int val, unsigned int & retval)
{
	unsigned int res = 0;				// May be we should return 1 when val == 0
	if (val)
	{
		// Muliply all values by 2, to simplify the testing:
		// 3*(res/2) < val*2 <= 3*res
		val <<= 1;
		unsigned int low = 3;
		res = 1;
		while (val > low)
		{
			low <<= 1;
			res <<= 1;
		}
	}

	retval = res;
    return (res == (val>>1)) ? 1 : 0;
}

void cgfxShaderNode::bindAttrValues()
{
	if ( !fEffect || !fTechnique.length() )
		return;

	MStatus  status;
	MObject  oNode = thisMObject();

	// This method should NEVER access the shape. If you find yourself tempted to access
	// any data from the shape here (like the matrices), be strong and resist! Any shape 
	// dependent data should be set in bindAttrViewValues instead!
	//

	for ( cgfxAttrDefList::iterator it( fAttrDefList ); it; ++it )
	{                                  // loop over fAttrDefList
		cgfxAttrDef* aDef = *it;

		try
		{

			switch (aDef->fType)
			{
			case cgfxAttrDef::kAttrTypeBool:
				{
					bool tmp;
					aDef->getValue(oNode, tmp);
					cgSetParameter1i(aDef->fParameterHandle, tmp);
					break;
				}

			case cgfxAttrDef::kAttrTypeInt:
				{
					int tmp;
					aDef->getValue(oNode, tmp);
					cgSetParameter1i(aDef->fParameterHandle, tmp);
					break;
				}

			case cgfxAttrDef::kAttrTypeFloat:
				{
					float tmp;
					aDef->getValue(oNode, tmp);
					cgSetParameter1f(aDef->fParameterHandle, tmp);
					break;
				}

			case cgfxAttrDef::kAttrTypeString:
				{
					MString tmp;
					aDef->getValue(oNode, tmp);
					cgSetStringParameterValue(aDef->fParameterHandle, tmp.asChar());
					break;
				}

			case cgfxAttrDef::kAttrTypeVector2:
				{
					float tmp[2];
					aDef->getValue(oNode, tmp[0], tmp[1]);
					cgSetParameter2fv(aDef->fParameterHandle, tmp);
					break;
				}

			case cgfxAttrDef::kAttrTypeVector3:
			case cgfxAttrDef::kAttrTypeColor3:
				{
					float tmp[3];
					aDef->getValue(oNode, tmp[0], tmp[1], tmp[2]);
					cgSetParameter3fv(aDef->fParameterHandle, tmp);
					break;
				}

			case cgfxAttrDef::kAttrTypeVector4:
			case cgfxAttrDef::kAttrTypeColor4:
				{
					float tmp[4];
					aDef->getValue(oNode, tmp[0], tmp[1], tmp[2], tmp[3]);
					cgSetParameter4fv(aDef->fParameterHandle, tmp);
					break;
				}

			case cgfxAttrDef::kAttrTypeWorldDir:
			case cgfxAttrDef::kAttrTypeWorldPos:
				{
					// Read the value
					float tmp[4];
					if (aDef->fSize == 3)
					{
						aDef->getValue(oNode, tmp[0], tmp[1], tmp[2]);
						tmp[3] = 1.0;
					}
					else
					{
						aDef->getValue(oNode, tmp[0], tmp[1], tmp[2], tmp[3]);
					}

					// Find the coordinate space, and whether it is a
					// point or a vector
					int base = cgfxAttrDef::kAttrTypeFirstPos;
					if (aDef->fType <= cgfxAttrDef::kAttrTypeLastDir) 
						base = cgfxAttrDef::kAttrTypeFirstDir;
					int space = aDef->fType - base;

					// Compute the transform matrix
					MMatrix mat;
					switch (space)
					{
						/* case 0:	object space, handled in view dependent method */
						case 1:	/* world space  - do nothing, identity */ break;
						/* case 2: eye space, unsupported yet */
						/* case 3: clip space, unsupported yet */
						/* case 4: screen space, unsupported yet */
					}

					if (base == cgfxAttrDef::kAttrTypeFirstPos)
					{
						MPoint point(tmp[0], tmp[1], tmp[2], tmp[3]);
						point *= mat;
						tmp[0] = (float)point.x;
						tmp[1] = (float)point.y;
						tmp[2] = (float)point.z;
						tmp[3] = (float)point.w;
					}
					else
					{
						MVector vec(tmp[0], tmp[1], tmp[2]);
						vec *= mat;
						tmp[0] = (float)vec.x;
						tmp[1] = (float)vec.y;
						tmp[2] = (float)vec.z;
						tmp[3] = 1.F;
					}

					cgSetParameterValuefr(aDef->fParameterHandle, aDef->fSize, tmp);
					break;
				}

			case cgfxAttrDef::kAttrTypeMatrix:
				{
					MMatrix tmp;
					float tmp2[4][4];
					aDef->getValue(oNode, tmp);

					if (aDef->fInvertMatrix)
					{
						tmp = tmp.inverse();
					}

					if (!aDef->fTransposeMatrix)
					{
						tmp = tmp.transpose();
					}

					tmp.get(tmp2);
					cgSetMatrixParameterfr(aDef->fParameterHandle, &tmp2[0][0]);
					break;
				}

			case cgfxAttrDef::kAttrTypeColor1DTexture:
			case cgfxAttrDef::kAttrTypeColor2DTexture:
			case cgfxAttrDef::kAttrTypeColor3DTexture:
			case cgfxAttrDef::kAttrTypeColor2DRectTexture:
			case cgfxAttrDef::kAttrTypeNormalTexture:
			case cgfxAttrDef::kAttrTypeBumpTexture:
			case cgfxAttrDef::kAttrTypeCubeTexture:
			case cgfxAttrDef::kAttrTypeEnvTexture:
			case cgfxAttrDef::kAttrTypeNormalizationTexture:

				{
					MString tmp;
					MObject textureNode = MObject::kNullObj;

					if( fTexturesByName)
					{
						aDef->getValue(oNode, tmp);
					}
					else
					{
						// If we have a fileTexture node connect, get the 
						// filename it is using
						MPlug srcPlug;
						aDef->getSource(oNode, srcPlug);
						MObject srcNode = srcPlug.node();
						if( srcNode != MObject::kNullObj)
						{
							MStatus rc;
							MFnDependencyNode dgFn( srcNode);
							MPlug filenamePlug = dgFn.findPlug( "fileTextureName", &rc);
							if( rc == MStatus::kSuccess)
							{
								filenamePlug.getValue( tmp);
								textureNode = filenamePlug.node(&rc);
							}

							// attach a monitor to this texture if we don't already have one
							// Note that we don't need to worry about handling node destroyed
							// or disconnected, as both of these will trigger attribute changed
							// messages before going away, and we will deregister our callback
							// in the handler!
							if( aDef->fTextureMonitor == kNullCallback && textureNode != MObject::kNullObj)
							{
								// If we don't have a callback, this may mean our texture is dirty
								// and needs to be re-loaded (because we can't actually delete the
								// texture itself in the DG callback we need to wait until we
								// know we have a GL context - like right here)
								aDef->releaseTexture();
								aDef->fTextureMonitor = MNodeMessage::addAttributeChangedCallback( textureNode, textureChangedCallback, aDef);
							}
						}
					}

					if (aDef->fTextureId == 0 || tmp != aDef->fStringDef)
					{
						// If the texture object already exists, reuse
						// it.  Otherwise, we create a new one.
						//
						if (aDef->fTextureId == 0)
						{
							GLuint val;
							glGenTextures(1, &val);
							aDef->fTextureId = val;
						}

						aDef->fStringDef = tmp;

						nv_dds::CDDSImage image;

						MString path = cgfxFindFile(tmp);

						// If that failed, try and resolve the texture path relative to the
						// effect
						//
						if (path.asChar() == NULL || !strcmp(path.asChar(), ""))
						{
							MFileObject effectFile;
							effectFile.setRawFullName( fShaderFxFile);
							path = cgfxFindFile( effectFile.path() + tmp);
						}


						if (path.asChar() != NULL && strcmp(path.asChar(), ""))
						{
							switch (aDef->fType)
							{
							case cgfxAttrDef::kAttrTypeEnvTexture:
							case cgfxAttrDef::kAttrTypeCubeTexture:
							case cgfxAttrDef::kAttrTypeNormalizationTexture:
								// we don't want to flip cube maps...
								image.load(path.asChar(),false);
								break;
							default:
								// Only flip 2D textures if we're using right-handed texture
								// coordinates. Most of the time, we want to do the flipping
								// on the UV coordinates rather than the texture so that procedural
								// texture coordinates generated inside the shader work as well
								// (and if we just flip the texture to compensate for Maya's UV
								// coordinate system, these will get inverted)
								image.load(path.asChar(), !invertTexCoords() );
								break;
							}
						}

						// Our common stand-in "texture"
						// The code below creates a separate stand-in GL texture 
						// for every attribute without a value (rather than sharing
						// the default across all node/attributes of a given type. 
						// This is done because the current design does not support
						// GL texture id sharing across nodes/attributes AND because
						// we want to avoid checking disk every frame for missing
						// textures. Once this plugin is re-factored to support a
						// shared texture cache, we should revisit this to share
						// default textures too
						//
						static unsigned char whitePixel[ 4] = { 255, 255, 255, 255};

						switch (aDef->fType)
						{
						case cgfxAttrDef::kAttrTypeColor1DTexture:
							glBindTexture(GL_TEXTURE_1D,aDef->fTextureId);
							if( image.is_valid())
							{
								// Load the image
								glTexParameteri(GL_TEXTURE_1D, GL_GENERATE_MIPMAP_SGIS, image.get_num_mipmaps() == 0);
								image.upload_texture1D();
							}
							else
							{
								// Create a dummy stand-in texture
								glTexImage1D( GL_TEXTURE_1D, 0, GL_RGBA, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
							}
							break;
						case cgfxAttrDef::kAttrTypeColor2DTexture:
						case cgfxAttrDef::kAttrTypeNormalTexture:
						case cgfxAttrDef::kAttrTypeBumpTexture:
#if !defined(WIN32) && !defined(LINUX)
						case cgfxAttrDef::kAttrTypeColor2DRectTexture:
#endif
							glBindTexture(GL_TEXTURE_2D,aDef->fTextureId);
							if( image.is_valid())
							{
								// Load the image
								glTexParameteri( GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, image.get_num_mipmaps() == 0);
								image.upload_texture2D();
							}
							else
							{
								// Try to use Maya's default file texture loading,
								// if the DDS loader failed. For now all that
								// we can support is 2D textures.
								//
								if (textureNode != MObject::kNullObj) 
								{
									MImage img;
									unsigned int width, height;

									if (MS::kSuccess == img.readFromTextureNode(textureNode))
									{
										// If we're using left handed texture coordinates, flip it upside down
										// (to undo the automatic flip it receives being read in by Maya)
										if( invertTexCoords() )
										{
											img.verticalFlip();
										}

										MStatus status = img.getSize( width, height);
										if (width > 0 && height > 0 && (status != MStatus::kFailure) )
										{
											// If not power of two and NPOT is not supported, then we need 
											// to resize the original system pixmap before binding.
											//
											if (width > 2 && height > 2)
											{
												unsigned int p2Width, p2Height;
												bool widthPowerOfTwo  = textureInitPowerOfTwo(width,  p2Width);
												bool heightPowerOfTwo = textureInitPowerOfTwo(height, p2Height);
												if(!widthPowerOfTwo || !heightPowerOfTwo)
												{
													width = p2Width;
													height = p2Height;
													img.resize( p2Width, p2Height, false /* preserverAspectRatio */);
												}
											}										
											glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, true);
											glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.pixels());
										}
										else
										{
											// Create a dummy stand-in texture
											glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, 
												GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
										}
									}
									else
									{
										// Create a dummy stand-in texture
										glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, 
											GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
									}
								}
								else
								{
									// Create a dummy stand-in texture
									glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, 
										GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
								}
							}
							break;
						case cgfxAttrDef::kAttrTypeEnvTexture:
						case cgfxAttrDef::kAttrTypeCubeTexture:
						case cgfxAttrDef::kAttrTypeNormalizationTexture:
							{
								glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, aDef->fTextureId);
								if( image.is_valid())
								{
									GLenum target;
									glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_GENERATE_MIPMAP_SGIS, image.get_num_mipmaps() == 0);
									// loop through cubemap faces and load them as 2D textures 
									for (int n = 0; n < 6; ++n)
									{
										// specify cubemap face
										target = GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB+n;
										image.upload_texture2D(image.is_cubemap() ? n : 0, target);
									}
								}
								// else Add default environment map here if needed
								break;
							}
						case cgfxAttrDef::kAttrTypeColor3DTexture:
							glBindTexture(GL_TEXTURE_3D,aDef->fTextureId);
							if( image.is_valid())
							{
							image.upload_texture3D();
							}
							break;
#if defined(WIN32) || defined(LINUX)
							// No such thing as NV texture rectangle
							// on Mac.
						case cgfxAttrDef::kAttrTypeColor2DRectTexture:
							glBindTexture(GL_TEXTURE_RECTANGLE_NV, aDef->fTextureId);
							if( image.is_valid())
							{
								// Load the image
								image.upload_textureRectangle();
							}
							else
							{
								// Create a dummy stand-in texture
								glTexImage2D( GL_TEXTURE_RECTANGLE_NV, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
							}
							break;
#endif
						default:
							assert(false);
						}
					}
					checkGlErrors("After loading texture");
					cgGLSetupSampler(aDef->fParameterHandle, aDef->fTextureId);
					break;
				}
#ifdef _WIN32
			case cgfxAttrDef::kAttrTypeTime:
				{
					int ival = timeGetTime() & 0xffffff;
					float val = (float)ival * 0.001f; 
					cgSetParameter1f(aDef->fParameterHandle, val);
					break;
				}
#endif
			case cgfxAttrDef::kAttrTypeOther:
			case cgfxAttrDef::kAttrTypeUnknown:
				break;

			case cgfxAttrDef::kAttrTypeObjectDir:
			case cgfxAttrDef::kAttrTypeViewDir:
			case cgfxAttrDef::kAttrTypeProjectionDir:
			case cgfxAttrDef::kAttrTypeScreenDir:

			case cgfxAttrDef::kAttrTypeObjectPos:
			case cgfxAttrDef::kAttrTypeViewPos:
			case cgfxAttrDef::kAttrTypeProjectionPos:
			case cgfxAttrDef::kAttrTypeScreenPos:

			case cgfxAttrDef::kAttrTypeWorldMatrix:
			case cgfxAttrDef::kAttrTypeViewMatrix:
			case cgfxAttrDef::kAttrTypeProjectionMatrix:
			case cgfxAttrDef::kAttrTypeWorldViewMatrix:
			case cgfxAttrDef::kAttrTypeWorldViewProjectionMatrix:
				// View dependent parameter
				break;
			default:
				M_CHECK( false );
			}                          // switch (aDef->fType)
		}
		catch ( cgfxShaderCommon::InternalError* e )   
		{
			if ( ++fErrorCount <= fErrorLimit ) 
			{
				size_t ee = (size_t)e;
				MFnDependencyNode fnNode( oNode );
				MString sMsg = "cgfxShader warning ";
				sMsg += (int)ee;
				sMsg += ": ";
				sMsg += fnNode.name();
				sMsg += " internal error while setting parameter \"";
				sMsg += aDef->fName;
				sMsg += "\" of effect \""; 
				sMsg += fShaderFxFile;
				sMsg += "\" for shape ";
				sMsg += currentPath().partialPathName();
				MGlobal::displayWarning( sMsg );
			}
		}
	}                                  // loop over fAttrDefList
}                                      // cgfxShaderNode::bindAttrValues

void
cgfxShaderNode::bindViewAttrValues(const MDagPath& shapePath)
{
	if ( !fEffect ||
		!fTechnique.length() )
		return;

	MStatus  status;
	MObject  oNode = thisMObject();

	MMatrix wMatrix, vMatrix, pMatrix, sMatrix;
	MMatrix wvMatrix, wvpMatrix, wvpsMatrix;
	{
		float tmp[4][4];

		if (shapePath.isValid())
			wMatrix = shapePath.inclusiveMatrix();
		else
			wMatrix.setToIdentity();

		glGetFloatv(GL_MODELVIEW_MATRIX, &tmp[0][0]);
		wvMatrix = MMatrix(tmp);

		vMatrix = wMatrix.inverse() * wvMatrix;

		glGetFloatv(GL_PROJECTION_MATRIX, &tmp[0][0]);
		pMatrix = MMatrix(tmp);

		wvpMatrix = wvMatrix * pMatrix;

		float vpt[4];
		float depth[2];

		glGetFloatv(GL_VIEWPORT, vpt);
		glGetFloatv(GL_DEPTH_RANGE, depth);

		// Construct the NDC -> screen space matrix
		//
		float x0, y0, z0, w, h, d;

		x0 = vpt[0];
		y0 = vpt[1];
		z0 = depth[0];
		w  = vpt[2];
		h  = vpt[3];
		d  = depth[1] - z0;

		// Make a reference to ease the typing
		//
		double* s = &sMatrix.matrix[0][0];

		s[ 0] = w/2;	s[ 1] = 0.0;	s[ 2] = 0.0;	s[ 3] = 0.0;
		s[ 4] = 0.0;	s[ 5] = h/2;	s[ 6] = 0.0;	s[ 7] = 0.0;
		s[ 8] = 0.0;	s[ 9] = 0.0;	s[10] = d/2;	s[11] = 0.0;
		s[12] = x0+w/2;	s[13] = y0+h/2;	s[14] = z0+d/2;	s[15] = 1.0;

		wvpsMatrix = wvpMatrix * sMatrix;
	}		

	for ( cgfxAttrDefList::iterator it( fAttrDefList ); it; ++it )
	{                                  // loop over fAttrDefList
		cgfxAttrDef* aDef = *it;

		try
		{

			switch (aDef->fType)
			{
			case cgfxAttrDef::kAttrTypeObjectDir:
			case cgfxAttrDef::kAttrTypeViewDir:
			case cgfxAttrDef::kAttrTypeProjectionDir:
			case cgfxAttrDef::kAttrTypeScreenDir:

			case cgfxAttrDef::kAttrTypeObjectPos:
			case cgfxAttrDef::kAttrTypeViewPos:
			case cgfxAttrDef::kAttrTypeProjectionPos:
			case cgfxAttrDef::kAttrTypeScreenPos:
				{
					float tmp[4];
					if (aDef->fSize == 3)
					{
						aDef->getValue(oNode, tmp[0], tmp[1], tmp[2]);
						tmp[3] = 1.0;
					}
					else
					{
						aDef->getValue(oNode, tmp[0], tmp[1], tmp[2], tmp[3]);
					}

					// Maya's API only provides for vectors of size 3.
					// When we do the matrix multiply, it will only
					// work correctly if the 4th coordinate is 1.0
					//

					MVector vec(tmp[0], tmp[1], tmp[2]);

					int space = aDef->fType - cgfxAttrDef::kAttrTypeFirstPos;
					if (space < 0)
					{
						space = aDef->fType - cgfxAttrDef::kAttrTypeFirstDir;
					}

					MMatrix mat;  // initially the identity matrix.

					switch (space)
					{
					case 0:	/* mat = identity */	break;
					case 1:	mat = wMatrix;			break;
					case 2:	mat = wvMatrix;			break;
					case 3:	mat = wvpMatrix;		break;
					case 4:	mat = wvpsMatrix;		break;
					}

					// Maya's transformation matrices are set up with 
					// the translation in row 3 (like OpenGL) rather 
					// than column 3. To transform a point or vector, 
					// use V*M, not M*V.   kh 11/2003
					int base = cgfxAttrDef::kAttrTypeFirstPos;
					if (aDef->fType <= cgfxAttrDef::kAttrTypeLastDir) 
						base = cgfxAttrDef::kAttrTypeFirstDir;
					if (base == cgfxAttrDef::kAttrTypeFirstPos)
					{
						MPoint point(tmp[0], tmp[1], tmp[2], tmp[3]);
						point *= wMatrix.inverse() * mat;
						tmp[0] = (float)point.x;
						tmp[1] = (float)point.y;
						tmp[2] = (float)point.z;
						tmp[3] = (float)point.w;
					}
					else
					{
						MVector vec(tmp[0], tmp[1], tmp[2]);
						vec *= wMatrix.inverse() * mat;
						tmp[0] = (float)vec.x;
						tmp[1] = (float)vec.y;
						tmp[2] = (float)vec.z;
						tmp[3] = 1.F;
					}

					cgSetParameterValuefc(aDef->fParameterHandle, aDef->fSize, tmp);
					break;
				}

			case cgfxAttrDef::kAttrTypeWorldMatrix:
			case cgfxAttrDef::kAttrTypeViewMatrix:
			case cgfxAttrDef::kAttrTypeProjectionMatrix:
			case cgfxAttrDef::kAttrTypeWorldViewMatrix:
			case cgfxAttrDef::kAttrTypeWorldViewProjectionMatrix:
				{
					MMatrix mat;

					switch (aDef->fType)
					{
					case cgfxAttrDef::kAttrTypeWorldMatrix:
						mat = wMatrix; break;
					case cgfxAttrDef::kAttrTypeViewMatrix:
						mat = vMatrix; break;
					case cgfxAttrDef::kAttrTypeProjectionMatrix:
						mat = pMatrix; break;
					case cgfxAttrDef::kAttrTypeWorldViewMatrix:
						mat = wvMatrix; break;
					case cgfxAttrDef::kAttrTypeWorldViewProjectionMatrix:
						mat = wvpMatrix; break;
					default:
						break;
					}

					if (aDef->fInvertMatrix)
					{
						mat = mat.inverse();
					}

					if (!aDef->fTransposeMatrix)
					{
						mat = mat.transpose();
					}

					float tmp[4][4];
					mat.get(tmp);
					cgSetMatrixParameterfr(aDef->fParameterHandle, &tmp[0][0]);
					break;
				}
				default:
					break;
			}                          // switch (aDef->fType)
		}
		catch ( cgfxShaderCommon::InternalError* e )   
		{
			if ( ++fErrorCount <= fErrorLimit ) 
			{
				size_t ee = (size_t)e;
				MFnDependencyNode fnNode( oNode );
				MString sMsg = "cgfxShader warning ";
				sMsg += (int)ee;
				sMsg += ": ";
				sMsg += fnNode.name();
				sMsg += " internal error while setting parameter \"";
				sMsg += aDef->fName;
				sMsg += "\" of effect \""; 
				sMsg += fShaderFxFile;
				sMsg += "\" for shape ";
				if (shapePath.isValid())
					sMsg += shapePath.partialPathName();
				else
					sMsg += "SWATCH GEOMETRY";
				MGlobal::displayWarning( sMsg );
			}
		}
	}                                  // loop over fAttrDefList
}

/* virtual */
MStatus cgfxShaderNode::glUnbind(const MDagPath& shapePath)
{
	if( fNewEffect.fTechnique && fNewEffect.fTechnique->fPasses)
	{
		// If we've batched a single pass shader, reset the pass state
		if( !fNewEffect.fTechnique->fMultiPass)
			cgResetPassState( fNewEffect.fTechnique->fPasses->fPass);

		// Shaders have an uncanny ability to corrupt depth state
		if( fDepthEnableState)
			glEnable( GL_DEPTH_TEST);
		else
			glDisable( GL_DEPTH_TEST);
		glDepthFunc( fDepthFunc);
		glBlendFunc( fBlendSourceFactor, fBlendDestFactor);

		if (fTechniqueHasBlending)
			glPopAttrib();
	}
	else
	{
		// Restore material attributes
		glPopAttrib();
	}

	glStateCache::instance().disableAll();
	glStateCache::instance().activeTexture( 0);

//	glPopClientAttrib();
//	glPopAttrib();

#ifdef KH_DEBUG
	MString ss = "  .. unbd ";
	if ( this && fConstructed )
		ss += name();
	ss += "\n";
	::OutputDebugString( ss.asChar() );
#endif
	return MS::kSuccess;
}


/* virtual */
MStatus cgfxShaderNode::glGeometry(const MDagPath& shapePath,
									int prim,
									unsigned int writable,
									int indexCount,
									const unsigned int * indexArray,
									int vertexCount,
									const int * vertexIDs,
									const float * vertexArray,
									int normalCount,
									const float ** normalArrays,
									int colorCount,
									const float ** colorArrays,
									int texCoordCount,
									const float ** texCoordArrays)
{
	MStatus stat;

#ifdef KH_DEBUG
	MString ss = "  .. geom ";
	if ( this && fConstructed )
		ss += name();
	ss += " ";
	ss += indexCount;
	ss += "i ";
	ss += vertexCount;
	ss += "v ";
	ss += normalCount;
	ss += "n ";
	ss += colorCount;
	ss += "c ";
	ss += texCoordCount;
	ss += "t ";
	ss += "\n";
	::OutputDebugString( ss.asChar() );
#endif

	try
	{
		if( fNewEffect.fTechnique && fNewEffect.fTechnique->fPasses)
		{
			// Set up the uniform attribute values for the effect.
			bindViewAttrValues(shapePath);

			// If our input shape is dirty, clear any cached data
			if( dirtyMask() != kDirtyNone)
				fNewEffect.flushCache( shapePath);

			// Now render the passes for this effect
			cgfxPass* pass = fNewEffect.fTechnique->fPasses;
			while( pass)
			{
				pass->bind( shapePath, vertexCount, vertexArray, fNormalsPerVertex, normalCount, normalArrays, colorCount, colorArrays, texCoordCount, texCoordArrays);
				glStateCache::instance().flushState();
				if( fNewEffect.fTechnique->fMultiPass) cgSetPassState( pass->fPass);
				glDrawElements(prim, indexCount, GL_UNSIGNED_INT, indexArray);
				if( fNewEffect.fTechnique->fMultiPass) cgResetPassState( pass->fPass);
				pass = pass->fNext;
			}
		}
		else // fEffect must be NULL
		{
			// Now call glDrawElements to put all the primitives on the
			// screen.  See the comment above re: glDrawRangeElements.
			//
			glStateCache::instance().enablePosition();
			glVertexPointer(3, GL_FLOAT, 0, vertexArray);

			if ( normalCount > 0 && normalArrays[ 0 ] )
			{
				glStateCache::instance().enableNormal();
				glNormalPointer(GL_FLOAT, 0, normalArrays[0]);
			}
			else
			{
				glStateCache::instance().disableNormal();
				glNormal3f(0.0, 0.0, 1.0);
			}
			glStateCache::instance().flushState();
			glDrawElements(prim, indexCount, GL_UNSIGNED_INT, indexArray);
		}

		checkGlErrors("After effects End");
	}
	catch ( cgfxShaderCommon::InternalError* e )   
	{
		reportInternalError( __FILE__, (size_t)e );
		stat = MS::kFailure;
	}
	catch ( ... )
	{
		reportInternalError( __FILE__, __LINE__ );
		stat = MS::kFailure;
	}

	return stat;
}                                      // cgfxShaderNode::geometry


/* virtual */
int
cgfxShaderNode::getTexCoordSetNames( MStringArray& names )
{
	names = fUVSets;
	return names.length();
}                                      // cgfxShaderNode::getTexCoordSetNames


#if MAYA_API_VERSION >= 700

/* virtual */
int
cgfxShaderNode::getColorSetNames( MStringArray& names )
{
	names = fColorSets;
	return names.length();
}

#else

/* virtual */
int cgfxShaderNode::colorsPerVertex()
{
	fColorType.setLength(1);
	fColorIndex.setLength(1);
	fColorType[0] = 0;
	fColorIndex[0] = 0;
	return 1;
} // cgfxShaderNode::texCoordsPerVertex

#endif

/* virtual */
int cgfxShaderNode::normalsPerVertex()
{
#ifdef KH_DEBUG
	MString ss = "  .. npv  ";
	if ( this && fConstructed )
		ss += name();
	ss += " ";
	ss += fNormalsPerVertex;
	ss += "\n";
	::OutputDebugString( ss.asChar() );
#endif

	// Now, when using MPxHwShaderNode, this is the first call Maya makes when
	// trying to render a plugin shader. So, in the cases where we were unable
	// to create our effect, try and do it here
	if (fEffect == NULL) {
#ifdef _WIN32
		::OutputDebugString( "CGFX: fEffect was NULL\n");
#endif

		// When batch off-screen rendering through "mayabatch -command hwRender ...",
		// the effect will be uninitialized because there was no active OpenGL
		// context at the time "cgfxShader -e -fx ..." was executed. This setup
		// is delayed until now when hardware renderer guarantees a valid context
		// and requests the plug-in to bind its resources to it. -cdt
		//
		createEffect();
	}

	return fNormalsPerVertex;

	// NB: Maya calls normalsPerVertex() both before and after bind().  
	// It appears that the normalCount passed to geometry() is 
	// obtained *before* the call to bind().  Therefore we set 
	// fNormalsPerVertex as early as possible.  kh 9/03
}                                      // cgfxShaderNode::normalsPerVertex


void
cgfxShaderNode::setAttrDefList( cgfxAttrDefList* list )
{
	if (fAttrDefList)
		cgfxAttrDef::purgeMObjectCache( fAttrDefList );

	if ( list )
	{
		list->addRef();
		cgfxAttrDef::validateMObjectCache( thisMObject(), list );
	}

	if (fAttrDefList)
		fAttrDefList->release();

	fAttrDefList = list;
}                                      // cgfxShaderNode::setAttrDefList


void cgfxShaderNode::getAttributeList(MStringArray& attrList) const
{
	MString tmp;
	int len = fAttributeListArray.length();

	attrList.clear();

	for (int i = 0; i < len; ++i)
	{
		tmp = fAttributeListArray[i];
		attrList.append(tmp);
	}
}

void cgfxShaderNode::setAttributeList(const MStringArray& attrList)
{
	MString tmp;
	int len = attrList.length();

	fAttributeListArray.clear();

	for (int i = 0; i < len; ++i)
	{
		tmp = attrList[i];
		fAttributeListArray.append(tmp);
	}
}


//
// Set the current per-vertex attributes the shader needs (replacing any existing set)
//
void 
cgfxShaderNode::setVertexAttributes( cgfxVertexAttribute* attributeList)
{
	// Backward compatibility: if we have values set in the old texCoordSources
	// or colorSources, find any varying attributes that use that register
	// and inherit the maya source
	if( fTexCoordSource.length())
	{
		int length = fTexCoordSource.length();
		for( int i = 0; i < length; i++)
		{
			MString semantic( "TEXCOORD");
			if( i)
				semantic += i;
			else
				semantic += "0";
			MString source( fTexCoordSource[ i]);
			if( source.index( ':') < 0)
				source = "uv:" + source;
			cgfxVertexAttribute* newAttribute = attributeList;
			while( newAttribute)
			{
				if( newAttribute->fSemantic == semantic || 
					(i == 6 && (newAttribute->fSemantic == "TANGENT" || newAttribute->fSemantic == "TANGENT0")) ||
					(i == 7 && (newAttribute->fSemantic == "BINORMAL" || newAttribute->fSemantic == "BINORMAL0")))
					newAttribute->fSourceName = source;
				newAttribute = newAttribute->fNext;
			}
		}
		fTexCoordSource.clear();
	}
	if( fColorSource.length())
	{
		int length = fColorSource.length();
		for( int i = 0; i < length; i++)
		{
			MString semantic( "COLOR");
			if( i)
				semantic += i;
			else
				semantic += "0";
			MString source( fColorSource[ i]);
			if( source.index( ':') < 0)
				source = "color:" + source;
			cgfxVertexAttribute* newAttribute = attributeList;
			while( newAttribute)
			{
				if( newAttribute->fSemantic == semantic)
					newAttribute->fSourceName = source;
				newAttribute = newAttribute->fNext;
			}
		}
		fColorSource.clear();
	}

	// If we have an existing attribute list, copy across the current values
	// the remove it
	cgfxVertexAttribute* oldAttributes = fVertexAttributes;
	while( oldAttributes)
	{
		cgfxVertexAttribute* oldAttribute = oldAttributes;
		oldAttributes = oldAttributes->fNext;
		cgfxVertexAttribute* newAttribute = attributeList;
		while( newAttribute)
		{
			if( newAttribute->fName == oldAttribute->fName)
			{
				newAttribute->fSourceName = oldAttribute->fSourceName;
				newAttribute->fSourceType = oldAttribute->fSourceType;
			}
			newAttribute = newAttribute->fNext;
		}
		delete oldAttribute;
	}

	// Now set our new attribute list
	fVertexAttributes = attributeList;

	// And determine the minimum set of data we need to request from Maya to
	// populate these values
	analyseVertexAttributes();
}


//
// Set the data set names that will be populating our vertex attributes
//
void
cgfxShaderNode::setVertexAttributeSource( const MStringArray& sources)
{
	// Flush any cached data our effect has - the inputs have changed
	fNewEffect.flushCache();

	// Set the attributes sources as specified
	int i = 0;
	int numSources = sources.length();
	cgfxVertexAttribute* attribute = fVertexAttributes;
	while( attribute)
	{
		attribute->fSourceName = ( i <	numSources) ? sources[ i++] : "";
		attribute = attribute->fNext;
	}

	// And determine the minimum set of data we need to request from Maya to
	// populate these values
	analyseVertexAttributes();
}

inline int findOrInsert( const MString& value, MStringArray& list)
{
	int length = list.length();
	for( int i = 0; i < length; i++)
		if( list[ i] == value)
			return i;
	list.append( value);
	return length;
}

//
// Analyse the per-vertex attributes to work out the minimum set of data we require
//
void 
cgfxShaderNode::analyseVertexAttributes()
{
	fUVSets.clear();
	fColorSets.clear();
	fNormalsPerVertex = 0;

	cgfxVertexAttribute* attribute = fVertexAttributes;
	while( attribute)
	{
		// Work out where this attribute should come from
		MString source( attribute->fSourceName);
		source.toLowerCase();
		if( attribute->fSourceName.length() == 0)
		{
			attribute->fSourceType = cgfxVertexAttribute::kNone;
		}
		else if( source == "position")
		{
			attribute->fSourceType = cgfxVertexAttribute::kPosition;
		}
		else if( source == "normal")
		{
			attribute->fSourceType = cgfxVertexAttribute::kNormal;
			if( fNormalsPerVertex < 1)
				fNormalsPerVertex = 1;
		}
		else
		{
			// Try and pull off the type
			MString set = attribute->fSourceName;
			int colon = set.index( ':');
			MString type;
			if( colon >= 0)
			{
				if( colon > 0) type = source.substring( 0, colon - 1);
				set = set.substring( colon + 1, set.length() - 1);				
			}

			// Now, work out what kind of set we have here
			if( type == "uv")
			{
				attribute->fSourceType = cgfxVertexAttribute::kUV;
				attribute->fSourceIndex = findOrInsert( set, fUVSets);
			}
			else if( type == "tangent")
			{
				attribute->fSourceType = cgfxVertexAttribute::kTangent;
				if( fNormalsPerVertex < 2)
					fNormalsPerVertex = 2;
				attribute->fSourceIndex = findOrInsert( set, fUVSets);
			}
			else if( type == "binormal")
			{
				attribute->fSourceType = cgfxVertexAttribute::kBinormal;
				if( fNormalsPerVertex < 3)
					fNormalsPerVertex = 3;
				attribute->fSourceIndex = findOrInsert( set, fUVSets);
			}
			else if( type == "color")
			{
				attribute->fSourceType = cgfxVertexAttribute::kColor;
				attribute->fSourceIndex = findOrInsert( set, fColorSets);
			}
			else
			{
				attribute->fSourceType = cgfxVertexAttribute::kUnknown;
			}
		}
		attribute = attribute->fNext;
	}

	//for( unsigned int i = 0; i < fUVSets.length(); i++) printf( "Requesting UVset[%d] = %s\n", i, fUVSets[i]);
}


// Data accessors for the texCoordSource attribute.
const MStringArray&
cgfxShaderNode::getTexCoordSource() const
{
#ifdef KH_DEBUG
	MString ss = "  .. gtcs ";
	if ( this && fConstructed )
		ss += name();
	ss += " ";
	for ( int ii = 0; ii < fTexCoordSource.length(); ++ii )
	{
		ss += "\"";
		ss += fTexCoordSource[ii];
		ss += "\" ";
	}
	ss += "\n";
	::OutputDebugString( ss.asChar() );
#endif
	return fTexCoordSource;
}                                      // cgfxShaderNode::getTexCoordSource


// Data accessors for the colorSource attribute.
const MStringArray&
cgfxShaderNode::getColorSource() const
{
#ifdef KH_DEBUG
	MString ss = "  .. gtcs ";
	if ( this && fConstructed )
		ss += name();
	ss += " ";
	for ( int ii = 0; ii < fColorSource.length(); ++ii )
	{
		ss += "\"";
		ss += fColorSource[ii];
		ss += "\" ";
	}
	ss += "\n";
	::OutputDebugString( ss.asChar() );
#endif
	return fColorSource;
}                                      // cgfxShaderNode::getColorSource


void 
cgfxShaderNode::setDataSources( const MStringArray* texCoordSources, 
							    const MStringArray* colorSources)
{
	if( texCoordSources )
	{
		int length_TC = texCoordSources->length();
		if ( length_TC > CGFXSHADERNODE_GL_TEXTURE_MAX )
			length_TC = CGFXSHADERNODE_GL_TEXTURE_MAX;
		fTexCoordSource.clear();
		for ( int i = 0; i < length_TC; ++i )
		{
			fTexCoordSource.append( (*texCoordSources)[ i ] );
		}
		// This method is unstable and may causes crashes in the API
		// Don't use for now.
		//fTexCoordSource.setLength( length_TC );
		//for ( int i = 0; i < length_TC; ++i )
		//	fTexCoordSource[ i ] = texCoordSources[ i ];
	}

	if( colorSources )
	{
		int length_CS = colorSources->length();
		if ( length_CS > CGFXSHADERNODE_GL_COLOR_MAX )
			length_CS = CGFXSHADERNODE_GL_COLOR_MAX;
		fColorSource.setLength( length_CS );
		for ( int i = 0; i < length_CS; ++i )
			fColorSource[ i ] = (*colorSources)[ i ];
	}

	fDataSetNames.clear();
	fNormalsPerVertex = 1;
	updateDataSource( fTexCoordSource, fTexCoordType, fTexCoordIndex);
	updateDataSource( fColorSource, fColorType, fColorIndex);
}


void
cgfxShaderNode::updateDataSource( MStringArray& v, MIntArray& typeList, MIntArray& indexList)
{
#ifdef KH_DEBUG
	MString ss = "  .. stcs ";
	if ( this && fConstructed )
		ss += name();
	ss += " ";
	for ( int ii = 0; ii < v.length(); ++ii )
	{
		ss += "\"";
		ss += v[ii];
		ss += "\" ";
	}
	ss += "\n";
	::OutputDebugString( ss.asChar() );
#endif

	int nDataSets = v.length();
	typeList.setLength( nDataSets );
	indexList.setLength( nDataSets );
	for ( int iDataSet = 0; iDataSet < nDataSets; ++iDataSet )
	{                                  // iDataSet loop
		MString s;
		int iType = etcNull;
		int iBuf = 0;

		// Strip leading and trailing spaces and control chars.
		const char* bp = v[ iDataSet ].asChar();
		const char* ep = v[ iDataSet ].length() + bp;
#ifdef _WIN32
		while ( bp < ep && *bp <= ' ' && *bp >= '\0') ++bp;
#else
		while ( bp < ep && *bp <= ' ') ++bp;
#endif

#ifdef _WIN32
		while ( bp < ep && ep[-1] <= ' ' && ep[-1] >= '\0' ) --ep;
#else
		while ( bp < ep && ep[-1] <= ' ' ) --ep;
#endif

		// Empty?
		if ( bp == ep )
			iType = etcNull;

		// Constant?  (1, 2, 3 or 4 float values)
		else if ( (*bp >= '0' && *bp <= '9') ||
			*bp == '-' ||
			*bp == '+' ||
			*bp == '.' )
		{
			const char* cp = bp;
			int nValues = 0;
			while ( cp < ep &&
				nValues < 4 )
			{
				float x;
				int   nc = 0;
				int   nv = sscanf( cp, " %f%n", &x, &nc );
				if ( nv != 1 )
					break;
				++nValues;
				cp += nc;
			}
			if ( nValues > 0 )
			{
				s.set( bp, (int)(cp - bp) );      // drop trailing junk
				for ( ; nValues < 4; ++nValues )
					s += " 0";  
				iType = etcConstant;
			}
		}

		// UV set name or reserved word.
		else
		{
			s.set( bp, (int)(ep - bp) );

			// Pull out any qualifiers (e.g. tangent:uvSet1) and register
			// the data set they require
			//
			MStringArray splitStrings;	
			#define kDefaultUVSet "map1"
			if ((MStatus::kSuccess == s.split( ':', splitStrings)) && splitStrings.length() > 1)
			{
				s = splitStrings[0];
				iBuf = findOrAppend( fDataSetNames, splitStrings[1]);
			}

			// Force reserved words to lower case.
			bp = s.asChar();
			if ( 0 == stricmp( "normal", bp ) )
			{
				s = "normal";
				iType = etcNormal;
			}
			else if ( 0 == stricmp( "tangent", bp ) )
			{
				s = "tangent";
				if( splitStrings.length() < 2)
				{
					splitStrings.setLength( 2);
					splitStrings[ 1] = kDefaultUVSet;
					iBuf = findOrAppend( fDataSetNames, kDefaultUVSet);
				}
				s += ":" + splitStrings[1];
				iType = etcTangent;
				if( fNormalsPerVertex < 2)
					fNormalsPerVertex = 2;
			}
			else if ( 0 == stricmp( "binormal", bp ) )
			{
				s = "binormal";
				if( splitStrings.length() < 2)
				{
					splitStrings.setLength( 2);
					splitStrings[ 1] = kDefaultUVSet;
					iBuf = findOrAppend( fDataSetNames, kDefaultUVSet);
				}
				s += ":" + splitStrings[1];
				iType = etcBinormal;
				fNormalsPerVertex = 3;
			}

			// Data set name... tell Maya that we want to retrieve this data set.
			else
			{
				iType = etcDataSet;
				iBuf = findOrAppend( fDataSetNames, s );
			}
		}

		// Tell our geometry() method where to get data.
		typeList[ iDataSet ] = iType;
		indexList[ iDataSet ] = iBuf;

		// Store cleaned-up string.
		v[ iDataSet ] = s;
	}                                  // iDataSet loop
}                                      // cgfxShaderNode::updateDataSource


// Data accessor for list of empty UV sets.
const MStringArray&
cgfxShaderNode::getEmptyUVSets() const
{
	static const MStringArray saNull;
	return saNull;
}                                      // cgfxShaderNode::getEmptyUVSets

const MObjectArray&
cgfxShaderNode::getEmptyUVSetShapes() const
{
	static const MObjectArray oaNull;
	return oaNull;
}                                      // cgfxShaderNode::getEmptyUVSetShapes


void
cgfxShaderNode::setEffect(CGeffect pNewEffect)
{
#ifdef KH_DEBUG
	char ssbuf[64];
	MString ss = "  .. se   ";
	ss += name();
	ss += " ";
	sprintf( ssbuf, "new=%X old=%X", pNewEffect, fEffect );
	ss += ssbuf;
	ss += "\n";
	::OutputDebugString( ss.asChar() );
#endif

	if (fEffect)
		cgDestroyEffect(fEffect);

	fEffect = pNewEffect;

	fNewEffect.setEffect( fEffect);
	// fNewEffect.setupAttributes( this); this will happen when the technique gets set!

	// Build string array containing technique names and descriptions.
	//     Each item in the technique list has the form
	//         "techniqueName<TAB>numPasses"
	//     where 
	//         numPasses is the number of passes defined by the 
	//             technique, or 0 if the technique is not valid.   
	//     (Future versions of the cgfxShader plug-in may append
	//      additional tab-separated fields.)
	fTechniqueList.clear();
	if (fEffect)
	{
		CGtechnique cgTechnique = cgGetFirstTechnique(fEffect);
		while (cgTechnique) 
		{
			MString s;

			const char*		techniqueName = cgGetTechniqueName(cgTechnique);
			if (techniqueName)
			{
				s += techniqueName;
			}

			if (cgValidateTechnique(cgTechnique) == CG_TRUE)
			{
				int		numPasses = 0;
				CGpass cgPass = cgGetFirstPass(cgTechnique);
				while (cgPass) 
				{
					++numPasses;
					cgPass = cgGetNextPass(cgPass);
				}
				s += "\t";
				s += numPasses;
			}
			else
			{
				// TO DO: should get the errors from the failed technique
				//
				s += "\t0";
			}

			fTechniqueList.append(s);
			cgTechnique = cgGetNextTechnique(cgTechnique);

		}
	}
}                                      // cgfxShaderNode::setEffect


/* virtual */ bool cgfxShaderNode::hasTransparency()
{
	// Always return false, so that transparencyOptions() will be
	// called to give finer grain control.
	return false;
}


/* virtual */
unsigned int cgfxShaderNode::transparencyOptions()
{
	if (fTechniqueHasBlending)
	{
		// Set as transparent, but we don't want any internal transparency algorithms
		// being used.
		return ( kIsTransparent | kNoTransparencyFrontBackCull | kNoTransparencyPolygonSort );
	}
	return 0;
}

//
// Scan the technique for passes which use blending
//
void cgfxShaderNode::setTechniqueHasBlending( CGtechnique technique)
{
	// Assume not blending
	fTechniqueHasBlending = false;

	// Check for : BlendEnable=true, BlendFunc=something valid on the first pass only.
	//
	// We ignore any depth enable and functions for now...
	//
	CGpass cgPass = cgGetFirstPass(technique);
	bool foundBlendEnabled = false;
	bool foundBlendFunc = false;
	if (cgPass) 
	{
		CGstateassignment stateAssignment = cgGetFirstStateAssignment(cgPass);
		while ( stateAssignment )
		{
			CGstate state = cgGetStateAssignmentState( stateAssignment);
			const char *stateName = cgGetStateName(state);

			// Check for blend enabled.
			if (!foundBlendEnabled && stricmp( stateName, "BlendEnable") == 0)
			{
				int numValues = 0;
				const CGbool *values = cgGetBoolStateAssignmentValues(stateAssignment, &numValues);
				if (values && numValues)
				{
					if (values[0])
					{
						foundBlendEnabled = true;
					}
				}
			}

			// Check for valid blend function 
			else if (!foundBlendFunc && stricmp( stateName, "BlendFunc") == 0)
			{
				int numValues = 0;
				const int * values = cgGetIntStateAssignmentValues(stateAssignment, &numValues);
				if (values && numValues==2)
				{
#if defined(CGFX_DEBUG_BLEND_FUNCTIONS)
					/* 
					#define GL_SRC_COLOR                      0x0300 = 768
					#define GL_ONE_MINUS_SRC_COLOR            0x0301 = 769
					#define GL_SRC_ALPHA                      0x0302 = 770
					#define GL_ONE_MINUS_SRC_ALPHA            0x0303 = 771
					#define GL_DST_ALPHA                      0x0304 = 772
					#define GL_ONE_MINUS_DST_ALPHA            0x0305 = 773
					*/
					MString blendStringTable[6] = 
					{
						"GL_SRC_COLOR", // SrcColor
						"GL_ONE_MINUS_SRC_COLOR", // OneMinusSrcColor
						"GL_SRC_ALPHA", // SrcAlpha
						"GL_ONE_MINUS_SRC_ALPHA", // OneMinusSrcAlpha
						"GL_DST_ALPHA", // DstAlpha
						"GL_ONE_MINUS_DST_ALPHA" // OneMinusDstAlpha
					};
#endif
					if ((values[0] >= GL_SRC_COLOR) && (values[0] <= GL_ONE_MINUS_DST_ALPHA) &&
					    (values[1] >= GL_SRC_COLOR) && (values[1] <= GL_ONE_MINUS_DST_ALPHA) )
					{
#if defined(CGFX_DEBUG_BLEND_FUNCTIONS)
						printf("Found blend function = %s, %s\n",
							blendStringTable[ values[0]-GL_SRC_COLOR].asChar(),
							blendStringTable[ values[1]-GL_SRC_COLOR].asChar());
#endif
						foundBlendFunc = true;
					}
				}
			}
			fTechniqueHasBlending = foundBlendEnabled && foundBlendFunc;
			if (fTechniqueHasBlending)
				break;
			stateAssignment = cgGetNextStateAssignment( stateAssignment);
		}
	}
}

void
cgfxShaderNode::setTechnique( const MString& techn )
{
	// If effect not loaded, just store the technique name.
	if (!fEffect)
	{
		fTechnique = techn;
		fTechniqueHasBlending = false;
		return;
	}

	// Search for requested technique.
	CGtechnique technique = cgGetNamedTechnique(fEffect, techn.asChar());
	if (cgValidateTechnique(technique) == CG_TRUE)
	{
		fTechnique = techn;  
		setTechniqueHasBlending( technique );

		// Setup the vertex parameters for this technique
		fNewEffect.setupAttributes( this);
		return;
	}
	
	// Requested technique was not found or not valid.  Revert to the old one.
	technique = cgGetNamedTechnique(fEffect, fTechnique.asChar());
	if (cgValidateTechnique(technique) == CG_TRUE)
	{ 
		setTechniqueHasBlending( technique );
		return;
	}

	// Old technique is no good.  Activate the first valid technique.
	technique = cgGetFirstTechnique(fEffect);
	while (technique) 
	{
		if (cgValidateTechnique(technique) == CG_TRUE)
		{
			fTechnique = cgGetTechniqueName(technique);

			setTechniqueHasBlending( technique );

			// Setup the vertex parameters for this technique
			fNewEffect.setupAttributes( this);
			return;
		}
		technique = cgGetNextTechnique(technique);
  }

	// No valid technique exists for the current effect.
	//   Save requested technique name.  We'll try to use it as the
	//   initial technique the next time a valid effect is loaded.
	fTechnique = techn;
	fTechniqueHasBlending = false;
}                                      // cgfxShaderNode::setTechnique


MStatus cgfxShaderNode::shouldSave ( const MPlug & plug, bool & ret )
{
	if (plug == sAttributeList)
	{
		ret = true;
		return MS::kSuccess;
	}
	else if (plug == sVertexAttributeList)
	{
		ret = true;
		return MS::kSuccess;
	}
	return MPxNode::shouldSave(plug, ret);
}


void cgfxShaderNode::setTexturesByName(bool texturesByName, bool updateAttributes)
{
	if( updateAttributes && fTexturesByName != texturesByName)
	{
		// We've been explicitly changed to a different
		// texture mode. 

		// If we have any current texture attributes, destroy them
		//
		MDGModifier dgMod;
		cgfxAttrDefList* nodeList = attrDefList(); 
		cgfxAttrDefList::iterator nmIt;
		bool foundTextures = false;
		for (nmIt = nodeList->begin(); nmIt; ++nmIt)
		{
			cgfxAttrDef* adef = (*nmIt);
			if(adef->fType >= cgfxAttrDef::kAttrTypeFirstTexture && adef->fType <= cgfxAttrDef::kAttrTypeLastTexture)
			{
				MObject theMObject = thisMObject();
				adef->destroyAttribute( theMObject, &dgMod);
				foundTextures = true;
			}
		}

		// Switch across to the new texture mode (before creating the
		// new attributes)
		//
		fTexturesByName = texturesByName;

		// Now re-create our texture attributes 
		//
		if( foundTextures)
		{
			dgMod.doIt();
			for (nmIt = nodeList->begin(); nmIt; ++nmIt)
			{
				cgfxAttrDef* adef = (*nmIt);
				if( adef->fType >= cgfxAttrDef::kAttrTypeFirstTexture && adef->fType <= cgfxAttrDef::kAttrTypeLastTexture)
				{
					adef->createAttribute(thisMObject(), &dgMod, this);
				}
			}
			dgMod.doIt();

			// Finally, if we just created new string attributes, we need to
			// set them to a sensible value or they won't show up
			//
			if( fTexturesByName)
			{
				for (nmIt = nodeList->begin(); nmIt; ++nmIt)
				{
					cgfxAttrDef* adef = (*nmIt);
					if( adef->fType >= cgfxAttrDef::kAttrTypeFirstTexture &&
						adef->fType <= cgfxAttrDef::kAttrTypeLastTexture)
					{
		                MObject theMObject = thisMObject();
						adef->setTexture( theMObject, adef->fStringDef, &dgMod);
					}
				}
			}
		}
	}
	else
	{
		fTexturesByName = texturesByName;
	}
}


// Get cgfxShader version string.
MString
cgfxShaderNode::getPluginVersion()
{
	MString sVer = "cgfxShader ";
	sVer += CGFXSHADER_VERSION;       
	sVer += " for Maya ";
	sVer += (int)(MAYA_API_VERSION / 100);
	sVer += ".";
	sVer += (int)(MAYA_API_VERSION % 100 / 10);
	sVer += " (";
	sVer += __DATE__;
	sVer += ")";
	return sVer;
}                                      // cgfxShaderNode::getPluginVersion


// Error reporting
void
cgfxShaderNode::reportInternalError( const char* function, size_t errcode )
{
	MString es = "cgfxShader";

	try
	{
		if ( this &&
			fConstructed )
		{
			if ( ++fErrorCount > fErrorLimit ) 
				return;
			MString s;
			s += "\"";
			s += name();
			s += "\": ";
			s += typeName();
			es = s;
		}
	}
	catch ( ... )
	{}
	es += " internal error ";
	es += (int)errcode;
	es += " in ";
	es += function;
#ifdef KH_DEBUG
	::OutputDebugString( es.asChar() );
	::OutputDebugString( "\n" );
#endif
	MGlobal::displayError( es );
}                                      // cgfxShaderNode::reportInternalError

void
cgfxShaderNode::cgErrorCallBack()
{
	MGlobal::displayInfo(__FUNCTION__);
	CGerror cgLastError = cgGetError();
	if(cgLastError)
	{
		MGlobal::displayError(cgGetErrorString(cgLastError));
		MGlobal::displayError(cgGetLastListing(sCgContext));
	}
}																			 // cgfxShaderNode::cgErrorCallBack

void
cgfxShaderNode::cgErrorHandler(CGcontext cgContext, CGerror cgError, void* userData)
{
	MGlobal::displayError(cgGetErrorString(cgError));
	MGlobal::displayError(cgGetLastListing(sCgContext));
}

