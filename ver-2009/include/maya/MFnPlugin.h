#ifndef _MFnPlugin
#define _MFnPlugin
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
// CLASS:    MFnPlugin
//
// ****************************************************************************

#if defined __cplusplus

// ****************************************************************************
// INCLUDED HEADER FILES


#include <maya/MFnBase.h>
#include <maya/MApiVersion.h>
#include <maya/MPxNode.h>
#include <maya/MPxData.h>

#if !defined(MNoPluginEntry)
#ifdef NT_PLUGIN
#include <maya/MTypes.h>
HINSTANCE MhInstPlugin;

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD /*dwReason*/, LPVOID /*lpReserved*/)
{
	MhInstPlugin = hInstance;
	return 1;
}
#endif // NT_PLUGIN
#endif // MNoPluginEntry

// ****************************************************************************
// DECLARATIONS

#define	PLUGIN_COMPANY "Autodesk"
struct _object;
//! \brief Opaque type used by the Python API to pass Python objects.
typedef struct _object PyObject;

class MString;
class MFileObject;
class MTypeId;
class MRenderPassDef;

class MPxMaterialInformation;
class MPxBakeEngine;

//! \brief Pointer to a function which returns a new instance of an MPxMaterialInformation node.
/*!
 \param[in,out] object Material node (e.g. shader)
*/
typedef MPxMaterialInformation* (*MMaterialInfoFactoryFnPtr) (MObject& object);

//! \brief Pointer to a function which returns a new instance of an MPxBakeEngine.
typedef MPxBakeEngine*	        (*MBakeEngineCreatorFnPtr)();

// ****************************************************************************
// CLASS DECLARATION (MFnPlugin)

//! \ingroup OpenMaya MFn
//! \brief Register and deregister plug-in services with Maya. 
/*!
This class is used in the initializePlugin and uninitializePlugin functions
in a Maya plugin in order to register new services with Maya.  The
constructor for this class is passed the MObject provided by Maya as an
argument to initializePlugin and uninitializePlugin.  Subsequent calls are
made to the various "register" methods from inside initializePlugin, and to
the various "deregister" methods from inside uninitializePlugin.

A plug-in's uninitializePlugin function is not called automatically when Maya
exits.  Meaning any code which deallocates resources or a similar task will
be called when the plug-in is explicitly unloaded but not when Maya exits.
In order to work-around this problem and execute the code when Maya exits use
the MSceneMessage class's addCallback method with a message of "kMayaExiting".
This will register a callback function that will be executed when the
"kMayaExiting" message occurs.  Use this message and the addCallback method
to deallocate any resources when exiting.

A side effect of including MFnPlugin.h is to embed an API version string into
the .o file produced.  If it is necessary to include MFnPlugin.h into more
than one file that comprises a plugin, the preprocessor macro
<i>MNoVersionString</i> should be defined in all but one of those files
prior to the inclusion of MFnPlugin.h.  If this is not done, the linker
will produce warnings about multiple definitions of the variable
<i>MApiVersion</i> as the .so file is being produced.  These warning are
harmless and can be ignored.  Normally, this will not arise as only the
file that contains the <i>initializePlugin</i> and <i>uninitializePlugin</i>
routines should need to include MFnPlugin.h.

It is possible to instantiate several MFnPlugin objects with
in a single plug-in binary. The vendor and version information that is
set for this plug-in is taken from the first instantiation as this
information works per binary rather than per command/node etc.

Modules are related to the topic of plug-ins and are described next.

<b>Modules </b>

Modules provide a way of informing Maya where a third party plug-in has
been installed.  This is the recommended way of allowing Maya to
know that a plug-in  is available and is important for 3rd party
developers who deliver plug-ins outside of the Maya installation
procedure.

The list of directories that Maya searches for modules can be accessed
through the MAYA_MODULE_PATH environment variable. Please consult the
online documentation for more details on this environment variable. To
examine this variable value, run the following from the MEL script
editor:

\code
	getenv "MAYA_MODULE_PATH";
\endcode

The MAYA_MODULE_PATH contains a list of directories and within each module
directory there are subdirectories that look like:

\code
	PRODUCT\VERSION
\endcode

Within that directory are zero or more text files. Each file has a
single line that looks like this:

\code
	+ NAME VERSION PATH
\endcode

The fields are defined as:

	\li <b>+</b> add this plug-in
	\li <b>NAME</b> plug-in name ( no spaces allowed )
	\li <b>VERSION</b> plug-in version ( no spaces allowed )
	\li <b>PATH</b> plug-in path ( spaces allowed )


If we had a plug-in named doubleHelix, the single line for a
Windows installation might be:

\code
	+ doubleHelix 6.0 c:\program files\DoubleHelix\
\endcode

On Linux, we might have:

\code
	+ doubleHelix 6.0 /usr/local/DoubleHelix/
\endcode

When Maya starts up it will search the paths defined by MAYA_MODULE_PATH.
For any paths found it will do the following:

	\li <b>PATH/scripts</b> is appended to MAYA_SCRIPT_PATH
	\li <b>PATH/plug-ins</b> is appended to MAYA_PLUG_IN_PATH
	\li <b>PATH/icons</b> is appended to XBMLANGPATH
	\li <b>PATH/resources/LANG</b> is appended to MAYA_PLUG_IN_RESOURCE_PATH


When the Plug-in Manager is opened it will display the plug-ins
described by the module files since the appropriate paths have been
modified.  Following the subdirectory naming structure using "plug-ins",
"scripts" and "icons" will allow Maya to find your files properly.
The "resources" directory is used when maya is running in a
localized language. Subdirectories of the form "resources/<LANG>"
can be used to provide localized plug-in resources for
the corresponding language.  The subdir name <b>LANG</b> will correspond
to the value returned by `about -uiLanguage`.
On Macintosh platforms, language subdirs of the form "<b>LANG</b>.lproj" are also supported.


NOTE: a specific module path may not exist on disk.  This will not
cause an error.  Maya will skip over this path and continue to
process the others.
*/
class OPENMAYA_EXPORT MFnPlugin : public MFnBase
{
public:
					MFnPlugin();
					MFnPlugin( MObject& object,
							   const char* vendor = "Unknown",
							   const char* version = "Unknown",
							   const char* requiredApiVersion = "Any",
							   MStatus* ReturnStatus = 0L );
	virtual			~MFnPlugin();
	virtual			MFn::Type type() const;

	MString			vendor( MStatus* ReturnStatus=NULL ) const;
	MString			version( MStatus* ReturnStatus=NULL ) const;
	MString			apiVersion( MStatus* ReturnStatus=NULL ) const;
	MString			name( MStatus* ReturnStatus=NULL ) const;
	MString			loadPath( MStatus* ReturnStatus=NULL ) const;
	MStatus			setName( const MString& newName,
							 bool allowRename = true );

	MStatus			setVersion( const MString& newVersion );

	MStatus			registerCommand(const MString& commandName,
									MCreatorFunction creatorFunction,
									MCreateSyntaxFunction
									    createSyntaxFunction = NULL);
	MStatus			deregisterCommand(	const MString& commandName );
	MStatus 		registerControlCommand(const MString& commandName,
										   MCreatorFunction creatorFunction
										   );
	MStatus			deregisterControlCommand(const MString& commandName);
	MStatus 		registerModelEditorCommand(const MString& commandName,
								   		MCreatorFunction creatorFunction,
								   		MCreatorFunction paneCreatorFunction);
	MStatus			deregisterModelEditorCommand(const MString& commandName);
	MStatus 		registerConstraintCommand(const MString& commandName,
								   		MCreatorFunction creatorFunction );
	MStatus			deregisterConstraintCommand(const MString& commandName);
    MStatus         registerContextCommand( const MString& commandName,
											MCreatorFunction creatorFunction );

    MStatus         registerContextCommand( const MString& commandName,
											MCreatorFunction creatorFunction,
											const MString& toolCmdName,
											MCreatorFunction toolCmdCreator,
											MCreateSyntaxFunction
												toolCmdSyntax = NULL
											);

    MStatus         deregisterContextCommand( const MString& commandName );
    MStatus         deregisterContextCommand( const MString& commandName,
											  const MString& toolCmdName );
	MStatus			registerNode(	const MString& typeName,
									const MTypeId& typeId,
									MCreatorFunction creatorFunction,
									MInitializeFunction initFunction,
									MPxNode::Type type = MPxNode::kDependNode,
									const MString* classification = NULL);
	MStatus			deregisterNode(	const MTypeId& typeId );
	MStatus			registerShape(	const MString& typeName,
									const MTypeId& typeId,
									MCreatorFunction creatorFunction,
									MInitializeFunction initFunction,
									MCreatorFunction uiCreatorFunction,
									const MString* classification = NULL);
	MStatus			registerTransform(	const MString& typeName,
										const MTypeId& typeId,
										MCreatorFunction creatorFunction,
										MInitializeFunction initFunction,
										MCreateXformMatrixFunction xformCreatorFunction,
										const MTypeId& xformId,
										const MString* classification = NULL);
	MStatus			registerData(	const MString& typeName,
									const MTypeId& typeId,
									MCreatorFunction creatorFunction,
									MPxData::Type type = MPxData::kData );
	MStatus			deregisterData(	const MTypeId& typeId );
	MStatus         registerDevice( const MString& deviceName,
									MCreatorFunction creatorFunction );
	MStatus         deregisterDevice( const MString& deviceName );
	MStatus			registerFileTranslator( const MString& translatorName,
										char* pixmapName,
										MCreatorFunction creatorFunction,
										char* optionsScriptName = NULL,
										char* defaultOptionsString = NULL,
										bool requiresFullMel = false );
	MStatus			deregisterFileTranslator( const MString& translatorName );
	MStatus			registerIkSolver( const MString& ikSolverName,
										MCreatorFunction creatorFunction );
	MStatus			deregisterIkSolver( const MString& ikSolverName );

	MStatus			registerCacheFormat( const MString& cacheFormatName,
										MCreatorFunction creatorFunction );
	MStatus			deregisterCacheFormat( const MString& cacheFormatName );

	MStatus			registerUIStrings(MInitializeFunction registerMStringResources,
									  const MString &pluginStringsProc);

	MStatus			registerUI(PyObject * creationProc,
							   PyObject * deletionProc,
							   PyObject * creationBatchProc = NULL,
							   PyObject * deletionBatchProc = NULL);
	MStatus			registerDragAndDropBehavior( const MString& behaviorName,
												 MCreatorFunction creatorFunction);

	MStatus         deregisterDragAndDropBehavior( const MString& behaviorName );

	MStatus			registerImageFile( const MString& imageFormatName,
									   MCreatorFunction creatorFunction,
									const MStringArray& imageFileExtensions);
	MStatus			deregisterImageFile( const MString& imageFormatName);

	MStatus			registerRenderPassImpl( const MString& passImplId,
											MRenderPassDef* passDef,
											MCreatorFunction creatorFunction,
											bool overload=false);
	MStatus			deregisterRenderPassImpl( const MString& passImplId);
	

	static MObject  findPlugin( const MString& pluginName );

	static bool		isNodeRegistered(	const MString& typeName);

	MTypeId			matrixTypeIdFromXformId(const MTypeId& xformTypeId, MStatus* ReturnStatus=NULL);

	MStringArray	addMenuItem(
							const MString& menuItemName,
							const MString& parentName,
							const MString& commandName,
							const MString& commandParams,
							bool needOptionBox = false,
							const MString *optBoxFunction = NULL,
							MStatus *retStatus = NULL
							);
	MStatus			removeMenuItem(MStringArray& menuItemNames);
	MStatus			registerMaterialInfo(const MString& type, MMaterialInfoFactoryFnPtr fnPtr );
	MStatus			unregisterMaterialInfo(const MString &typeName);
	MStatus			registerBakeEngine(const MString &typeName, MBakeEngineCreatorFnPtr fnPtr );
	MStatus			unregisterBakeEngine(const MString &typeName);

	static void			setRegisteringCallableScript();
	static bool			registeringCallableScript();

	//	Deprecated Methods

	//!	Obsolete
	MStatus			registerTransform(	const MString& typeName,
										const MTypeId& typeId,
										MCreatorFunction creatorFunction,
										MInitializeFunction initFunction,
										MCreatorFunction xformCreatorFunction,
										const MTypeId& xformId,
										const MString* classification = NULL);

BEGIN_NO_SCRIPT_SUPPORT:
	//!     NO SCRIPT SUPPORT
	MStatus			registerUI(const MString & creationProc,
							   const MString & deletionProc,
							   const MString & creationBatchProc = "",
							   const MString & deletionBatchProc = "");
END_NO_SCRIPT_SUPPORT:

protected:
	virtual const char* className() const;

private:
					MFnPlugin( const MObject& object,
							   const char* vendor = "Unknown",
							   const char* version = "Unknown",
							   const char* requiredApiVersion = "Any",
							   MStatus* ReturnStatus = 0L );
	MFnPlugin&		operator=( const MFnPlugin & );
	MFnPlugin*		operator& () const;
	MFnPlugin*		operator& ();
};

#endif /* __cplusplus */
#endif /* _MFnPlugin */
