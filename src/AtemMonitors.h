#pragma once

#include "BMDSwitcherAPI_h.h"
#include "ofLog.h"
#include "ofEvent.h"
#include "ofEventUtils.h"

// Callback class for monitoring property changes on a mix effect block.
class MixEffectBlockMonitor : public IBMDSwitcherMixEffectBlockCallback {
public:
	MixEffectBlockMonitor() : mRefCount(1) {}
	virtual ~MixEffectBlockMonitor() {}

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID* ppv) {
		if (!ppv)
			return E_POINTER;

		if (IsEqualGUID(iid, IID_IBMDSwitcherMixEffectBlockCallback)) {
			*ppv = static_cast<IBMDSwitcherMixEffectBlockCallback*>(this);
			AddRef();
			return S_OK;
		}

		if (IsEqualGUID(iid, IID_IUnknown)) {
			*ppv = static_cast<IUnknown*>(this);
			AddRef();
			return S_OK;
		}

		*ppv = NULL;
		return E_NOINTERFACE;
	}

	ULONG STDMETHODCALLTYPE AddRef(void) {
		return InterlockedIncrement(&mRefCount);
	}

	ULONG STDMETHODCALLTYPE Release(void) {
		int newCount = InterlockedDecrement(&mRefCount);
		if (newCount == 0)
			delete this;
		return newCount;
	}

	HRESULT STDMETHODCALLTYPE Notify(BMDSwitcherMixEffectBlockEventType eventType) override {

		ofNotifyEvent(effectBlockChanged, eventType);

		switch (eventType) {
		case bmdSwitcherMixEffectBlockEventTypeProgramInputChanged:
			ofLogVerbose() << "program input changed";
			break;
		case bmdSwitcherMixEffectBlockEventTypePreviewInputChanged:
			ofLogVerbose() << "preview input changed";
			break;
		case bmdSwitcherMixEffectBlockEventTypeInTransitionChanged:
			ofLogVerbose() << "in transition changed";
			break;
		case bmdSwitcherMixEffectBlockEventTypeTransitionPositionChanged:
			ofLogVerbose() << "transition position changed";
			break;
		case bmdSwitcherMixEffectBlockEventTypeTransitionFramesRemainingChanged:
			ofLogVerbose() << "transition frame remaining changed";
			break;
		case bmdSwitcherMixEffectBlockEventTypeFadeToBlackFramesRemainingChanged:
			ofLogVerbose() << "fade to black frames remaining changed";
			break;
		default:	// ignore other property changes not used for this sample app
			break;
		}
		return S_OK;
	}

	static ofEvent<BMDSwitcherMixEffectBlockEventType> effectBlockChanged;

private:
	LONG mRefCount;
};

// Monitor the properties on Switcher Inputs.
// In this sample app we're only interested in changes to the Long Name property to update the PopupButton list
class InputMonitor : public IBMDSwitcherInputCallback {
public:
	InputMonitor(IBMDSwitcherInput* input) : mInput(input), mRefCount(1) {
		mInput->AddRef();
		mInput->AddCallback(this);
	}

	virtual ~InputMonitor() {
		mInput->Release();
	}

	void RemoveCallback(void) { mInput->RemoveCallback(this); }

	// IBMDSwitcherInputCallback interface
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID* ppv) {
		if (!ppv)
			return E_POINTER;

		if (IsEqualGUID(iid, IID_IBMDSwitcherInputCallback)) {
			*ppv = static_cast<IBMDSwitcherInputCallback*>(this);
			AddRef();
			return S_OK;
		}

		if (IsEqualGUID(iid, IID_IUnknown)) {
			*ppv = static_cast<IUnknown*>(this);
			AddRef();
			return S_OK;
		}

		*ppv = NULL;
		return E_NOINTERFACE;
	}

	ULONG STDMETHODCALLTYPE AddRef(void) {
		return InterlockedIncrement(&mRefCount);
	}

	ULONG STDMETHODCALLTYPE Release(void) {
		int newCount = InterlockedDecrement(&mRefCount);
		if (newCount == 0)
			delete this;
		return newCount;
	}

	HRESULT STDMETHODCALLTYPE Notify(BMDSwitcherInputEventType eventType) override {
		switch (eventType) {
		case bmdSwitcherInputEventTypeLongNameChanged:
			ofLogNotice() << "switcher input longname changed";
			//PostMessage(mHwnd, WM_SWITCHER_INPUT_LONGNAME_CHANGED, 0, 0);
			break;
		default:	// ignore other property changes not used for this sample app
			break;
		}
		return S_OK;
	}

	IBMDSwitcherInput* input() { return mInput; }

private:
	IBMDSwitcherInput* mInput;
	LONG mRefCount;
};

// Callback class to monitor switcher disconnection
class SwitcherMonitor : public IBMDSwitcherCallback {
public:
	SwitcherMonitor() : mRefCount(1) {}
	virtual ~SwitcherMonitor() {}

	// IBMDSwitcherCallback interface
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID* ppv) {
		if (!ppv)
			return E_POINTER;

		if (IsEqualGUID(iid, IID_IBMDSwitcherCallback)) {
			*ppv = static_cast<IBMDSwitcherCallback*>(this);
			AddRef();
			return S_OK;
		}

		if (IsEqualGUID(iid, IID_IUnknown)) {
			*ppv = static_cast<IUnknown*>(this);
			AddRef();
			return S_OK;
		}

		*ppv = NULL;
		return E_NOINTERFACE;
	}

	ULONG STDMETHODCALLTYPE AddRef(void) {
		return InterlockedIncrement(&mRefCount);
	}

	ULONG STDMETHODCALLTYPE Release(void) {
		int newCount = InterlockedDecrement(&mRefCount);
		if (newCount == 0)
			delete this;
		return newCount;
	}

	// Switcher event callback
	HRESULT STDMETHODCALLTYPE Notify(BMDSwitcherEventType eventType, BMDSwitcherVideoMode coreVideoMode) {
		if (eventType == bmdSwitcherEventTypeDisconnected) {
			ofLogNotice() << "switcher disconnected.";
			//PostMessage(mHwnd, WM_SWITCHER_DISCONNECTED, 0, 0);
		}

		return S_OK;
	}

private:
	LONG mRefCount;
};