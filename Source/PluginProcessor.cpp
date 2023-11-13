/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParametersInfo.h"

//==============================================================================
DelayVSTAudioProcessor::DelayVSTAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), parameters(*this, nullptr, "Parameters", createParameters())
#endif
{
}

DelayVSTAudioProcessor::~DelayVSTAudioProcessor()
{
}

//==============================================================================
const juce::String DelayVSTAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DelayVSTAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DelayVSTAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DelayVSTAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double DelayVSTAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DelayVSTAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DelayVSTAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DelayVSTAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String DelayVSTAudioProcessor::getProgramName (int index)
{
    return {};
}

void DelayVSTAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void DelayVSTAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Todo: remove floats to a global const values.
    auto delayBufferSize = sampleRate * 10.0; // Ten seconds of audio. 
    delayBuffer.setSize(getTotalNumOutputChannels(), (int)delayBufferSize);
    smoothFeedback.reset(sampleRate, 0.001);
    smoothDelayTime.reset(sampleRate, 0.001);
}

void DelayVSTAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DelayVSTAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void DelayVSTAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();


    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        
        fillDelayBuffer(channel, buffer);
        fillBufferFromDelayBuffer(channel,  buffer, delayBuffer);
        fillDelayBuffer(channel,  buffer);
    }

    updateWritePosition(buffer, delayBuffer);

}

void DelayVSTAudioProcessor::fillDelayBuffer(int channel, juce::AudioBuffer<float>& buffer)
{
    auto* channelData = buffer.getWritePointer(channel);
    auto bufferSize = buffer.getNumSamples();
    auto delayBufferSize = delayBuffer.getNumSamples();

    // Check to see if main buffer copies to delay buffer without needing to  wrap.
    if (delayBufferSize >= writePosition + bufferSize )
    {
        // Copy main buffer contents to delay buffer without wrapping.
        delayBuffer.copyFrom(channel, writePosition, channelData, bufferSize);
    }
    else {
        // Calculate how much space is left at the end of the delay buffer.
        auto numSamplesToEnd = delayBufferSize - writePosition;

        // Copy that amount of contents to the end.
        delayBuffer.copyFrom(channel, writePosition, channelData, numSamplesToEnd);

        // Calculate how much contents is remaing to copy.
        auto numSamplesAtStart = bufferSize - numSamplesToEnd;

        // Copy that remaing amount to the begining of delay buffer.
        delayBuffer.copyFrom(channel, 0, buffer.getWritePointer(channel, numSamplesToEnd), numSamplesAtStart);
    }
}

void DelayVSTAudioProcessor::fillBufferFromDelayBuffer(int channel, juce::AudioBuffer<float>& buffer, juce::AudioBuffer<float>& delayBuffer)
{
    auto bufferSize = buffer.getNumSamples();
    auto delayBufferSize = delayBuffer.getNumSamples();
    auto delayTime = getSmoothValue(ParametersInfo::delayTime);
    auto feedback = getSmoothValue(ParametersInfo::feedback);
    auto delaySampleSize = delayTime * getSampleRate() / 1000.0f; // Todo: change float to a global const value (milisecondToSeconed).



    auto readPosition = std::round(writePosition - delaySampleSize);

    if (readPosition < 0)
        readPosition += delayBufferSize;

    if (readPosition + bufferSize < delayBufferSize)
    {
        buffer.addFromWithRamp(channel, 0, delayBuffer.getReadPointer(channel, readPosition), bufferSize, feedback, feedback);
    }
    else {
        auto numSamplesToEnd = delayBufferSize - readPosition;
        buffer.addFromWithRamp(channel, 0, delayBuffer.getReadPointer(channel, readPosition), numSamplesToEnd, feedback, feedback);

        auto numSamplesAtStart = bufferSize - numSamplesToEnd;
        buffer.addFromWithRamp(channel, numSamplesToEnd, delayBuffer.getReadPointer(channel, 0), numSamplesAtStart, feedback, feedback);

    }
}

void DelayVSTAudioProcessor::updateWritePosition(juce::AudioBuffer<float>& buffer, juce::AudioBuffer<float>& delayBuffer)
{
    auto bufferSize = buffer.getNumSamples();
    auto delayBufferSize = delayBuffer.getNumSamples();

    writePosition += bufferSize;
    writePosition %= delayBufferSize;
}

float  DelayVSTAudioProcessor::getSmoothValue(juce::Identifier identifier)
{
    auto id = ParametersInfo::getId(identifier);
    auto* targetValue = parameters.getRawParameterValue(id);

    // Todo: change string to a global const value.
    // change to switch case or different method, if we got more parameters.
    if (id == "FEEDBACK") {
        smoothFeedback.setTargetValue(targetValue->load());
        return smoothFeedback.getNextValue();
    }
    else 
    {
        smoothDelayTime.setTargetValue(targetValue->load());
        return smoothDelayTime.getNextValue();
    }
    
}

//==============================================================================
bool DelayVSTAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* DelayVSTAudioProcessor::createEditor()
{
     return new juce::GenericAudioProcessorEditor (*this);
    //return new DelayVSTAudioProcessorEditor(*this);
}

//==============================================================================
void DelayVSTAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void DelayVSTAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DelayVSTAudioProcessor();
}

juce::AudioProcessorValueTreeState::ParameterLayout DelayVSTAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    for (const auto& [key, param] : ParametersInfo::parameterInfoMap)
    {
        parameters.push_back(std::make_unique<juce::AudioParameterFloat>(param.id, param.labelName, param.defaultValue, param.lowerLimit, param.upperLimit));
    }

    return { parameters.begin(), parameters.end() };

}
