#ifndef RENDERTARGET_INCLUDED
#define RENDERTARGET_INCLUDED

class RenderTarget
{
public:
	RenderTarget( int width, int height, bool depthBuffer );

	void SetActive( bool active );

};


#endif // RENDERTARGET_INCLUDED