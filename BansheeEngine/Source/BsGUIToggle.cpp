#include "BsGUIToggle.h"
#include "BsImageSprite.h"
#include "BsGUIWidget.h"
#include "BsGUISkin.h"
#include "BsSpriteTexture.h"
#include "BsTextSprite.h"
#include "BsGUILayoutOptions.h"
#include "BsGUIMouseEvent.h"
#include "CmTexture.h"

using namespace CamelotFramework;

namespace BansheeEngine
{
	const String& GUIToggle::getGUITypeName()
	{
		static String name = "Toggle";
		return name;
	}

	GUIToggle::GUIToggle(GUIWidget& parent, const GUIElementStyle* style, const WString& text, const GUILayoutOptions& layoutOptions)
		:GUIElement(parent, style, layoutOptions), mText(text), mNumImageRenderElements(0), mIsToggled(false)
	{
		mImageSprite = cm_new<ImageSprite, PoolAlloc>();
		mTextSprite = cm_new<TextSprite, PoolAlloc>();

		mImageDesc.texture = mStyle->normal.texture;

		if(mImageDesc.texture != nullptr)
		{
			mImageDesc.width = mImageDesc.texture->getTexture()->getWidth();
			mImageDesc.height = mImageDesc.texture->getTexture()->getHeight();
		}

		mImageDesc.borderLeft = mStyle->border.left;
		mImageDesc.borderRight = mStyle->border.right;
		mImageDesc.borderTop = mStyle->border.top;
		mImageDesc.borderBottom = mStyle->border.bottom;
	}

	GUIToggle::~GUIToggle()
	{
		cm_delete<PoolAlloc>(mTextSprite);
		cm_delete<PoolAlloc>(mImageSprite);
	}

	GUIToggle* GUIToggle::create(GUIWidget& parent, const WString& text, const GUIElementStyle* style)
	{
		if(style == nullptr)
		{
			const GUISkin* skin = parent.getSkin();
			style = skin->getStyle(getGUITypeName());
		}

		return new (cm_alloc<GUIToggle, PoolAlloc>()) GUIToggle(parent, style, text, getDefaultLayoutOptions(style));
	}

	GUIToggle* GUIToggle::create(GUIWidget& parent, const GUILayoutOptions& layoutOptions, const WString& text, const GUIElementStyle* style)
	{
		if(style == nullptr)
		{
			const GUISkin* skin = parent.getSkin();
			style = skin->getStyle(getGUITypeName());
		}

		return new (cm_alloc<GUIToggle, PoolAlloc>()) GUIToggle(parent, style, text, layoutOptions);
	}

	UINT32 GUIToggle::getNumRenderElements() const
	{
		UINT32 numElements = mImageSprite->getNumRenderElements();
		numElements += mTextSprite->getNumRenderElements();

		return numElements;
	}

	const HMaterial& GUIToggle::getMaterial(UINT32 renderElementIdx) const
	{
		if(renderElementIdx >= mNumImageRenderElements)
			return mTextSprite->getMaterial(mNumImageRenderElements - renderElementIdx);
		else
			return mImageSprite->getMaterial(renderElementIdx);
	}

	UINT32 GUIToggle::getNumQuads(UINT32 renderElementIdx) const
	{
		UINT32 numQuads = 0;
		if(renderElementIdx >= mNumImageRenderElements)
			numQuads = mTextSprite->getNumQuads(mNumImageRenderElements - renderElementIdx);
		else
			numQuads = mImageSprite->getNumQuads(renderElementIdx);

		return numQuads;
	}

	void GUIToggle::updateRenderElementsInternal()
	{		
		mImageDesc.offset = mOffset;
		mImageDesc.width = mWidth;
		mImageDesc.height = mHeight;
		mImageDesc.clipRect = mClipRect;

		mImageSprite->update(mImageDesc);
		mBounds = mImageSprite->getBounds();
		mNumImageRenderElements = mImageSprite->getNumRenderElements();

		TEXT_SPRITE_DESC textDesc;
		textDesc.text = mText;
		textDesc.font = mStyle->font;
		textDesc.fontSize = mStyle->fontSize;

		Rect contentBounds = mBounds;

		contentBounds.x += mStyle->margins.left + mStyle->contentOffset.left;
		contentBounds.y += mStyle->margins.top + mStyle->contentOffset.top;
		contentBounds.width = (UINT32)std::max(0, (INT32)contentBounds.width - 
			(INT32)(mStyle->margins.left + mStyle->margins.right + mStyle->contentOffset.left + mStyle->contentOffset.right));
		contentBounds.height = (UINT32)std::max(0, (INT32)contentBounds.height - 
			(INT32)(mStyle->margins.top + mStyle->margins.bottom + mStyle->contentOffset.top + mStyle->contentOffset.bottom));

		textDesc.offset = Int2(contentBounds.x, contentBounds.y);
		textDesc.width = contentBounds.width;
		textDesc.height = contentBounds.height;
		textDesc.clipRect = Rect(0, 0, textDesc.width, textDesc.height);
		textDesc.horzAlign = mStyle->textHorzAlign;
		textDesc.vertAlign = mStyle->textVertAlign;

		mTextSprite->update(textDesc);
	}

	UINT32 GUIToggle::_getOptimalWidth() const
	{
		if(mImageDesc.texture != nullptr)
		{
			return mImageDesc.texture->getTexture()->getWidth();
		}

		return 0;
	}

	UINT32 GUIToggle::_getOptimalHeight() const
	{
		if(mImageDesc.texture != nullptr)
		{
			return mImageDesc.texture->getTexture()->getHeight();
		}

		return 0;
	}

	UINT32 GUIToggle::_getRenderElementDepth(UINT32 renderElementIdx) const
	{
		if(renderElementIdx >= mNumImageRenderElements)
			return _getDepth();
		else
			return _getDepth() + 1;
	}

	void GUIToggle::fillBuffer(UINT8* vertices, UINT8* uv, UINT32* indices, UINT32 startingQuad, UINT32 maxNumQuads, 
		UINT32 vertexStride, UINT32 indexStride, UINT32 renderElementIdx) const
	{
		if(renderElementIdx >= mNumImageRenderElements)
			mTextSprite->fillBuffer(vertices, uv, indices, startingQuad, maxNumQuads, vertexStride, indexStride, mNumImageRenderElements - renderElementIdx);
		else
			mImageSprite->fillBuffer(vertices, uv, indices, startingQuad, maxNumQuads, vertexStride, indexStride, renderElementIdx);
	}

	bool GUIToggle::mouseEvent(const GUIMouseEvent& ev)
	{
		if(ev.getType() == GUIMouseEventType::MouseOver)
		{
			if(mIsToggled)
				mImageDesc.texture = mStyle->hoverOn.texture;
			else
				mImageDesc.texture = mStyle->hover.texture;

			markAsDirty();
			return true;
		}
		else if(ev.getType() == GUIMouseEventType::MouseOut)
		{
			if(mIsToggled)
				mImageDesc.texture = mStyle->normalOn.texture;
			else
				mImageDesc.texture = mStyle->normal.texture;

			markAsDirty();
			return true;
		}
		else if(ev.getType() == GUIMouseEventType::MouseDown)
		{
			if(mIsToggled)
				mImageDesc.texture = mStyle->activeOn.texture;
			else
				mImageDesc.texture = mStyle->active.texture;

			markAsDirty();
			return true;
		}
		else if(ev.getType() == GUIMouseEventType::MouseUp)
		{
			mIsToggled = !mIsToggled;

			if(mIsToggled)
				mImageDesc.texture = mStyle->normalOn.texture;
			else
				mImageDesc.texture = mStyle->normal.texture;

			markAsDirty();
			return true;
		}

		return false;
	}
}