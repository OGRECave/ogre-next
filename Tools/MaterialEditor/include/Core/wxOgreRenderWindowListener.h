
#ifndef __wxOgreRenderWindowListener_defined__
#define __wxOgreRenderWindowListener_defined__

class wxMouseEvent;
class wxKeyEvent;

class wxOgreRenderWindowListener
{
public:
	virtual void OnMouseEvents( wxMouseEvent &evt ) = 0;
	virtual void OnKeyDown( wxKeyEvent &evt ) = 0;
	virtual void OnKeyUp( wxKeyEvent &evt ) = 0;
};

#endif
