/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

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
                       )
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
    auto delayBufferSize = sampleRate * 2.0; // Two seconds of audio.
    delayBuffer.setSize(getTotalNumOutputChannels(), (int)delayBufferSize);
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
    auto bufferSize = buffer.getNumSamples();
    auto delayBufferSize = delayBuffer.getNumSamples();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);
        fillDelayBuffer(channel, channelData, bufferSize, delayBufferSize);
        fillBufferFromDelayBuffer(channel, channelData, bufferSize, delayBufferSize, buffer);
    }

    writePosition += bufferSize;
    writePosition %= delayBufferSize;
}

void DelayVSTAudioProcessor::fillDelayBuffer(int channel, float* channelData, int bufferSize, int delayBufferSize)
{
    // Check to see if main buffer copies to delay buffer without needing to  wrap.
    if (writePosition + bufferSize < delayBufferSize)
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
        delayBuffer.copyFrom(channel, 0, channelData + numSamplesToEnd, numSamplesAtStart);
    }
}

void DelayVSTAudioProcessor::fillBufferFromDelayBuffer(int channel, float* channelData, int bufferSize, int delayBufferSize, juce::AudioBuffer<float>& buffer)
{
    // 1 second of audio drom the past.
    float delayRate = getSampleRate() * 1.0f;
    auto gainFactor = 0.7f;
    auto readPosition = writePosition - delayRate;

    if (readPosition < 0)
        readPosition += delayBufferSize;

    if (readPosition + bufferSize < delayBufferSize)
    {
        buffer.addFromWithRamp(channel, 0, delayBuffer.getReadPointer(channel, readPosition), bufferSize, gainFactor, gainFactor);
    }
    else {
        auto numSamplesToEnd = delayBufferSize - readPosition;
        buffer.addFromWithRamp(channel, 0, delayBuffer.getReadPointer(channel, readPosition), numSamplesToEnd, gainFactor, gainFactor);

        // auto numSamplesAtStart = readPosition - bufferSize;
        auto numSamplesAtStart = bufferSize - numSamplesToEnd;
        buffer.addFromWithRamp(channel, numSamplesToEnd, delayBuffer.getReadPointer(channel, 0), numSamplesAtStart, gainFactor, gainFactor);

    }
}

//==============================================================================
bool DelayVSTAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* DelayVSTAudioProcessor::createEditor()
{
    return new DelayVSTAudioProcessorEditor (*this);
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
