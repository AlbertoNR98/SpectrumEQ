/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct CustomRotarySlider : juce::Slider
{
    CustomRotarySlider() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
                                        juce::Slider::TextEntryBoxPosition::NoTextBox)
    {

    }
};

//==============================================================================
/**
*/
class SpectrumEQAudioProcessorEditor  : public juce::AudioProcessorEditor,
    juce::AudioProcessorParameter::Listener,
    juce::Timer
{
public:
    SpectrumEQAudioProcessorEditor (SpectrumEQAudioProcessor&);
    ~SpectrumEQAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    virtual void parameterValueChanged(int parameterIndex, float newValue) override;

    void parameterGestureChanged(int parameterIndex, bool gestureIsStaring) override { }

    virtual void timerCallback() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SpectrumEQAudioProcessor& audioProcessor;

    juce::Atomic<bool> parametersChanged { false };

    CustomRotarySlider peakFreqSlider, 
                       peakGainSlider, 
                       peakQualitySlider,
                       lowCutFreqSlider,
                       highCutFreqSlider, 
                       lowCutSlopeSlider,
                       highCutSlopeSlider;

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;

    Attachment peakFreqSliderAttachment,
               peakGainSliderAttachment,
               peakQualitySliderAttachment,
               lowCutFreqSliderAttachment,
               highCutFreqSliderAttachment,
               lowCutSlopeSliderAttachment,
               highCutSlopeSliderAttachment;

    std::vector<juce::Component*> getComps();

    MonoChain monoChain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumEQAudioProcessorEditor)
};
