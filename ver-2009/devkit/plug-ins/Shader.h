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

//
//
// Shader.h
//
//  This file defines the abstract interface for different shader types to be used with .fx
// files in the Maya renderer. The goal is to have a default shader, a GLSL fx shader, and an
// HLSL fx shader 
//
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SHADER_H
#define SHADER_H

#include <string>

class shader {
  public:

    enum DataType {
      dtFloat,
      dtVec2,
      dtVec3,
      dtVec4,
      dtInt,
      dtIVec2,
      dtIVec3,
      dtIVec4,
      dtBool,
      dtBVec2,
      dtBVec3,
      dtBVec4,
      dtMat2,
      dtMat3,
      dtMat4,
      dtString,
      dtUnknown
    };

    enum Semantic {
      /* no semantic, standard parameter */
      smNone,
      /* standard matrices */
      smWorld,
      smView,
      smProjection,
      smWorldView,
      smViewProjection,
      smWorldViewProjection,
      /* inverse matrices */
      smWorldI,
      smViewI,
      smProjectionI,
      smWorldViewI,
      smViewProjectionI,
      smWorldViewProjectionI,
      /* transpose matrices */
      smWorldT,
      smViewT,
      smProjectionT,
      smWorldViewT,
      smViewProjectionT,
      smWorldViewProjectionT,
      /* inverse tranpose matrices */
      smWorldIT,
      smViewIT,
      smProjectionIT,
      smWorldViewIT,
      smViewProjectionIT,
      smWorldViewProjectionIT,
      /* view position (in world space) */
      smViewPosition,
      /* time related semantics */
      smTime,
      /* viewport related values */
      smViewportSize,
      /* color related semantics */
      smAmbient,
      smDiffuse,
      smEmissive,
      smSpecular,
      smOpacity,
      smSpecularPower,
      /* texture related semantics */
      smHeight,
      smNormal,
      /* semantic is unknown */
      smUnknown
    };

    enum SamplerType {
      st1D,
      st2D,
      st3D,
      stCube,
      st1DShadow,
      st2DShadow,
      stUnknown
    };

    virtual ~shader() {};

    virtual bool valid() = 0;
    virtual int passCount() = 0;
    virtual int techniqueCount() = 0;
    virtual const char* techniqueName( int n) = 0;
    virtual bool build() = 0;
    virtual void bind() = 0;
	virtual void setShapeDependentState() = 0;
    virtual void unbind() = 0;
    virtual void setTechnique( int t) = 0;
    virtual void setPass( int p) = 0;
    virtual const char* getVertexShader( int pass) = 0;
    virtual const char* getPixelShader( int pass) = 0;

    //need to have queries for attribs and uniforms
    virtual int uniformCount() = 0;
    virtual int samplerCount() = 0;
    virtual int attributeCount() = 0;
    virtual const char* uniformName(int i) = 0;
    virtual DataType uniformType(int i) = 0;
    virtual Semantic uniformSemantic(int i) = 0;
    virtual float* uniformDefault(int i) { return 0;};
    virtual const char* samplerName(int i) = 0;
    virtual SamplerType samplerType(int i) = 0;
    virtual const char* attributeName(int i) = 0;
    virtual DataType attributeType(int i) = 0;
    virtual int attributeHandle(int i) = 0;

    //need set functions for current values
    virtual void updateUniformBool( int i, bool val) = 0;
    virtual void updateUniformInt( int i, int val) = 0;
    virtual void updateUniformFloat( int i, float val) = 0;
    virtual void updateUniformBVec( int i, const bool* val) = 0;
    virtual void updateUniformIVec( int i, const int* val) = 0;
    virtual void updateUniformVec( int i, const float* val) = 0;
    virtual void updateUniformMat( int i, const float* val) = 0;
    virtual void updateUniformMat( int i, const double* val) = 0;
    virtual void updateSampler( int i, unsigned int val) = 0;

    //predefined attributes
    virtual bool usesColor() = 0;
    virtual bool usesNormal() = 0;
    virtual bool usesTexCoord( int set) = 0;
    virtual bool usesTangent() = 0;
    virtual bool usesBinormal() = 0;
    virtual int tangentSlot() = 0;
    virtual int binormalSlot() = 0;

    //error reporting
    virtual const char* errorString() = 0;

    static int size(DataType dt);
    static bool isBound(Semantic sm);
    static bool isColor(Semantic sm);
    static bool isClamped(Semantic sm);
    static void getLimits( Semantic sm, float& min, float& max);

    //how do we handle error reporting?
    static shader* create( const char *filename);
    static std::string sError;

    static bool sSupportSM3;
};

inline int shader::size(DataType dt) {

  switch (dt) {
    case dtFloat:
    case dtInt:
    case dtBool:
      return 1;
    case dtVec2:
    case dtIVec2:
    case dtBVec2:
      return 2;
    case dtVec3:
    case dtIVec3:
    case dtBVec3:
      return 3;
    case dtVec4:
    case dtIVec4:
    case dtBVec4:
      return 4;
    case dtMat2:
      return 4;
    case dtMat3:
      return 9;
    case dtMat4:
      return 16;
    case dtString:
	default:
	  break;
  };

  return 0;
}

inline bool shader::isColor(Semantic sm) {
  if ( (sm >= smAmbient) && (sm <= smSpecular))
    return true;

  return false;
}

inline bool shader::isBound(Semantic sm) {
  if ( (sm >= smWorld) && (sm <= smWorldViewProjectionIT) ||
       (sm >= smViewPosition) && (sm <= smViewportSize) )
    return true;

  return false;
}

inline bool shader::isClamped(Semantic sm) {
  if (isColor(sm) || (sm == smSpecularPower))
    return true;

  return false;
}

inline void shader::getLimits( Semantic sm, float& min, float& max) {
  if (isColor(sm)) {
    min = 0.0f;
    max = 1.0f;
  }
  else if (sm == smSpecularPower) {
    min = 1.0f;
    max = 128.0f;
  }
}

#endif //SHADER_H

