#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (p),
        processorRef (p)
{

    if (CMAKE_BUILD_TYPE == "Debug")
    {
        addAndMakeVisible (inspectButton);

        // this chunk of code instantiates and opens the melatonin inspector
        inspectButton.onClick = [&] {
            if (!inspector)
            {
                inspector = std::make_unique<melatonin::Inspector> (*this);
                inspector->onClose = [this]() { inspector.reset(); };
            }

            inspector->setVisible (true);
        };
    }

    // Wet Gain Slider
    wetGainSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    dryGainSlider.setRange (0.0f, 1.0f, 0.01f);
    wetGainSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 100, 20);
    addAndMakeVisible(wetGainSlider);
    wetGainLabel.setText("Wet", juce::dontSendNotification);
    wetGainLabel.attachToComponent(&wetGainSlider, true);
    addAndMakeVisible(wetGainLabel);
    wetGainAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (p.parameters, "wet_gain", wetGainSlider));
    
    // Dry Gain Slider
    dryGainSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    dryGainSlider.setRange(0.0f, 1.0f, 0.01f);
    dryGainSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 100, 20);
    addAndMakeVisible(dryGainSlider);
    dryGainLabel.setText("Dry", juce::dontSendNotification);
    dryGainLabel.attachToComponent(&dryGainSlider, true);
    addAndMakeVisible(dryGainLabel);
    dryGainAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (p.parameters, "dry_gain", dryGainSlider));

    // Preset Selector
    presetSelector.addItemList({ "Room", "Studio Small", "Studio Medium", "Studio Large", "Hall",
                                "Half Echo", "Space Echo", "Chaos Echo", "Delay", "Off" }, 1);
    addAndMakeVisible(presetSelector);
    presetLabel.setText("Preset", juce::dontSendNotification);
    presetLabel.attachToComponent(&presetSelector, true);
    addAndMakeVisible(presetLabel);
    presetAttachment.reset (new juce::AudioProcessorValueTreeState::ComboBoxAttachment (p.parameters, "preset", presetSelector));

    // Crush Selector
    crushSelector.addItemList ({ "Hi Def", "OG", "Crushed", "Scrunted" }, 1);
    addAndMakeVisible (crushSelector);
    crushLabel.setText ("Crush", juce::dontSendNotification);
    crushLabel.attachToComponent (&crushSelector, true);
    addAndMakeVisible (crushLabel);
    crushAttachment.reset (new juce::AudioProcessorValueTreeState::ComboBoxAttachment (p.parameters, "crush", crushSelector));

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);
}

PluginEditor::~PluginEditor()
{
}

void PluginEditor::paint (juce::Graphics& g)
{
    auto area = getLocalBounds();
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (16.0f);

    auto helloWorld = juce::String (PRODUCT_NAME_WITHOUT_VERSION) + " v" VERSION + " running in " + CMAKE_BUILD_TYPE;
    g.drawText (helloWorld, area.removeFromTop (150), juce::Justification::centred, false);
}

void PluginEditor::resized()
{
    auto area = getLocalBounds();
    
    // Title area
    area.removeFromTop(40);

    // Wet Gain Slider
    wetGainLabel.setBounds(area.removeFromLeft(40));
    wetGainSlider.setBounds(area.removeFromTop(30));

    area.removeFromTop(10); // spacing

    // Dry Gain Slider
    dryGainLabel.setBounds(area.removeFromLeft(40));
    dryGainSlider.setBounds(area.removeFromTop(30));

    area.removeFromTop(10); // spacing

    // Preset Selector
    presetLabel.setBounds(area.removeFromLeft(40));
    presetSelector.setBounds(area.removeFromTop(30).withWidth(200));

    area.removeFromTop(10); // spacing

    // Crush Button
    crushLabel.setBounds (area.removeFromLeft (40));
    crushSelector.setBounds (area.removeFromTop (30).withWidth (200));

    area.removeFromTop (10); // spacing

    // Inspect Button
    inspectButton.setBounds(area.removeFromTop(50).withSizeKeepingCentre(100, 50));
}