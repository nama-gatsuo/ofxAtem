#include "ofxAtem.h"

std::string convertToString(const BSTR& bstr) {
	CString cstr(bstr);
	CT2CA pszConvertedAnsiString(cstr);

	return std::string(pszConvertedAnsiString);
}

namespace ofxAtem {

	bool Device::connect(const std::string& ipAddress) {

		// Create an IBMDSwitcherDiscovery object to access switcher device
		HRESULT result = CoInitializeEx(NULL, NULL);
		if (FAILED(result)) {
			ofLogError(__FUNCTION__) << "Initialization of COM failed.";
			return false;
		}

		// Create an IBMDSwitcherDiscovery object to access switcher device
		result = switcherDiscovery.CoCreateInstance(CLSID_CBMDSwitcherDiscovery, NULL, CLSCTX_ALL);
		if (FAILED(result)) {
			ofLogError(__FUNCTION__) << "A Switcher Discovery instance could not be created.  The Switcher drivers may not be installed.";
			return false;
		}

		// Connect to switcher with address provided by arguments
		CComBSTR addressString = _com_util::ConvertStringToBSTR(ipAddress.data());
		BMDSwitcherConnectToFailure	connectToFailReason;
		result = switcherDiscovery->ConnectTo(addressString, &switcher, &connectToFailReason);
		if (FAILED(result)) {
			ofLogError(__FUNCTION__) << "Failed to connect to switcher at address " << ipAddress;
			return false;
		}

		productName = get_product_name(switcher);

		// Get the mix effect block iterator
		get_switcher_mix_effect_blocks(switcher, switcherMixEffectBlocks);

		// Create an InputMonitor for each input so we can catch any changes to input names
		get_switcher_inputs(switcher, switcherInputs);

		switcherMediaPool = switcher;

		switcherMonitor = new SwitcherMonitor();
		switcher->AddCallback(switcherMonitor);

		// For every input, install a callback to monitor property changes on the input
		for (auto input : switcherInputs) {
			InputMonitor* inputMonitor = new InputMonitor(input);
			//input->AddCallback(inputMonitor);
			inputMonitors.push_back(inputMonitor);
		}

		for (auto meb : switcherMixEffectBlocks) {
			MixEffectBlockMonitor* mixEffectBlockMonitor = new MixEffectBlockMonitor();
			meb->AddCallback(mixEffectBlockMonitor);
			mixEffectBlockMonitors.push_back(mixEffectBlockMonitor);
		}

		readInputMap();
		readActiveIds();

		ofAddListener(MixEffectBlockMonitor::effectBlockChanged, this, &Device::onMixEffectBlockUpdated);

		return true;

	}

	void Device::printInfo() {

		// Print current and MultiView video modes
		BMDSwitcherVideoMode currentVideoMode;
		if (SUCCEEDED(switcher->GetVideoMode(&currentVideoMode))) {
			std::string currentVideoModeStr = LookupString<BMDSwitcherVideoMode>(kSwitcherVideoModes, currentVideoMode);
			printf(" %-40s %s\n", "Current Video Mode:", currentVideoModeStr.c_str());

			BMDSwitcherVideoMode multiViewVideoMode;
			if (SUCCEEDED(switcher->GetMultiViewVideoMode(currentVideoMode, &multiViewVideoMode))) {
				std::string multiViewVideoModeStr = LookupString<BMDSwitcherVideoMode>(kSwitcherVideoModes, multiViewVideoMode);
				printf(" %-40s %s\n", "MultiView Video Mode:", multiViewVideoModeStr.c_str());
			}
		}

		// Print the power status of switcher
		BMDSwitcherPowerStatus powerStatus;
		if (SUCCEEDED(switcher->GetPowerStatus(&powerStatus))) {
			printf(" %-40s %s\n", "Power Supply 1:", powerStatus & bmdSwitcherPowerStatusSupply1 ? "Powered" : "Not powered");
			printf(" %-40s %s\n", "Power Supply 2:", powerStatus & bmdSwitcherPowerStatusSupply2 ? "Powered" : "Not powered");
		}

		// Print whether Fairlight or original audio mixer
		fairlightAudioMixer = switcher;
		printf(" %-40s %s\n", "Audio Mixer:", fairlightAudioMixer ? "Fairlight" : "Original");

		// Print Mix Effect block count
		printf(" %-40s %d\n", "Number of Mix Effect Blocks:", (int)switcherMixEffectBlocks.size());

		for (unsigned int i = 0; i < switcherMixEffectBlocks.size(); i++) {
			printf(" - Number of Upstream Keyers for ME%d:     %d\n", i, get_usk_count_for_meb(switcherMixEffectBlocks[i]));
			printf(" - Transition Styles supported by ME%d:    ", i);
			for (auto& transitionStyleStr : get_transition_styles_for_meb(switcherMixEffectBlocks[i]))
				printf("%s ", transitionStyleStr.c_str());
			printf("\n");
		}

		printf(" %-40s %s\n", "Supports Advanced Chroma Keyers:", does_support_advanced_chroma_keyers(switcherMixEffectBlocks) ? "Yes" : "No");
		printf(" %-40s %d\n", "Number of Downstream Keyers", get_downstream_keyer_count(switcher));

		// Print swicther input type counts
		printf(" %-40s %d\n", "Number of External Inputs:", get_input_type_count(switcherInputs, bmdSwitcherPortTypeExternal));
		printf(" %-40s %d\n", "Number of SuperSources:", get_input_type_count(switcherInputs, bmdSwitcherPortTypeSuperSource));
		printf(" %-40s %d\n", "Number of Media Players:", get_input_type_count(switcherInputs, bmdSwitcherPortTypeMediaPlayerFill));
		printf(" %-40s %d\n", "Number of AUX Outputs:", get_input_type_count(switcherInputs, bmdSwitcherPortTypeAuxOutput));

		// Get Switcher Media pool.

		if (switcherMediaPool) {
			// Get Switcher stills interfaceobject
			if (switcherMediaPool->GetStills(&switcherStills) == S_OK) {
				printf(" %-40s %u\n", "Number of Stills in Media Pool:", get_media_pool_stills_count(switcherStills));
			}

			printf(" %-40s %u\n", "Number of Clips in Media Pool:", get_media_pool_clip_count(switcherMediaPool));
		}

		print_supported_video_modes(switcher);
		print_switcher_inputs(switcherInputs);
		print_input_availability_matrix(switcherInputs, (int)switcherMixEffectBlocks.size());
		if (fairlightAudioMixer) {
			// Print Fairlight audiomixer inputs
			print_fairlight_audio_inputs(fairlightAudioMixer, switcherInputs);
		} else {
			// Print original audio mixer inputs
			CComQIPtr<IBMDSwitcherAudioMixer> audioMixer = switcher;
			if (audioMixer)
				print_audio_inputs(audioMixer, switcherInputs);
		}

		if (switcherStills) {
			print_media_pool_stills(switcherStills);
		}

		if (switcherMediaPool) {
			print_media_pool_clips(switcherMediaPool);
		}

	}

	void Device::disconnect() {
		// Uninitalize COM on this thread
		CoUninitialize();

		switcherDiscovery.Release();

		switcher->RemoveCallback(switcherMonitor);
		switcher.Release();
		switcherMonitor->Release();

		for (int i = 0; i < switcherInputs.size(); i++) {
			switcherInputs[i]->RemoveCallback(inputMonitors[i]);
			switcherInputs[i].Release();
			inputMonitors[i]->Release();
		}
		for (int i = 0; i < switcherMixEffectBlocks.size(); i++) {
			//switcherMixEffectBlocks[i]->RemoveCallback(mixEffectBlockMonitors[i]);
			switcherMixEffectBlocks[i].Release();
			mixEffectBlockMonitors[i]->Release();
		}
		switcherMediaPool.Release();
		switcherStills.Release();
		fairlightAudioMixer.Release();

		ofRemoveListener(MixEffectBlockMonitor::effectBlockChanged, this, &Device::onMixEffectBlockUpdated);

	}

	bool Device::setProgramByIndex(int index) {
		BMDSwitcherInputId bmdId = inputMap[index]->bmdId;
		if (FAILED(switcherMixEffectBlocks[0]->SetProgramInput(bmdId))) return false;
		return true;
	}

	bool Device::setPreviewByIndex(int index) {
		BMDSwitcherInputId bmdId = inputMap[index]->bmdId;
		if (FAILED(switcherMixEffectBlocks[0]->SetPreviewInput(bmdId))) return false;
		return true;
	}

	int Device::getProgramIndex() const { return currentProgram->index; }

	int Device::getPreviewIndex() const { return currentPreview->index; }

	void Device::onMixEffectBlockUpdated(BMDSwitcherMixEffectBlockEventType& e) {
		if (e == bmdSwitcherMixEffectBlockEventTypeProgramInputChanged ||
			e == bmdSwitcherMixEffectBlockEventTypePreviewInputChanged) {

			readActiveIds();
		}
	}

	bool Device::readActiveIds() {

		HRESULT resultProgram, resultPreview;

		BMDSwitcherInputId bmdProgramId;
		resultProgram = switcherMixEffectBlocks[0]->GetProgramInput(&bmdProgramId);

		for (int i = 0; i < inputMap.size(); i++) {
			if (inputMap[i]->bmdId == bmdProgramId) {
				currentProgram = inputMap[i];
				break;
			}
		}

		BMDSwitcherInputId bmdPreviewId;
		resultPreview = switcherMixEffectBlocks[0]->GetPreviewInput(&bmdPreviewId);

		for (int i = 0; i < inputMap.size(); i++) {
			if (inputMap[i]->bmdId == bmdPreviewId) {
				currentPreview = inputMap[i];
				break;
			}
		}

		return SUCCEEDED(resultProgram) && SUCCEEDED(resultPreview);
	}

	bool Device::readInputMap() {

		inputMap.clear();

		HRESULT result;
		IBMDSwitcherInputIterator* inputIterator = NULL;
		IBMDSwitcherInput* input = NULL;

		result = switcher->CreateIterator(IID_IBMDSwitcherInputIterator, (void**)&inputIterator);
		if (FAILED(result)) {
			ofLogError() << "Could not create IBMDSwitcherInputIterator iterator";
			return false;
		}

		BSTR longName, shortName;
		int index = 0;
		while (S_OK == inputIterator->Next(&input)) {
			BMDSwitcherInputId id;
			BMDSwitcherPortType portType;

			input->GetInputId(&id);
			input->GetLongName(&longName);
			input->GetShortName(&shortName);
			input->GetPortType(&portType);

			std::string portTypeStr = LookupString<BMDSwitcherPortType>(kSwitcherPortTypes, portType);
			if (portType == bmdSwitcherPortTypeExternal) {
				BMDSwitcherExternalPortType externalPortType;
				input->GetCurrentExternalPortType(&externalPortType);
				portTypeStr += " (" + LookupString<BMDSwitcherExternalPortType>(kSwitcherExternalPortTypes, externalPortType) + ")";
			}


			ofPtr<Input> inputPtr = std::make_shared<Input>();
			inputPtr->bmdId = id;
			inputPtr->index = index;
			inputPtr->longName = convertToString(longName);
			inputPtr->shortName = convertToString(shortName);
			inputPtr->portType = portTypeStr;

			inputMap.push_back(std::move(inputPtr));

			input->Release();

			index++;
		}

		inputIterator->Release();

		return true;
	}




}



