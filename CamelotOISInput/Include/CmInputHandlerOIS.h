#pragma once

#include "CmOISPrerequisites.h"
#include "CmInputHandler.h"

#include <OIS/OISEvents.h>
#include <OIS/OISInputManager.h>
#include <OIS/OISKeyboard.h>
#include <OIS/OISMouse.h>

namespace CamelotFramework
{
	class CM_OIS_EXPORT InputHandlerOIS : public InputHandler, public OIS::KeyListener, public OIS::MouseListener
	{
	public:
		InputHandlerOIS(unsigned int hWnd);
		virtual ~InputHandlerOIS();

		const WString& getInputString() const { return mInputString; }
	private:
		OIS::InputManager*	mInputManager;
		OIS::Mouse*			mMouse;
		OIS::Keyboard*		mKeyboard;
		WString				mInputString;

		virtual bool keyPressed(const OIS::KeyEvent& arg);
		virtual bool keyReleased(const OIS::KeyEvent& arg);
		virtual bool mouseMoved(const OIS::MouseEvent& arg);
		virtual bool mousePressed(const OIS::MouseEvent& arg, OIS::MouseButtonID id);
		virtual bool mouseReleased(const OIS::MouseEvent& arg, OIS::MouseButtonID id);

		virtual void update();
	};
}