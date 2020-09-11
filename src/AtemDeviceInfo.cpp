#include "AtemDeviceInfo.h"


void get_switcher_inputs(const CComPtr<IBMDSwitcher>& switcher, std::vector<CComPtr<IBMDSwitcherInput>>& switcherInputs) {
	CComPtr<IBMDSwitcherInputIterator> inputIterator;
	if (switcher->CreateIterator(IID_IBMDSwitcherInputIterator, (void**)&inputIterator) == S_OK) {
		CComPtr<IBMDSwitcherInput> input;
		while (inputIterator->Next(&input) == S_OK)
			switcherInputs.push_back(std::move(input));
	}
}

void get_switcher_mix_effect_blocks(const CComPtr<IBMDSwitcher>& switcher, std::vector<CComPtr<IBMDSwitcherMixEffectBlock>>& mixEffectBlocks) {
	CComPtr<IBMDSwitcherMixEffectBlockIterator> mixEffectBlockIterator;
	if (switcher->CreateIterator(IID_IBMDSwitcherMixEffectBlockIterator, (void**)&mixEffectBlockIterator) == S_OK) {
		CComPtr<IBMDSwitcherMixEffectBlock> mixEffectBlock;
		while (mixEffectBlockIterator->Next(&mixEffectBlock) == S_OK)
			mixEffectBlocks.push_back(std::move(mixEffectBlock));
	}
}

int get_downstream_keyer_count(const CComPtr<IBMDSwitcher>& switcher) {
	int											downstreamKeyerCount = 0;
	CComPtr<IBMDSwitcherDownstreamKeyIterator>	dskIterator;
	CComPtr<IBMDSwitcherDownstreamKey>			dsk;

	if (switcher->CreateIterator(IID_IBMDSwitcherDownstreamKeyIterator, (void**)&dskIterator) != S_OK)
		return 0;

	while (dskIterator->Next(&dsk) == S_OK) {
		downstreamKeyerCount++;
		dsk.Release();
	}

	return downstreamKeyerCount;
}

bool does_support_advanced_chroma_keyers(const std::vector<CComPtr<IBMDSwitcherMixEffectBlock>>& mixEffectBlocks) {
	for (auto& mixEffectBlock : mixEffectBlocks) {
		CComPtr<IBMDSwitcherKeyIterator>	keyIterator;
		CComPtr<IBMDSwitcherKey>			keyer;

		if (mixEffectBlock->CreateIterator(IID_IBMDSwitcherKeyIterator, (void**)&keyIterator) != S_OK)
			continue;

		while (keyIterator->Next(&keyer) == S_OK) {
			BOOL advancedChromaSupported = FALSE;
			// Check whether advanced chroma keying is supported by switcher
			if ((keyer->DoesSupportAdvancedChroma(&advancedChromaSupported) == S_OK) && advancedChromaSupported)
				return true;

			keyer.Release();
		}
	}

	return false;
}

std::string get_product_name(const CComPtr<IBMDSwitcher>& switcher) {
	CComBSTR productNameString;

	// *** Print the product name of the Switcher
	if (switcher->GetProductName(&productNameString) != S_OK) {
		return "N/A";
	}

	int wlen = ::SysStringLen(productNameString);
	int mblen = ::WideCharToMultiByte(CP_ACP, 0, (wchar_t*)productNameString, wlen, NULL, 0, NULL, NULL);

	std::string productName(mblen, '\0');
	mblen = ::WideCharToMultiByte(CP_ACP, 0, (wchar_t*)productNameString, wlen, &productName[0], mblen, NULL, NULL);

	return productName;
}

int get_usk_count_for_meb(const CComPtr<IBMDSwitcherMixEffectBlock>& mixEffectBlock) {
	int									upstreamKeyerCount = 0;
	CComPtr<IBMDSwitcherKeyIterator>	keyIterator;
	CComPtr<IBMDSwitcherKey>			keyer;

	if (mixEffectBlock->CreateIterator(IID_IBMDSwitcherKeyIterator, (void**)&keyIterator) != S_OK)
		return 0;

	while (keyIterator->Next(&keyer) == S_OK) {
		upstreamKeyerCount++;
		keyer.Release();
	}

	return upstreamKeyerCount;
}

std::vector<std::string> get_transition_styles_for_meb(const CComPtr<IBMDSwitcherMixEffectBlock>& mixEffectBlock) {
	std::vector<std::string>	supportedStyles;

	for (auto& transitionStyle : kSwitcherTransitionStyles) {
		CComPtr<IUnknown> transitionParameters;

		if (mixEffectBlock->QueryInterface(transitionStyle.first, (void**)&transitionParameters) == S_OK)
			supportedStyles.push_back(transitionStyle.second);
	}

	return supportedStyles;
}

int get_input_type_count(const std::vector<CComPtr<IBMDSwitcherInput>>& switcherInputs, BMDSwitcherPortType portType) {
	int portCount = 0;

	for (auto& input : switcherInputs) {
		BMDSwitcherPortType type;
		if ((input->GetPortType(&type) == S_OK) && (type == portType))
			portCount++;
	}

	return portCount;
}

int get_media_pool_clip_count(const CComPtr<IBMDSwitcherMediaPool>& mediaPool) {
	unsigned int	clipCount;
	int				validClipCount = 0;

	if (mediaPool->GetClipCount(&clipCount) != S_OK)
		return 0;

	for (unsigned int i = 0; i < clipCount; i++) {
		// Count only clips marked as valid
		CComPtr<IBMDSwitcherClip> clip;
		if (mediaPool->GetClip(i, &clip) == S_OK) {
			BOOL isValid;
			if ((clip->IsValid(&isValid) == S_OK) && isValid)
				validClipCount++;
		}
	}

	return validClipCount;
}

int get_media_pool_stills_count(const CComPtr<IBMDSwitcherStills>& stills) {
	unsigned int	stillsCount;
	int				validStillsCount = 0;

	if (stills->GetCount(&stillsCount) != S_OK)
		return 0;

	for (unsigned int i = 0; i < stillsCount; i++) {
		// Count only stills marked as valid
		BOOL isValid;
		if ((stills->IsValid(i, &isValid) == S_OK) && isValid)
			validStillsCount++;
	}

	return validStillsCount;
}

void print_supported_video_modes(const CComPtr<IBMDSwitcher>& switcher) {
	printf("\nSwitcher Video Mode Support:\n");
	printf(" %-25s%-35s%s\n", "Video Mode", "HD Down Converted Video Mode", "MultiView Video Mode");

	for (auto& mode : kSwitcherVideoModes) {
		BOOL videoModeSupported;
		BMDSwitcherVideoMode hdDownConvertedVideoMode;
		BMDSwitcherVideoMode multiViewVideoMode;
		std::string hdDownConvertedVideoModeStr = "-----";
		std::string multiViewVideoModeStr = "-----";

		if ((switcher->DoesSupportVideoMode(mode.first, &videoModeSupported) != S_OK) || !videoModeSupported)
			continue;

		if (switcher->GetDownConvertedHDVideoMode(mode.first, &hdDownConvertedVideoMode) == S_OK)
			hdDownConvertedVideoModeStr = LookupString<BMDSwitcherVideoMode>(kSwitcherVideoModes, hdDownConvertedVideoMode);

		if (switcher->GetMultiViewVideoMode(mode.first, &multiViewVideoMode) == S_OK)
			multiViewVideoModeStr = LookupString<BMDSwitcherVideoMode>(kSwitcherVideoModes, multiViewVideoMode);

		printf(" %-25s%-35s%s\n", mode.second.c_str(), hdDownConvertedVideoModeStr.c_str(), multiViewVideoModeStr.c_str());
	}
}

void print_switcher_inputs(const std::vector<CComPtr<IBMDSwitcherInput>>& switcherInputs) {
	printf("\nSwitcher Inputs:\n");
	printf(" %-7s%-12s%-22s%s\n", "ID", "Short Name", "Long Name", "Type");
	for (auto& input : switcherInputs) {
		BMDSwitcherPortType portType;
		BMDSwitcherInputId inputId;
		BSTR longName[21];
		BSTR shortName[5];
		BMDSwitcherExternalPortType externalPortType;
		std::string portTypeStr;

		if (input->GetPortType(&portType) != S_OK)
			continue;

		if (input->GetInputId(&inputId) != S_OK)
			continue;

		if (input->GetShortName(shortName) != S_OK)
			continue;

		if (input->GetLongName(longName) != S_OK)
			continue;

		if ((portType == bmdSwitcherPortTypeExternal) && (input->GetCurrentExternalPortType(&externalPortType) != S_OK))
			continue;

		portTypeStr = LookupString<BMDSwitcherPortType>(kSwitcherPortTypes, portType);

		printf(" %-7lld%-12s%-22s%s",
			inputId,
			BSTRToCString(*shortName),
			BSTRToCString(*longName),
			portTypeStr.c_str());

		if (portType == bmdSwitcherPortTypeExternal)
			printf(" (%s)", LookupString<BMDSwitcherExternalPortType>(kSwitcherExternalPortTypes, externalPortType).c_str());

		printf("\n");
	}
}

void print_input_availability_matrix(const std::vector<CComPtr<IBMDSwitcherInput>>& switcherInputs, int mixEffectCount) {
	printf("\nSwitcher Input Availability Matrix:\n");
	printf(" %-7s", "Input");
	for (int i = 0; i < mixEffectCount; i++)
		printf("ME%d  ", i);
	printf("AUX  MV   SSA  SSB  CUT\n");

	for (auto& input : switcherInputs) {
		BMDSwitcherInputAvailability	inputAvailability;
		BSTR							shortName[5];

		if (input->GetInputAvailability(&inputAvailability) != S_OK)
			continue;

		if (input->GetShortName(shortName) != S_OK)
			continue;

		printf(" %-7s", BSTRToCString(*shortName));

		for (auto availability : kSwitcherInputAvailabilty) {
			if (((mixEffectCount < 2) && (availability == bmdSwitcherInputAvailabilityMixEffectBlock1)) ||
				((mixEffectCount < 3) && (availability == bmdSwitcherInputAvailabilityMixEffectBlock2)) ||
				((mixEffectCount < 4) && (availability == bmdSwitcherInputAvailabilityMixEffectBlock3))) {
				continue;
			}
			printf(" %s   ", (availability & inputAvailability) ? "*" : "-");
		}
		printf("\n");
	}
}

void print_fairlight_audio_inputs(const CComPtr<IBMDSwitcherFairlightAudioMixer>& fairlightAudioMixer, std::vector<CComPtr<IBMDSwitcherInput>>& switcherInputs) {
	BMDSwitcherInputId									mediaPlayerTargetId = 0;
	CComPtr<IBMDSwitcherFairlightAudioInputIterator>	fairlightAudioInputIterator;
	CComPtr<IBMDSwitcherFairlightAudioInput>			fairlightAudioInput;

	printf("\nFairlight Audio Mixer Inputs:\n");
	printf(" %-7s%s\n", "ID", "Type");

	if (fairlightAudioMixer->CreateIterator(IID_IBMDSwitcherFairlightAudioInputIterator, (void**)&fairlightAudioInputIterator) != S_OK)
		return;

	while (fairlightAudioInputIterator->Next(&fairlightAudioInput) == S_OK) {
		BMDSwitcherFairlightAudioInputType inputType;
		BMDSwitcherAudioInputId inputId;
		BMDSwitcherExternalPortType externalPortType;
		std::string inputTypeStr;
		std::string externalPortTypeStr;

		if (fairlightAudioInput->GetType(&inputType) != S_OK)
			continue;

		if (fairlightAudioInput->GetId(&inputId) != S_OK)
			continue;

		if (fairlightAudioInput->GetCurrentExternalPortType(&externalPortType) != S_OK)
			continue;

		inputTypeStr = LookupString<BMDSwitcherFairlightAudioInputType>(kSwitcherFairlightAudioInputTypes, inputType);
		externalPortTypeStr = LookupString<BMDSwitcherExternalPortType>(kSwitcherExternalPortTypes, externalPortType);

		printf(" %-7lld%s (%s",
			inputId,
			inputTypeStr.c_str(),
			externalPortTypeStr.c_str());
		if (inputType == bmdSwitcherFairlightAudioInputTypeEmbeddedWithVideo) {
			// Display the short name of the corresponding switcher input
			CComQIPtr<IBMDSwitcherInput> switcherInput = fairlightAudioInput;
			if (switcherInput) {
				BSTR inputShortName[5];
				if (switcherInput->GetShortName(inputShortName) == S_OK)
					printf(" - %s", BSTRToCString(*inputShortName));
			}
		} else if (inputType == bmdSwitcherFairlightAudioInputTypeMediaPlayer) {
			// Cycle through switcher inputs and match port type with Media Player Fill
			for (auto& switcherInput : switcherInputs) {
				BMDSwitcherPortType portType;
				BMDSwitcherInputId inputId;
				if ((switcherInput->GetPortType(&portType) == S_OK) &&
					(portType == bmdSwitcherPortTypeMediaPlayerFill) &&
					(switcherInput->GetInputId(&inputId) == S_OK) &&
					(inputId >= mediaPlayerTargetId)) {
					BSTR inputShortName[5];
					if (switcherInput->GetShortName(inputShortName) == S_OK)
						printf(" - %s", BSTRToCString(*inputShortName));
					mediaPlayerTargetId = inputId + 1;
					break;
				}
			}
		}

		printf(")\n");

		fairlightAudioInput.Release();
	}
}

void print_audio_inputs(const CComPtr<IBMDSwitcherAudioMixer>& audioMixer, std::vector<CComPtr<IBMDSwitcherInput>>& switcherInputs) {
	BMDSwitcherInputId							mediaPlayerTargetId = 0;
	CComPtr<IBMDSwitcherAudioInputIterator>		audioInputIterator;
	CComPtr<IBMDSwitcherAudioInput>				audioInput;

	printf("\nAudio Mixer Inputs:\n");
	printf(" %-7s%s\n", "ID", "Type");

	if (audioMixer->CreateIterator(IID_IBMDSwitcherAudioInputIterator, (void**)&audioInputIterator) != S_OK)
		return;

	while (audioInputIterator->Next(&audioInput) == S_OK) {
		BMDSwitcherAudioInputType inputType;
		BMDSwitcherAudioInputId inputId;
		BMDSwitcherExternalPortType externalPortType;
		std::string inputTypeStr;
		std::string externalPortTypeStr;

		if (audioInput->GetType(&inputType) != S_OK)
			continue;

		if (audioInput->GetAudioInputId(&inputId) != S_OK)
			continue;

		if (audioInput->GetCurrentExternalPortType(&externalPortType) != S_OK)
			continue;

		inputTypeStr = LookupString<BMDSwitcherAudioInputType>(kSwitcherAudioInputTypes, inputType);
		externalPortTypeStr = LookupString<BMDSwitcherExternalPortType>(kSwitcherExternalPortTypes, externalPortType);

		printf(" %-7lld%s (%s",
			inputId,
			inputTypeStr.c_str(),
			externalPortTypeStr.c_str());
		if (inputType == bmdSwitcherAudioInputTypeEmbeddedWithVideo) {
			// Display the short name of the corresponding switcher input
			CComQIPtr<IBMDSwitcherInput> switcherInput = audioInput;
			if (switcherInput) {
				BSTR inputShortName[5];
				if (switcherInput->GetShortName(inputShortName) == S_OK)
					printf(" - %s", BSTRToCString(*inputShortName));
			}
		} else if (inputType == bmdSwitcherAudioInputTypeMediaPlayer) {
			// Cycle through switcher inputs and match port type with Media Player Fill
			for (auto& switcherInput : switcherInputs) {
				BMDSwitcherPortType portType;
				BMDSwitcherInputId inputId;
				if ((switcherInput->GetPortType(&portType) == S_OK) &&
					(portType == bmdSwitcherPortTypeMediaPlayerFill) &&
					(switcherInput->GetInputId(&inputId) == S_OK) &&
					(inputId >= mediaPlayerTargetId)) {
					BSTR inputShortName[5];
					if (switcherInput->GetShortName(inputShortName) == S_OK)
						printf(" - %s", BSTRToCString(*inputShortName));
					mediaPlayerTargetId = inputId + 1;
					break;
				}
			}
		}

		printf(")\n");

		audioInput.Release();
	}
}

void print_media_pool_stills(const CComPtr<IBMDSwitcherStills>& stills) {
	unsigned int stillsCount;
	printf("\nMedia Pool Stills:\n");
	printf(" %-7s%s\n", "ID", "Name");

	if (stills->GetCount(&stillsCount) != S_OK)
		return;

	for (unsigned int i = 0; i < stillsCount; i++) {
		BOOL isValid;
		if ((stills->IsValid(i, &isValid) == S_OK) && isValid) {
			CComBSTR stillName;
			if (stills->GetName(i, &stillName) == S_OK)
				printf(" %-7d%s\n", i, BSTRToCString(stillName));
		}
	}
	printf("\n");
}

void print_media_pool_clips(const CComPtr<IBMDSwitcherMediaPool>& mediaPool) {
	unsigned int clipCount;
	printf("\nMedia Pool Clips:\n");
	printf(" %-7s%-40s%s\n", "ID", "Name", "Frame Count");

	if (mediaPool->GetClipCount(&clipCount) != S_OK)
		return;

	for (unsigned int i = 0; i < clipCount; i++) {
		CComPtr<IBMDSwitcherClip> clip;
		if (mediaPool->GetClip(i, &clip) == S_OK) {
			BOOL isValid;
			if ((clip->IsValid(&isValid) == S_OK) && isValid) {
				CComBSTR		clipName;
				unsigned int	clipIndex;
				unsigned int	clipFrameCount;
				if ((clip->GetName(&clipName) == S_OK) &&
					(clip->GetIndex(&clipIndex) == S_OK) &&
					(clip->GetFrameCount(&clipFrameCount) == S_OK))
					printf(" %-7d%-40s%u\n", clipIndex, BSTRToCString(clipName), clipFrameCount);
			}
		}
	}
	printf("\n");
}

const char* BSTRToCString(const CComBSTR& str) {
	_bstr_t bstr(str);
	const size_t newSize = (bstr.length() + 1) * 2;
	char* retString = new char[newSize];

	strcpy_s(retString, newSize, (char*)bstr);

	return retString;
}
