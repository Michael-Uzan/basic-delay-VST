/*
  ==============================================================================

    ParametersInfo.h
    Created: 12 Nov 2023 3:39:19pm
    Author:  AudioLab

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <map>

namespace ParametersInfo
{
    struct ParameterValues
    {
        juce::String id;
        juce::String labelName;
        float defaultValue;
        float lowerLimit;
        float upperLimit;
    };

    extern const juce::Identifier delayTime;
    extern const juce::Identifier feedback;
    /*

    Todo: Featues to add:

    extern const juce::Identifier mix;
    extern const juce::Identifier lowpass;
    extern const juce::Identifier highpass;
    */

    extern std::map<juce::Identifier, ParameterValues> parameterInfoMap;
    juce::String getId(juce::Identifier identifier);
}