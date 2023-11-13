/*
  ==============================================================================

    ParametersInfo.cpp
    Created: 12 Nov 2023 3:39:19pm
    Author:  AudioLab

  ==============================================================================
*/

#include "ParametersInfo.h"

const juce::Identifier ParametersInfo::delayTime{ "delayTime" };
const juce::Identifier ParametersInfo::feedback{ "feedback" };
/*

Todo: Featues to add:

const juce::Identifier ParametersInfo::mix{ "mix" };
const juce::Identifier ParametersInfo::lowpass{ "lowpass" };
const juce::Identifier ParametersInfo::highpass{ "highpass" };
*/

std::map<juce::Identifier, ParametersInfo::ParameterValues> ParametersInfo::parameterInfoMap
{
    { ParametersInfo::delayTime, { "DELAYMS", "Delay Ms", 0.0f, 1000.0f, 0.2f } },
    { ParametersInfo::feedback, { "FEEDBACK", "Feedback", 0.0f, 1.0f, 0.4f } },
    /*
    
    Todo: Featues to add:

    { mix, { "Mix", 0.5f, 0.0f, 1.0f, 0.1f } },
    { lowpass, { "Lowpass", 15000.0f, 400.0f, 21000.0f, 0.01f } },
    { highpass, { "Highpass", 300.0f, 1.0f, 3000.0f, 0.01f } },
    */
};

juce::String ParametersInfo::getId(juce::Identifier identifier) {
    auto param = ParametersInfo::parameterInfoMap.find(identifier);

    return param->second.id;
}