/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

void LookAndFeel::drawRotarySlider(juce::Graphics& g,
                                   int x,
                                   int y,
                                   int width,
                                   int height,
                                   float sliderPosProportional,
                                   float rotaryStartAngle,
                                   float rotaryEndAngle,
                                   juce::Slider& slider)
{
    using namespace juce;

    auto bounds = Rectangle<float>(x, y, width, height);

    auto enabled = slider.isEnabled();

    // g.setColour(Colour(97u, 18u, 167u)); // Purple
    g.setColour(enabled ? Colour(86u, 191u, 240u) : Colours::darkgrey);
    g.fillEllipse(bounds);

    //g.setColour(Colour(255u, 154u, 1u)); // Orange
    g.setColour(enabled ? Colour(94u, 86u, 240u) : Colours::grey);
    g.drawEllipse(bounds, 1.f);

    if(auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider))
    {
        auto center = bounds.getCentre();

        Path p;

        Rectangle<float> r;
        r.setLeft(center.getX() - 2);
        r.setRight(center.getX() + 2);
        r.setTop(bounds.getY());
        r.setBottom(center.getY() - rswl->getTextHeight() * 1.5);

        p.addRoundedRectangle(r, 2.f);
        p.addRectangle(r);

        jassert(rotaryStartAngle < rotaryEndAngle);

        auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);

        p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));

        g.fillPath(p);

        g.setFont(rswl->getTextHeight());
        auto text = rswl->getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);

        r.setSize(strWidth + 4, rswl->getTextHeight() + 2);
        r.setCentre(bounds.getCentre());

        g.setColour(enabled ? Colours::black : Colours::darkgrey);
        g.fillRect(r);

        g.setColour(enabled ? Colours::white : Colours::lightgrey);
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
    }
}

void LookAndFeel::drawToggleButton(juce::Graphics& g,
                                   juce::ToggleButton& toggleButton,
                                   bool shouldDrawButtonAsHighlighted,
                                   bool shouldDrawButtonAsDown)
{
    using namespace juce;

    if (auto* pb = dynamic_cast<PowerButton*>(&toggleButton))
    {
        Path powerButton;

        auto bounds = toggleButton.getLocalBounds();

        /*
        // Draw for debugging
        g.setColour(Colours::red);
        g.drawRect(bounds);
        */

        auto size = jmin(bounds.getWidth(), bounds.getHeight()) - 6;
        auto r = bounds.withSizeKeepingCentre(size, size).toFloat();

        float ang = 30.f;

        size -= 6;

        powerButton.addCentredArc(r.getCentreX(),
                                  r.getCentreY(),
                                  size * 0.5,
                                  size * 0.5,
                                  0.f,
                                  degreesToRadians(ang),
                                  degreesToRadians(360.f - ang),
                                  true);

        powerButton.startNewSubPath(r.getCentreX(), r.getY());
        powerButton.lineTo(r.getCentre());

        PathStrokeType pst(2.f, PathStrokeType::JointStyle::curved);

        auto color = toggleButton.getToggleState() ? Colours::dimgrey : Colour(0u, 172u, 1u);

        g.setColour(color);
        g.strokePath(powerButton, pst);
        g.drawEllipse(r, 2.f);
    }
    else if (auto* analyzerButton = dynamic_cast<AnalyzerButton*>(&toggleButton))
    {
        auto color = !toggleButton.getToggleState() ? Colours::dimgrey : Colour(0u, 172u, 1u);
        g.setColour(color);

        auto bounds = toggleButton.getLocalBounds();
        g.drawRect(bounds);

        g.strokePath(analyzerButton->randomPath, PathStrokeType(1.f));
    }
}

//==============================================================================
void RotarySliderWithLabels::paint(juce::Graphics& g)
{
    using namespace juce;

    auto startAng = degreesToRadians(180.f + 45.f);
    auto endAng = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;

    auto range = getRange();

    auto sliderBounds = getSliderBounds();

    /*
    // Draw bounds for debugging
    g.setColour(Colours::red);
    g.drawRect(getLocalBounds());
    g.setColour(Colours::yellow);
    g.drawRect(sliderBounds);
    */

    getLookAndFeel().drawRotarySlider(g, 
                                      sliderBounds.getX(),
                                      sliderBounds.getY(),
                                      sliderBounds.getWidth(),
                                      sliderBounds.getHeight(),
                                      jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),
                                      startAng,
                                      endAng, 
                                      *this);

    auto center = sliderBounds.toFloat().getCentre();
    auto radius = sliderBounds.getWidth() * 0.5f;

    // g.setColour(Colour(0u, 172u, 1u));  // Green
    g.setColour(Colour(255u, 255u, 255u));
    g.setFont(getTextHeight());

    auto numChoices = labels.size();
    for (int i = 0; i < numChoices; ++i)
    {
        auto pos = labels[i].pos;
        jassert(0.f <= pos);
        jassert(pos <= 1.f);

        auto ang = jmap(pos, 0.f, 1.f, startAng, endAng);

        auto c = center.getPointOnCircumference(radius + getTextHeight() * 0.5f + 1, ang);

        Rectangle<float> r;
        auto str = labels[i].label;
        r.setSize(g.getCurrentFont().getStringWidth(str), getTextHeight());
        r.setCentre(c);
        r.setY(r.getY() + getTextHeight());

        g.drawFittedText(str, r.toNearestInt(), juce::Justification::centred, 1);
    }
}

juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const
{
    auto bounds = getLocalBounds();

    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());

    size -= getTextHeight() * 2;
    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), 0);
    r.setY(2);

    return r;
}

juce::String RotarySliderWithLabels::getDisplayString() const
{
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
        return choiceParam->getCurrentChoiceName();

    juce::String str;
    bool addK = false;

    if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
    {
        float val = getValue();

        if (val > 999.f)    // 999 Hz
        {
            val /= 1000.f;
            addK = true; // Represented as kHz
        }

        str = juce::String(val, (addK ? 2 : 0));
    }
    else
    {
        jassertfalse;
    }

    if (suffix.isNotEmpty())
    {
        str << " ";
        if (addK)
            str << "k";
        
        str << suffix;
    }

    return str;
}

//==============================================================================
ResponseCurveComponent::ResponseCurveComponent(SpectrumEQAudioProcessor& p) :
    audioProcessor(p),
    leftPathProducer(audioProcessor.leftChannelFifo),
    rightPathProducer(audioProcessor.rightChannelFifo)
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->addListener(this);
    }

    updateChain();

    startTimerHz(60);
}

ResponseCurveComponent::~ResponseCurveComponent()
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->removeListener(this);
    }
}

void ResponseCurveComponent::updateResponseCurve()
{
    using namespace juce;

    auto responseArea = getAnalysisArea(); // getRenderArea();
    auto w = responseArea.getWidth();

    auto& lowcut = monoChain.get<ChainPositions::LowCut>();
    auto& lowPeak = monoChain.get<ChainPositions::LowPeak>();
    auto& lowMidPeak = monoChain.get<ChainPositions::LowMidPeak>();
    auto& highMidPeak = monoChain.get<ChainPositions::HighMidPeak>();
    auto& highPeak = monoChain.get<ChainPositions::HighPeak>();
    auto& highcut = monoChain.get<ChainPositions::HighCut>();

    auto sampleRate = audioProcessor.getSampleRate();

    std::vector<double> mags;

    mags.resize(w);

    for (int i = 0; i < w; ++i)
    {
        double mag = 1.f;
        auto freq = mapToLog10(double(i) / double(w), 20.0, 20000.0);

        if (!monoChain.isBypassed<ChainPositions::LowPeak>())
            mag *= lowPeak.coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if (!monoChain.isBypassed<ChainPositions::LowMidPeak>())
            mag *= lowMidPeak.coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if (!monoChain.isBypassed<ChainPositions::HighMidPeak>())
            mag *= highMidPeak.coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if (!monoChain.isBypassed<ChainPositions::HighPeak>())
            mag *= highPeak.coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if (!monoChain.isBypassed<ChainPositions::LowCut>())
        {
            if (!lowcut.isBypassed<0>())
                mag *= lowcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (!lowcut.isBypassed<1>())
                mag *= lowcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (!lowcut.isBypassed<2>())
                mag *= lowcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (!lowcut.isBypassed<3>())
                mag *= lowcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }

        if (!monoChain.isBypassed<ChainPositions::HighCut>())
        {
            if (!highcut.isBypassed<0>())
                mag *= highcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (!highcut.isBypassed<1>())
                mag *= highcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (!highcut.isBypassed<2>())
                mag *= highcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (!highcut.isBypassed<3>())
                mag *= highcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }

        mags[i] = Decibels::gainToDecibels(mag);
    }

    responseCurve.clear();

    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();

    auto map = [outputMin, outputMax](double input)
    {
        return jmap(input, -24.0, 24.0, outputMin, outputMax);
    };

    responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));

    for (size_t i = 1; i < mags.size(); ++i)
    {
        responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
    }
}

void ResponseCurveComponent::paint(juce::Graphics& g)
{
    using namespace juce;

    g.fillAll(Colours::black);

    drawBackgroundGrid(g);

    auto responseArea = getAnalysisArea();

    if (shouldShowFFTAnalysis)
    {
        auto leftChannelFFTPath = leftPathProducer.getPath();
        leftChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));
        g.setColour(Colour(97u, 18u, 167u));
        g.strokePath(leftChannelFFTPath, PathStrokeType(1.f));

        auto rightChannelFFTPath = rightPathProducer.getPath();
        rightChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));
        g.setColour(Colour(215u, 201u, 134u));
        g.strokePath(rightChannelFFTPath, PathStrokeType(1.f));
    }

    g.setColour(Colours::white);
    g.strokePath(responseCurve, PathStrokeType(2.f));

    Path border;
    
    border.setUsingNonZeroWinding(false);
    border.addRoundedRectangle(getRenderArea(), 4);
    border.addRectangle(getLocalBounds());

    g.setColour(Colours::black);
    g.fillPath(border);

    drawTextLabels(g);

    g.setColour(Colours::orange);
    g.drawRoundedRectangle(getRenderArea().toFloat(), 4.f, 1.f);
}

std::vector<float> ResponseCurveComponent::getFrequencies()
{
    return std::vector<float>
    {
        20, /*30, 40,*/ 50, 100,
        200, /*300, 400,*/ 500, 1000,
        2000, /*3000, 4000,*/ 5000, 10000,
        20000
    };
}

std::vector<float> ResponseCurveComponent::getGains()
{
    return std::vector<float>
    {
        -24, -12, 0, 12, 24
    };
}

std::vector<float> ResponseCurveComponent::getXs(const std::vector<float>& freqs, float left, float width)
{
    std::vector<float> xs;
    for (auto f : freqs)
    {
        auto normX = juce::mapFromLog10(f, 20.f, 20000.f);
        xs.push_back(left + width * normX);
    }

    return xs;
}

void ResponseCurveComponent::drawBackgroundGrid(juce::Graphics& g)
{
    using namespace juce;
    auto freqs = getFrequencies();

    auto renderArea = getAnalysisArea();
    auto left = renderArea.getX();
    auto right = renderArea.getRight();
    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width = renderArea.getWidth();

    auto xs = getXs(freqs, left, width);

    g.setColour(Colours::dimgrey);
    for (auto x : xs)
    {
        g.drawVerticalLine(x, top, bottom);
    }

    auto gain = getGains();

    for (auto gDb : gain)
    {
        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));
        g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u) : Colours::darkgrey); // Green line for 0 dB
        g.drawHorizontalLine(y, left, right);
    }
}

void ResponseCurveComponent::drawTextLabels(juce::Graphics& g)
{
    using namespace juce;
    g.setColour(Colours::lightgrey);
    const int fontHeight = 10;
    g.setFont(fontHeight);

    auto renderArea = getAnalysisArea();
    auto left = renderArea.getX();

    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width = renderArea.getWidth();

    auto freqs = getFrequencies();
    auto xs = getXs(freqs, left, width);

    for (int i = 0; i < freqs.size(); ++i)
    {
        auto f = freqs[i];
        auto x = xs[i];

        bool addK = false;
        String str;
        if (f > 999.f)
        {
            addK = true;
            f /= 1000.f;
        }

        str << f;
        if (addK)
            str << "k";
        str << "Hz";

        auto textWidth = g.getCurrentFont().getStringWidth(str);

        Rectangle<int> r;

        r.setSize(textWidth, fontHeight);
        r.setCentre(x, 0);
        r.setY(1);

        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }

    auto gain = getGains();

    for (auto gDb : gain)
    {
        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));

        String str;
        if (gDb > 0)
            str << "+";
        str << gDb;

        auto textWidth = g.getCurrentFont().getStringWidth(str);
        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setX(getWidth() - textWidth);
        r.setCentre(r.getCentreX(), y);
        
        g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u) : Colours::lightgrey);
        g.drawFittedText(str, r, juce::Justification::centredLeft, 1);

        str.clear();
        str << (gDb - 24.f);
        
        r.setX(1);
        textWidth = g.getCurrentFont().getStringWidth(str);
        r.setSize(textWidth, fontHeight);
        g.setColour(Colours::lightgrey);
        g.drawFittedText(str, r, juce::Justification::centredLeft, 1);
    }
}

void ResponseCurveComponent::resized()
{
    using namespace juce; 

    responseCurve.preallocateSpace(getWidth() * 3);
    updateResponseCurve();
}

void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
}

void ResponseCurveComponent::timerCallback()
{
    if (shouldShowFFTAnalysis)
    {
        auto fftBounds = getAnalysisArea().toFloat();
        auto sampleRate = audioProcessor.getSampleRate();

        leftPathProducer.process(fftBounds, sampleRate);
        rightPathProducer.process(fftBounds, sampleRate);
    }

    if (parametersChanged.compareAndSetBool(false, true))
    {
        updateChain();
        updateResponseCurve();
    }

    repaint();
}

void ResponseCurveComponent::updateChain()
{
    auto chainSettings = getChainSettings(audioProcessor.apvts);

    monoChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed);
    // monoChain.setBypassed<ChainPositions::Peak>(chainSettings.peakBypassed);
    monoChain.setBypassed<ChainPositions::LowPeak>(chainSettings.lowPeakBypassed);
    monoChain.setBypassed<ChainPositions::LowMidPeak>(chainSettings.lowMidPeakBypassed);
    monoChain.setBypassed<ChainPositions::HighMidPeak>(chainSettings.highMidPeakBypassed);
    monoChain.setBypassed<ChainPositions::HighPeak>(chainSettings.highPeakBypassed);
    monoChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed);

    /*
    auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
    updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    */

    auto lowPeakCoefficients = makeLowPeakFilter(chainSettings, audioProcessor.getSampleRate());
    updateCoefficients(monoChain.get<ChainPositions::LowPeak>().coefficients, lowPeakCoefficients);

    auto lowMidPeakCoefficients = makeLowMidPeakFilter(chainSettings, audioProcessor.getSampleRate());
    updateCoefficients(monoChain.get<ChainPositions::LowMidPeak>().coefficients, lowMidPeakCoefficients);

    auto highMidPeakCoefficients = makeHighMidPeakFilter(chainSettings, audioProcessor.getSampleRate());
    updateCoefficients(monoChain.get<ChainPositions::HighMidPeak>().coefficients, highMidPeakCoefficients);

    auto highPeakCoefficients = makeHighPeakFilter(chainSettings, audioProcessor.getSampleRate());
    updateCoefficients(monoChain.get<ChainPositions::HighPeak>().coefficients, highPeakCoefficients);

    auto lowCutCoefficients = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
    auto highCutCoefficients = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());

    updateCutFilter(monoChain.get<ChainPositions::LowCut>(),
                    lowCutCoefficients,
                    chainSettings.lowCutSlope);

    updateCutFilter(monoChain.get<ChainPositions::HighCut>(),
                    highCutCoefficients,
                    chainSettings.highCutSlope);
}

juce::Rectangle<int> ResponseCurveComponent::getRenderArea()
{
    auto bounds = getLocalBounds();

    bounds.removeFromTop(12);
    bounds.removeFromBottom(2);
    bounds.removeFromLeft(20);
    bounds.removeFromRight(20);

    return bounds;
}

juce::Rectangle<int> ResponseCurveComponent::getAnalysisArea()
{
    auto bounds = getRenderArea();
    bounds.removeFromTop(4);
    bounds.removeFromBottom(4);
    return bounds;
}

//==============================================================================
void PathProducer::process(juce::Rectangle<float> fftBounds, double sampleRate)
{
    juce::AudioBuffer<float> tempIncomingBuffer;
    while (leftChannelFifo->getNumCompleteBuffersAvailable() > 0)
    {
        if (leftChannelFifo->getAudioBuffer(tempIncomingBuffer))
        {
            auto size = tempIncomingBuffer.getNumSamples();

            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, 0),
                monoBuffer.getReadPointer(0, size),
                monoBuffer.getNumSamples() - size);

            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size),
                tempIncomingBuffer.getReadPointer(0, 0),
                size);

            leftChannelFFTDataGenerator.produceFFTDataForRendering(monoBuffer, -48.f);
        }
    }

    const auto fftSize = leftChannelFFTDataGenerator.getFFTSize();
    const auto binWidth = sampleRate / double(fftSize);

    while (leftChannelFFTDataGenerator.getNumAvailableFFTDataBlocks() > 0)
    {
        std::vector<float> fftData;
        if (leftChannelFFTDataGenerator.getFFTData(fftData))
        {
            pathProducer.generatePath(fftData, fftBounds, fftSize, binWidth, -48.f);
        }
    }

    while (pathProducer.getNumPathsAvailable() > 0)
    {
        pathProducer.getPath(leftChannelFFTPath);
    }
}

//==============================================================================
SpectrumEQAudioProcessorEditor::SpectrumEQAudioProcessorEditor (SpectrumEQAudioProcessor& p) : 
      AudioProcessorEditor (&p), audioProcessor (p), 

      lowPeakFreqSlider(*audioProcessor.apvts.getParameter("Low Peak Freq"), "Hz"),
      lowPeakGainSlider(*audioProcessor.apvts.getParameter("Low Peak Gain"), "dB"),
      lowPeakQualitySlider(*audioProcessor.apvts.getParameter("Low Peak Quality"), ""),
      lowMidPeakFreqSlider(*audioProcessor.apvts.getParameter("LowMid Peak Freq"), "Hz"),
      lowMidPeakGainSlider(*audioProcessor.apvts.getParameter("LowMid Peak Gain"), "dB"),
      lowMidPeakQualitySlider(*audioProcessor.apvts.getParameter("LowMid Peak Quality"), ""),
      highMidPeakFreqSlider(*audioProcessor.apvts.getParameter("HighMid Peak Freq"), "Hz"),
      highMidPeakGainSlider(*audioProcessor.apvts.getParameter("HighMid Peak Gain"), "dB"),
      highMidPeakQualitySlider(*audioProcessor.apvts.getParameter("HighMid Peak Quality"), ""),
      highPeakFreqSlider(*audioProcessor.apvts.getParameter("High Peak Freq"), "Hz"),
      highPeakGainSlider(*audioProcessor.apvts.getParameter("High Peak Gain"), "dB"),
      highPeakQualitySlider(*audioProcessor.apvts.getParameter("High Peak Quality"), ""),

      lowCutFreqSlider(*audioProcessor.apvts.getParameter("LowCut Freq"), "Hz"),
      highCutFreqSlider(*audioProcessor.apvts.getParameter("HighCut Freq"), "Hz"),
      lowCutSlopeSlider(*audioProcessor.apvts.getParameter("LowCut Slope"), "dB/Oct"),
      highCutSlopeSlider(*audioProcessor.apvts.getParameter("HighCut Slope"), "dB/Oct"),
      
      responseCurveComponent(audioProcessor),
      lowPeakFreqSliderAttachment(audioProcessor.apvts, "Low Peak Freq", lowPeakFreqSlider),
      lowPeakGainSliderAttachment(audioProcessor.apvts, "Low Peak Gain", lowPeakGainSlider),
      lowPeakQualitySliderAttachment(audioProcessor.apvts, "Low Peak Quality", lowPeakQualitySlider),
      lowMidPeakFreqSliderAttachment(audioProcessor.apvts, "LowMid Peak Freq", lowMidPeakFreqSlider),
      lowMidPeakGainSliderAttachment(audioProcessor.apvts, "LowMid Peak Gain", lowMidPeakGainSlider),
      lowMidPeakQualitySliderAttachment(audioProcessor.apvts, "LowMid Peak Quality", lowMidPeakQualitySlider),
      highMidPeakFreqSliderAttachment(audioProcessor.apvts, "HighMid Peak Freq", highMidPeakFreqSlider),
      highMidPeakGainSliderAttachment(audioProcessor.apvts, "HighMid Peak Gain", highMidPeakGainSlider),
      highMidPeakQualitySliderAttachment(audioProcessor.apvts, "HighMid Peak Quality", highMidPeakQualitySlider),
      highPeakFreqSliderAttachment(audioProcessor.apvts, "High Peak Freq", highPeakFreqSlider),
      highPeakGainSliderAttachment(audioProcessor.apvts, "High Peak Gain", highPeakGainSlider),
      highPeakQualitySliderAttachment(audioProcessor.apvts, "High Peak Quality", highPeakQualitySlider),

      lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCut Freq", lowCutFreqSlider),
      highCutFreqSliderAttachment(audioProcessor.apvts, "HighCut Freq", highCutFreqSlider),
      lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCut Slope", lowCutSlopeSlider),
      highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCut Slope", highCutSlopeSlider),

      lowcutBypassButtonAttachment(audioProcessor.apvts, "LowCut Bypassed", lowcutBypassButton),
      lowPeakBypassButtonAttachment(audioProcessor.apvts, "Low Peak Bypassed", lowPeakBypassButton),
      lowMidPeakBypassButtonAttachment(audioProcessor.apvts, "LowMid Peak Bypassed", lowMidPeakBypassButton),
      highMidPeakBypassButtonAttachment(audioProcessor.apvts, "HighMid Peak Bypassed", highMidPeakBypassButton),
      highPeakBypassButtonAttachment(audioProcessor.apvts, "High Peak Bypassed", highPeakBypassButton),

      highcutBypassButtonAttachment(audioProcessor.apvts, "HighCut Bypassed", highcutBypassButton),
      analyzerEnabledButtonAttachment(audioProcessor.apvts, "Analyzer Enabled", analyzerEnabledButton)
{
    lowPeakFreqSlider.labels.add({ 0.f, "60 Hz" });
    lowPeakFreqSlider.labels.add({ 1.f, "200 Hz" });

    lowPeakGainSlider.labels.add({ 0.f, "-24 dB" });
    lowPeakGainSlider.labels.add({ 1.f, "+24 dB" });

    lowPeakQualitySlider.labels.add({ 0.f, "0.1" });
    lowPeakQualitySlider.labels.add({ 1.f, "10.0" });

    lowMidPeakFreqSlider.labels.add({ 0.f, "200 Hz" });
    lowMidPeakFreqSlider.labels.add({ 1.f, "600 Hz" });

    lowMidPeakGainSlider.labels.add({ 0.f, "-24 dB" });
    lowMidPeakGainSlider.labels.add({ 1.f, "+24 dB" });

    lowMidPeakQualitySlider.labels.add({ 0.f, "0.1" });
    lowMidPeakQualitySlider.labels.add({ 1.f, "10.0" });

    highMidPeakFreqSlider.labels.add({ 0.f, "600 Hz" });
    highMidPeakFreqSlider.labels.add({ 1.f, "3 kHz" });

    highMidPeakGainSlider.labels.add({ 0.f, "-24 dB" });
    highMidPeakGainSlider.labels.add({ 1.f, "+24 dB" });

    highMidPeakQualitySlider.labels.add({ 0.f, "0.1" });
    highMidPeakQualitySlider.labels.add({ 1.f, "10.0" });

    highPeakFreqSlider.labels.add({ 0.f, "3 kHz" });
    highPeakFreqSlider.labels.add({ 1.f, "8 kHz" });

    highPeakGainSlider.labels.add({ 0.f, "-24 dB" });
    highPeakGainSlider.labels.add({ 1.f, "+24 dB" });

    highPeakQualitySlider.labels.add({ 0.f, "0.1" });
    highPeakQualitySlider.labels.add({ 1.f, "10.0" });

    lowCutFreqSlider.labels.add({ 0.f, "20 Hz" });
    lowCutFreqSlider.labels.add({ 1.f, "60 Hz" });

    highCutFreqSlider.labels.add({ 0.f, "8 kHz" });
    highCutFreqSlider.labels.add({ 1.f, "20 kHz" });

    lowCutSlopeSlider.labels.add({ 0.0f, "12" });
    lowCutSlopeSlider.labels.add({ 1.f, "48" });

    highCutSlopeSlider.labels.add({ 0.0f, "12" });
    highCutSlopeSlider.labels.add({ 1.f, "48" });

    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }

    lowPeakBypassButton.setLookAndFeel(&lnf);
    lowMidPeakBypassButton.setLookAndFeel(&lnf);
    highMidPeakBypassButton.setLookAndFeel(&lnf);
    highPeakBypassButton.setLookAndFeel(&lnf);
    lowcutBypassButton.setLookAndFeel(&lnf);
    highcutBypassButton.setLookAndFeel(&lnf);
    analyzerEnabledButton.setLookAndFeel(&lnf);

    auto safePtr = juce::Component::SafePointer<SpectrumEQAudioProcessorEditor>(this);
    lowPeakBypassButton.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
        {
            auto bypassed = comp->lowPeakBypassButton.getToggleState();

            comp->lowPeakFreqSlider.setEnabled(!bypassed);
            comp->lowPeakGainSlider.setEnabled(!bypassed);
            comp->lowPeakQualitySlider.setEnabled(!bypassed);
        }
    };

    lowMidPeakBypassButton.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
        {
            auto bypassed = comp->lowMidPeakBypassButton.getToggleState();

            comp->lowMidPeakFreqSlider.setEnabled(!bypassed);
            comp->lowMidPeakGainSlider.setEnabled(!bypassed);
            comp->lowMidPeakQualitySlider.setEnabled(!bypassed);
        }
    };

    highMidPeakBypassButton.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
        {
            auto bypassed = comp->highMidPeakBypassButton.getToggleState();

            comp->highMidPeakFreqSlider.setEnabled(!bypassed);
            comp->highMidPeakGainSlider.setEnabled(!bypassed);
            comp->highMidPeakQualitySlider.setEnabled(!bypassed);
        }
    };

    highPeakBypassButton.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
        {
            auto bypassed = comp->highPeakBypassButton.getToggleState();

            comp->highPeakFreqSlider.setEnabled(!bypassed);
            comp->highPeakGainSlider.setEnabled(!bypassed);
            comp->highPeakQualitySlider.setEnabled(!bypassed);
        }
    };

    lowcutBypassButton.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
        {
            auto bypassed = comp->lowcutBypassButton.getToggleState();

            comp->lowCutFreqSlider.setEnabled(!bypassed);
            comp->lowCutSlopeSlider.setEnabled(!bypassed);
        }
    };

    highcutBypassButton.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
        {
            auto bypassed = comp->highcutBypassButton.getToggleState();

            comp->highCutFreqSlider.setEnabled(!bypassed);
            comp->highCutSlopeSlider.setEnabled(!bypassed);
        }
    };

    analyzerEnabledButton.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
        {
            auto enabled = comp->analyzerEnabledButton.getToggleState();
            comp->responseCurveComponent.toggleAnalysisEnablement(enabled);
        }
    };

   // setSize(480, 500);
    setSize(800, 600);
}

SpectrumEQAudioProcessorEditor::~SpectrumEQAudioProcessorEditor() {
    lowPeakBypassButton.setLookAndFeel(nullptr);
    lowMidPeakBypassButton.setLookAndFeel(nullptr);
    highMidPeakBypassButton.setLookAndFeel(nullptr);
    highPeakBypassButton.setLookAndFeel(nullptr);
    lowcutBypassButton.setLookAndFeel(nullptr);
    highcutBypassButton.setLookAndFeel(nullptr);
    analyzerEnabledButton.setLookAndFeel(nullptr);
}

//==============================================================================
void SpectrumEQAudioProcessorEditor::paint(juce::Graphics& g)
{
    using namespace juce;

    g.fillAll(juce::Colours::black);

    g.setColour(Colours::grey);
    g.setFont(14);
    g.drawFittedText("LowCut", lowCutSlopeSlider.getBounds(), juce::Justification::centredBottom, 1);
    g.drawFittedText("Low Peak", lowPeakQualitySlider.getBounds(), juce::Justification::centredBottom, 1);
    g.drawFittedText("LowMid Peak", lowMidPeakQualitySlider.getBounds(), juce::Justification::centredBottom, 1);
    g.drawFittedText("HighMid Peak", highMidPeakQualitySlider.getBounds(), juce::Justification::centredBottom, 1);
    g.drawFittedText("High Peak", highPeakQualitySlider.getBounds(), juce::Justification::centredBottom, 1);
    g.drawFittedText("HighCut", highCutSlopeSlider.getBounds(), juce::Justification::centredBottom, 1);
}

void SpectrumEQAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(4);

    auto analyzerEnabledArea = bounds.removeFromTop(25);
    analyzerEnabledArea.setWidth(100);
    analyzerEnabledArea.setX(5);
    analyzerEnabledArea.removeFromTop(2);

    analyzerEnabledButton.setBounds(analyzerEnabledArea);

    bounds.removeFromTop(5);

    float hRatio = 27.f / 100.f; // JUCE_LIVE_CONSTANT(33) / 100.f;
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * hRatio);

    responseCurveComponent.setBounds(responseArea);

    bounds.removeFromTop(5);

    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.166);
    auto lowPeakArea = bounds.removeFromLeft(bounds.getWidth() * 0.2);
    auto lowMidPeakArea = bounds.removeFromLeft(bounds.getWidth() * 0.25);
    auto highMidPeakArea = bounds.removeFromLeft(bounds.getWidth() * 0.333);
    auto highPeakArea = bounds.removeFromLeft(bounds.getWidth() * 0.5);
    auto highCutArea = bounds;


    lowcutBypassButton.setBounds(lowCutArea.removeFromTop(25));
    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5));
    lowCutSlopeSlider.setBounds(lowCutArea);

    lowPeakBypassButton.setBounds(lowPeakArea.removeFromTop(25));
    lowPeakFreqSlider.setBounds(lowPeakArea.removeFromTop(lowPeakArea.getHeight() * 0.33));
    lowPeakGainSlider.setBounds(lowPeakArea.removeFromTop(lowPeakArea.getHeight() * 0.5));
    lowPeakQualitySlider.setBounds(lowPeakArea);

    lowMidPeakBypassButton.setBounds(lowMidPeakArea.removeFromTop(25));
    lowMidPeakFreqSlider.setBounds(lowMidPeakArea.removeFromTop(lowMidPeakArea.getHeight() * 0.33));
    lowMidPeakGainSlider.setBounds(lowMidPeakArea.removeFromTop(lowMidPeakArea.getHeight() * 0.5));
    lowMidPeakQualitySlider.setBounds(lowMidPeakArea);

    highMidPeakBypassButton.setBounds(highMidPeakArea.removeFromTop(25));
    highMidPeakFreqSlider.setBounds(highMidPeakArea.removeFromTop(highMidPeakArea.getHeight() * 0.33));
    highMidPeakGainSlider.setBounds(highMidPeakArea.removeFromTop(highMidPeakArea.getHeight() * 0.5));
    highMidPeakQualitySlider.setBounds(highMidPeakArea);

    highPeakBypassButton.setBounds(highPeakArea.removeFromTop(25));
    highPeakFreqSlider.setBounds(highPeakArea.removeFromTop(highPeakArea.getHeight() * 0.33));
    highPeakGainSlider.setBounds(highPeakArea.removeFromTop(highPeakArea.getHeight() * 0.5));
    highPeakQualitySlider.setBounds(highPeakArea);

    highcutBypassButton.setBounds(highCutArea.removeFromTop(25));
    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
    highCutSlopeSlider.setBounds(highCutArea);
}

std::vector<juce::Component*> SpectrumEQAudioProcessorEditor::getComps()
{
    return
    {
        &lowPeakFreqSlider,
        &lowPeakGainSlider,
        &lowPeakQualitySlider,
        &lowMidPeakFreqSlider,
        &lowMidPeakGainSlider,
        &lowMidPeakQualitySlider,
        &highMidPeakFreqSlider,
        &highMidPeakGainSlider,
        &highMidPeakQualitySlider,
        &highPeakFreqSlider,
        &highPeakGainSlider,
        &highPeakQualitySlider,
        &lowCutFreqSlider,
        &highCutFreqSlider,
        &lowCutSlopeSlider,
        &highCutSlopeSlider,
        &responseCurveComponent,

        &lowcutBypassButton,
        &lowPeakBypassButton,
        &lowMidPeakBypassButton,
        &highMidPeakBypassButton,
        &highPeakBypassButton,
        &highcutBypassButton,
        &analyzerEnabledButton
    };
}