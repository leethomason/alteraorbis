#include "rendertarget.h"
#include "platformgl.h"

RenderTarget::RenderTarget( int width, int height, bool depthBuffer )
{
/*	
	glGenFramebuffers(1, fb, 0);
	glGenRenderbuffers(1, depthRb, 0); // the depth buffer
	glGenTextures(1, renderTex, 0);

// generate texture
GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, renderTex[0]);

// parameters - we have to make sure we clamp the textures to the edges
GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S,
		GLES20.GL_CLAMP_TO_EDGE);
GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T,
		GLES20.GL_CLAMP_TO_EDGE);
GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER,
		GLES20.GL_LINEAR);
GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER,
		GLES20.GL_LINEAR);

// create it
// create an empty intbuffer first
int[] buf = new int[texW * texH];
texBuffer = ByteBuffer.allocateDirect(buf.length
		* FLOAT_SIZE_BYTES).order(ByteOrder.nativeOrder()).asIntBuffer();;

// generate the textures
GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_RGB, texW, texH, 0, GLES20.GL_RGB, GLES20.GL_UNSIGNED_SHORT_5_6_5, texBuffer);

// create render buffer and bind 16-bit depth buffer
GLES20.glBindRenderbuffer(GLES20.GL_RENDERBUFFER, depthRb[0]);
GLES20.glRenderbufferStorage(GLES20.GL_RENDERBUFFER, GLES20.GL_DEPTH_COMPONENT16, texW, texH);
*/
}
