#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PluginProcessor::PluginProcessor()
    : AudioProcessor (BusesProperties()
#if !JucePlugin_IsMidiEffect
    #if !JucePlugin_IsSynth
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
    #endif
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
#endif
              ),
      parameters (*this, nullptr, "Parameters", createParameterLayout())
{
    wet_gain = static_cast<juce::AudioParameterFloat*> (parameters.getParameter ("wet_gain"));
    dry_gain = static_cast<juce::AudioParameterFloat*> (parameters.getParameter ("dry_gain"));
    crush = static_cast<juce::AudioParameterChoice*> (parameters.getParameter ("crush"));
    preset = static_cast<juce::AudioParameterChoice*> (parameters.getParameter ("preset"));

    lastLoadedPreset = -1;
    lastCrush = 0;


    verb_.init (48000);
}

PluginProcessor::~PluginProcessor()
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout PluginProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>("wet_gain", "Wet Gain", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("dry_gain", "Dry Gain", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("preset", "Preset", juce::StringArray{ "Preset 1", "Preset 2", "Preset 3", "Preset 4", "Preset 5", "Preset 6", "Preset 7", "Preset 8", "Preset 9", "Preset 10" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("crush", "Crush", juce::StringArray { "Hi Def", "OG", "Crushed", "Scrunted" }, 0));

    return { params.begin(), params.end() };
}

const juce::String PluginProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PluginProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool PluginProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool PluginProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double PluginProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PluginProcessor::getNumPrograms()
{
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
        // so this should be at least 1, even if you're not really implementing programs.
}

int PluginProcessor::getCurrentProgram()
{
    return 0;
}

void PluginProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String PluginProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void PluginProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    verb_.init (sampleRate);
    juce::ignoreUnused (samplesPerBlock);
}

void PluginProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

        // This checks if the input layout matches the output layout
    #if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    #endif

    return true;
#endif
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    // Ensure we have at least 2 output channels (stereo)
    jassert (totalNumOutputChannels >= 2);

    verb_.setWetGain (wet_gain->get());
    verb_.setDryGain (dry_gain->get());

    // Handle preset changes
    int currentPreset = preset->getIndex();
    if (currentPreset != lastLoadedPreset)
    {
        verb_.setPreset (currentPreset);
        lastLoadedPreset = currentPreset;
    }

    // Handle crush changes
    int currentCrush = crush->getIndex();

    // Clear any output channels beyond the first two
    for (auto i = 2; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    float* leftChannel = buffer.getWritePointer (0);
    float* rightChannel = buffer.getWritePointer (1);

    // Apply downsampling if crush is 1 or greater
    if (currentCrush >= 1)
    {
        // Temporary buffers for downsampled audio
        std::vector<float> leftDownsampled (buffer.getNumSamples() / 2);
        std::vector<float> rightDownsampled (buffer.getNumSamples() / 2);

        // Downsample
        for (int i = 0; i < buffer.getNumSamples() / 2; ++i)
        {
            leftDownsampled[i] = leftChannel[i * 2];
            if (totalNumInputChannels > 1)
            {
                rightDownsampled[i] = rightChannel[i * 2];
            }
        }

        // Upsample using linear interpolation
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float fraction = (float) (i % 2) / 2.0f;
            int index = i / 2;
            int nextIndex = (index + 1) % (buffer.getNumSamples() / 2);

            leftChannel[i] = leftDownsampled[index] * (1 - fraction) + leftDownsampled[nextIndex] * fraction;
            if (totalNumInputChannels > 1)
            {
                rightChannel[i] = rightDownsampled[index] * (1 - fraction) + rightDownsampled[nextIndex] * fraction;
            }
        }
    }

    // Apply bitcrushing
    if (currentCrush >= 2)
    {
        int bitDepth = (currentCrush == 2) ? 12 : 10;
        float crushFactor = pow (2.0f, bitDepth - 1);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            leftChannel[sample] = round (leftChannel[sample] * crushFactor) / crushFactor;
            if (totalNumInputChannels > 1)
            {
                rightChannel[sample] = round (rightChannel[sample] * crushFactor) / crushFactor;
            }
        }
    }

    if (totalNumInputChannels == 1)
    {
        // Mono input: use the same input for both channels of the reverb
        verb_.process (leftChannel, leftChannel, buffer.getNumSamples());
        // Copy the processed left channel to the right channel
        memcpy (rightChannel, leftChannel, sizeof (float) * buffer.getNumSamples());
    }
    else
    {
        // Stereo input: process normally
        verb_.process (leftChannel, rightChannel, buffer.getNumSamples());
    }

    // If we have more than 2 output channels, copy the stereo output to them
    for (auto i = 2; i < totalNumOutputChannels; ++i)
    {
        buffer.copyFrom (i, 0, buffer, i % 2, 0, buffer.getNumSamples());
    }
}

//==============================================================================
bool PluginProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor (*this);
}

//==============================================================================
void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
