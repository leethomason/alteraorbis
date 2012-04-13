#include "rendertarget.h"
#include "platformgl.h"
#include "engine.h"

RenderTarget::RenderTarget( int width, int height, bool depthBuffer )
{
	CHECK_GL_ERROR;

	screenport = 0;
	savedScreenport = 0;

	glGenFramebuffers(1, &frameBufferID );
	glGenRenderbuffers(1, &depthBufferID ); // the depth buffer
	glGenTextures(1, &renderTextureID );

	glBindTexture( GL_TEXTURE_2D, renderTextureID );

	// Texture params. Be sure to clamp and turn filtering to not use mips.
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// generate the textures
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, 0 );

	// create 16 bit depth buffer
	glBindRenderbuffer( GL_RENDERBUFFER, depthBufferID );
	glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height );

	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	CHECK_GL_ERROR;

	gpuMem.w = width;
	gpuMem.h = height;
	gpuMem.format = Texture::RGB16;
	gpuMem.flags = Texture::PARAM_LINEAR;
	gpuMem.item = 0;
	gpuMem.inUse = true;
	gpuMem.glID = renderTextureID;

	texture.w = width;
	texture.h = height;
	texture.format = Texture::RGB16;
	texture.flags = Texture::PARAM_LINEAR;
	texture.creator = 0;
	texture.item = 0;
	texture.gpuMem = &gpuMem;
}


RenderTarget::~RenderTarget()
{
	CHECK_GL_ERROR;
	glDeleteRenderbuffers( 1, &depthBufferID );
	glDeleteTextures( 1, &renderTextureID );
	glDeleteFramebuffers( 1, &frameBufferID );
	CHECK_GL_ERROR;
	delete screenport;
}


void RenderTarget::SetActive( bool active, Engine* engine )
{

	if ( active ) {
		if ( !screenport ) {
			screenport = new Screenport( gpuMem.w, gpuMem.h, 0, gpuMem.h );
		}
		savedScreenport = engine->GetScreenportMutable();
		engine->SetScreenport( screenport );
		screenport->Resize( gpuMem.w, gpuMem.h, 0 );

		glBindFramebuffer( GL_FRAMEBUFFER, frameBufferID );
		glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTextureID, 0);
		glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBufferID );

#ifdef DEBUG
		int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		GLASSERT( status == GL_FRAMEBUFFER_COMPLETE);
#endif

		glClearColor(.0f, .0f, .0f, 1.0f);
		glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	}
	else {
		engine->SetScreenport( savedScreenport );
		savedScreenport->Resize( 0, 0, 0 );
		glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	}
}
