/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SpectrumEQAudioProcessor::SpectrumEQAudioProcessor()
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

SpectrumEQAudioProcessor::~SpectrumEQAudioProcessor()
{
}

//==============================================================================
const juce::String SpectrumEQAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SpectrumEQAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SpectrumEQAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SpectrumEQAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SpectrumEQAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SpectrumEQAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SpectrumEQAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SpectrumEQAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SpectrumEQAudioProcessor::getProgramName (int index)
{
    return {};
}

void SpectrumEQAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SpectrumEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;

    leftChain.prepare(spec);
    rightChain.prepare(spec);

    updateFilters();

    leftChannelFifo.prepare(samplesPerBlock);
    rightChannelFifo.prepare(samplesPerBlock);
}

void SpectrumEQAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SpectrumEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (//layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
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

void SpectrumEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    updateFilters();

    juce::dsp::AudioBlock<float> block(buffer);
    
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    leftChain.process(leftContext);
    rightChain.process(rightContext);

    leftChannelFifo.update(buffer);
    rightChannelFifo.update(buffer);
}

//==============================================================================
bool SpectrumEQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SpectrumEQAudioProcessor::createEditor()
{
    return new SpectrumEQAudioProcessorEditor (*this);  // Custom GUI
    // return new juce::GenericAudioProcessorEditor(*this); // Generic GUI based on AudioProcessor params.
}

//==============================================================================
void SpectrumEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.

    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void SpectrumEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid())
    {
        apvts.replaceState(tree);
        updateFilters();
    }
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts) 
{
    ChainSettings settings;
      
    settings.lowCutFreq = apvts.getRawParameterValue("LowCut Freq")->load();
    settings.highCutFreq = apvts.getRawParameterValue("HighCut Freq")->load();

    settings.lowPeakFreq = apvts.getRawParameterValue("Low Peak Freq")->load();
    settings.lowPeakGainInDecibels = apvts.getRawParameterValue("Low Peak Gain")->load();
    settings.lowPeakQuality = apvts.getRawParameterValue("Low Peak Quality")->load();

    settings.lowMidPeakFreq = apvts.getRawParameterValue("LowMid Peak Freq")->load();
    settings.lowMidPeakGainInDecibels = apvts.getRawParameterValue("LowMid Peak Gain")->load();
    settings.lowMidPeakQuality = apvts.getRawParameterValue("LowMid Peak Quality")->load();

    settings.highMidPeakFreq = apvts.getRawParameterValue("HighMid Peak Freq")->load();
    settings.highMidPeakGainInDecibels = apvts.getRawParameterValue("HighMid Peak Gain")->load();
    settings.highMidPeakQuality = apvts.getRawParameterValue("HighMid Peak Quality")->load();

    settings.highPeakFreq = apvts.getRawParameterValue("High Peak Freq")->load();
    settings.highPeakGainInDecibels = apvts.getRawParameterValue("High Peak Gain")->load();
    settings.highPeakQuality = apvts.getRawParameterValue("High Peak Quality")->load();

    settings.lowCutSlope = static_cast<Slope>(apvts.getRawParameterValue("LowCut Slope")->load());
    settings.highCutSlope = static_cast<Slope>(apvts.getRawParameterValue("HighCut Slope")->load());

    settings.lowCutBypassed = apvts.getRawParameterValue("LowCut Bypassed")->load() > 0.5f; // if > 0.5, bypassed

    settings.lowPeakBypassed = apvts.getRawParameterValue("Low Peak Bypassed")->load() > 0.5f;
    settings.lowMidPeakBypassed = apvts.getRawParameterValue("LowMid Peak Bypassed")->load() > 0.5f;
    settings.highMidPeakBypassed = apvts.getRawParameterValue("HighMid Peak Bypassed")->load() > 0.5f;
    settings.highPeakBypassed = apvts.getRawParameterValue("High Peak Bypassed")->load() > 0.5f;

    settings.highCutBypassed = apvts.getRawParameterValue("HighCut Bypassed")->load() > 0.5f;

    return settings;
}

Coefficients makeLowPeakFilter(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
        chainSettings.lowPeakFreq,
        chainSettings.lowPeakQuality,
        juce::Decibels::decibelsToGain(chainSettings.lowPeakGainInDecibels));
}

Coefficients makeLowMidPeakFilter(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
        chainSettings.lowMidPeakFreq,
        chainSettings.lowMidPeakQuality,
        juce::Decibels::decibelsToGain(chainSettings.lowMidPeakGainInDecibels));
}

Coefficients makeHighMidPeakFilter(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
        chainSettings.highMidPeakFreq,
        chainSettings.highMidPeakQuality,
        juce::Decibels::decibelsToGain(chainSettings.highMidPeakGainInDecibels));
}

Coefficients makeHighPeakFilter(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
        chainSettings.highPeakFreq,
        chainSettings.highPeakQuality,
        juce::Decibels::decibelsToGain(chainSettings.highPeakGainInDecibels));
}

void SpectrumEQAudioProcessor::updateLowPeakFilter(const ChainSettings& chainSettings)
{
    auto peakCoefficients = makeLowPeakFilter(chainSettings, getSampleRate());

    leftChain.setBypassed<ChainPositions::LowPeak>(chainSettings.lowPeakBypassed);
    rightChain.setBypassed<ChainPositions::LowPeak>(chainSettings.lowPeakBypassed);

    updateCoefficients(leftChain.get<ChainPositions::LowPeak>().coefficients, peakCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::LowPeak>().coefficients, peakCoefficients);
}

void SpectrumEQAudioProcessor::updateLowMidPeakFilter(const ChainSettings& chainSettings)
{
    auto peakCoefficients = makeLowMidPeakFilter(chainSettings, getSampleRate());

    leftChain.setBypassed<ChainPositions::LowMidPeak>(chainSettings.lowMidPeakBypassed);
    rightChain.setBypassed<ChainPositions::LowMidPeak>(chainSettings.lowMidPeakBypassed);

    updateCoefficients(leftChain.get<ChainPositions::LowMidPeak>().coefficients, peakCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::LowMidPeak>().coefficients, peakCoefficients);
}

void SpectrumEQAudioProcessor::updateHighMidPeakFilter(const ChainSettings& chainSettings)
{
    auto peakCoefficients = makeHighMidPeakFilter(chainSettings, getSampleRate());

    leftChain.setBypassed<ChainPositions::HighMidPeak>(chainSettings.highMidPeakBypassed);
    rightChain.setBypassed<ChainPositions::HighMidPeak>(chainSettings.highMidPeakBypassed);

    updateCoefficients(leftChain.get<ChainPositions::HighMidPeak>().coefficients, peakCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::HighMidPeak>().coefficients, peakCoefficients);
}

void SpectrumEQAudioProcessor::updateHighPeakFilter(const ChainSettings& chainSettings)
{
    auto peakCoefficients = makeHighPeakFilter(chainSettings, getSampleRate());

    leftChain.setBypassed<ChainPositions::HighPeak>(chainSettings.highPeakBypassed);
    rightChain.setBypassed<ChainPositions::HighPeak>(chainSettings.highPeakBypassed);

    updateCoefficients(leftChain.get<ChainPositions::HighPeak>().coefficients, peakCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::HighPeak>().coefficients, peakCoefficients);
}

void updateCoefficients(Coefficients& old, const Coefficients& replacements)
{
    *old = *replacements;
}

void SpectrumEQAudioProcessor::updateLowCutFilters(const ChainSettings& chainSettings)
{
    auto cutCoefficients = makeLowCutFilter(chainSettings, getSampleRate());
    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();

    leftChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed);
    rightChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed);

    updateCutFilter(leftLowCut, cutCoefficients, chainSettings.lowCutSlope);
    updateCutFilter(rightLowCut, cutCoefficients, chainSettings.lowCutSlope);
}

void SpectrumEQAudioProcessor::updateHighCutFilters(const ChainSettings& chainSettings)
{
    auto cutCoefficients = makeHighCutFilter(chainSettings, getSampleRate());
    auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
    auto& rightHighCut = rightChain.get<ChainPositions::HighCut>();

    leftChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed);
    rightChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed);

    updateCutFilter(leftHighCut, cutCoefficients, chainSettings.highCutSlope);
    updateCutFilter(rightHighCut, cutCoefficients, chainSettings.highCutSlope);
}

void SpectrumEQAudioProcessor::updateFilters()
{
    auto chainSettings = getChainSettings(apvts);

    updateLowCutFilters(chainSettings);
    updateLowPeakFilter(chainSettings);
    updateLowMidPeakFilter(chainSettings);
    updateHighMidPeakFilter(chainSettings);
    updateHighPeakFilter(chainSettings);
    updateHighCutFilters(chainSettings);
}

juce::AudioProcessorValueTreeState::ParameterLayout SpectrumEQAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>("LowCut Freq", 
                                                           "LowCut Freq",
                                                            juce::NormalisableRange<float>(20.f, 60.f, 1.f, 0.25f),
                                                            20.f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("HighCut Freq",
                                                           "HighCut Freq",
                                                            juce::NormalisableRange<float>(8000.f, 20000.f, 1.f, 0.25f),
                                                            20000.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Low Peak Freq",
                                                           "Low Peak Freq",
                                                            juce::NormalisableRange<float>(60.f, 200.f, 1.f, 0.25f),
                                                            60.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Low Peak Gain",
                                                           "Low Peak Gain",
                                                            juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
                                                            0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Low Peak Quality",
                                                           "Low Peak Quality",
                                                            juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
                                                            1.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("LowMid Peak Freq",
                                                           "LowMid Peak Freq",
                                                           juce::NormalisableRange<float>(200.f, 600.f, 1.f, 0.25f),
                                                           200.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("LowMid Peak Gain",
                                                           "LowMid Peak Gain",
                                                           juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
                                                           0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("LowMid Peak Quality",
                                                           "LowMid Peak Quality",
                                                           juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
                                                           1.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("HighMid Peak Freq",
                                                           "HighMid Peak Freq",
                                                           juce::NormalisableRange<float>(600.f, 3000.f, 1.f, 0.25f),
                                                           600.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("HighMid Peak Gain",
                                                           "HighMid Peak Gain",
                                                           juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
                                                           0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("HighMid Peak Quality",
                                                           "HighMid Peak Quality",
                                                           juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
                                                           1.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("High Peak Freq",
                                                           "High Peak Freq",
                                                           juce::NormalisableRange<float>(3000.f, 8000.f, 1.f, 0.25f),
                                                           3000.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("High Peak Gain",
                                                           "High Peak Gain",
                                                           juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
                                                           0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("High Peak Quality",
                                                           "High Peak Quality",
                                                           juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
                                                           1.f));

    juce::StringArray stringArray;
    for (int i = 0; i < 4; ++i) 
    {
        juce::String str;
        str << (12 + i * 12);
        str << " dB/Oct";
        stringArray.add(str);
    }

    layout.add(std::make_unique<juce::AudioParameterChoice>("LowCut Slope", "LowCut Slope", stringArray, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>("HighCut Slope", "HighCut Slope", stringArray, 0));

    layout.add(std::make_unique<juce::AudioParameterBool>("LowCut Bypassed", "LowCut Bypassed", false));
    layout.add(std::make_unique<juce::AudioParameterBool>("Low Peak Bypassed", "Low Peak Bypassed", false));
    layout.add(std::make_unique<juce::AudioParameterBool>("LowMid Peak Bypassed", "LowMid Peak Bypassed", false));
    layout.add(std::make_unique<juce::AudioParameterBool>("HighMid Peak Bypassed", "HighMid Peak Bypassed", false));
    layout.add(std::make_unique<juce::AudioParameterBool>("High Peak Bypassed", "High Peak Bypassed", false));
    layout.add(std::make_unique<juce::AudioParameterBool>("HighCut Bypassed", "HighCut Bypassed", false));
    layout.add(std::make_unique<juce::AudioParameterBool>("Analyzer Enabled", "Analyzer Enabled", true));

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpectrumEQAudioProcessor();
}
