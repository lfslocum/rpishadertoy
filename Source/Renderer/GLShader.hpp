/*******************************************************************************
*  GLShader.hpp - classes for manipulating gl shaders                          *
*                                                                              *
*  Joshua Slocum                                                      8/23/12  *
*******************************************************************************/


#include "GLCommon.h"


struct RendererParams
{
  GLuint current_time_ms; 
};

class GLProgram
{
public:
  GLProgram(string vert_source, string frag_source, RendererParams* params)
  ~GLProgram();

  bool initialize(void);
  bool verify(void);
  
protected:
  bool compile(void);
  bool link(void);
  bool use(void);

  GLuint program_id;
  GLuint vshader_id;
  GLuint fshader_id;

  virtual bool activateBuffers(void) = 0;
  virtual bool setUniforms(void) = 0;
private:
};




struct ShaderToyParams
{
  GLfloat orientation;
};


class ShaderToy : public GLProgram
{
public:
  ShaderToy(ShaderToyParams* toy_params);
  ~ShaderToy(void);

  void draw(void);
  
protected:
  bool activateBuffers(void);
  bool setUniforms(void);

private:
  VertexBuffer vb;
};
