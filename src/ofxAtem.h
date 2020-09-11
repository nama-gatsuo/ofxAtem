#pragma once

#include "ofMain.h"
#include "AtemDeviceInfo.h"
#include "AtemMonitors.h"

namespace ofxAtem {

	struct Input {
		std::string shortName;
		std::string longName;
		BMDSwitcherInputId bmdId;
		int index;
		std::string portType;
	};

	class Device {
	public:
		Device() {}
		~Device() {}

		bool connect(const std::string& ipAddress);
		void printInfo();
		void disconnect();

		bool setProgramByIndex(int index);
		bool setPreviewByIndex(int index);
		int getProgramIndex() const;
		int getPreviewIndex() const;

		const std::string& getProductName() const { return productName; }

		const std::vector<ofPtr<Input>>& getInputMap() const { return inputMap; }

		void onMixEffectBlockUpdated(BMDSwitcherMixEffectBlockEventType& e);

	private:
		bool readActiveIds();
		bool readInputMap();

		CComPtr<IBMDSwitcherDiscovery> switcherDiscovery;
		CComPtr<IBMDSwitcher> switcher;
		std::vector<CComPtr<IBMDSwitcherInput>>	switcherInputs;
		std::vector<CComPtr<IBMDSwitcherMixEffectBlock>> switcherMixEffectBlocks;
		CComQIPtr<IBMDSwitcherMediaPool> switcherMediaPool;
		CComPtr<IBMDSwitcherStills>	switcherStills;
		CComQIPtr<IBMDSwitcherFairlightAudioMixer> fairlightAudioMixer;
		std::string	productName;

		SwitcherMonitor* switcherMonitor;
		std::vector<InputMonitor*> inputMonitors;
		std::vector<MixEffectBlockMonitor*> mixEffectBlockMonitors;

		std::vector<ofPtr<Input>> inputMap;
		ofPtr<Input> currentProgram, currentPreview;
	};


}
