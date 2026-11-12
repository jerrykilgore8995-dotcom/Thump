#pragma once
#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>
#include <vector>
#include <memory>

class ThumpLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    ThumpLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
        float start, float end, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float sliderPos,
        float minPos, float maxPos, juce::Slider::SliderStyle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool over, bool down) override;
    void drawComboBox (juce::Graphics&, int width, int height, bool down,
        int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
};

// Three-position machined-gold hardware-style selector, host-automatable JUCE parameter.
class CompressorModeSwitch final : public juce::Component
{
public:
    CompressorModeSwitch (juce::AudioProcessorValueTreeState&, const juce::String& id);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
private:
    juce::RangedAudioParameter* parameter = nullptr;
    std::atomic<float>* rawValue = nullptr;
};

class ThumpAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    explicit ThumpAudioProcessorEditor (ThumpAudioProcessor&);
    ~ThumpAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    struct Position { juce::Component* component; juce::Rectangle<float> rect; };
    struct Tag { juce::String text; juce::Rectangle<float> rect; float size=13; };
    ThumpAudioProcessor& processor;
    ThumpLookAndFeel vintage;
    juce::ComboBox presetMenu;
    std::vector<std::unique_ptr<juce::ComboBox>> dropdowns;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> dropdownBindings;
    std::array<juce::ComboBox*, 2> fetRatioMenu {nullptr,nullptr};
    std::array<juce::Slider*, 2> vcaRatioSlider {nullptr,nullptr};
    std::vector<std::unique_ptr<juce::Slider>> sliders;
    std::vector<std::unique_ptr<juce::ToggleButton>> switches;
    std::vector<std::unique_ptr<CompressorModeSwitch>> modes;
    juce::Slider* lowFetAttack = nullptr;
    juce::Slider* lowFetRelease = nullptr;
    juce::Slider* highFetAttack = nullptr;
    juce::Slider* highFetRelease = nullptr;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderBindings;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> switchBindings;
    std::vector<Position> positions;
    std::vector<Tag> tags;
    void addKnob (const char* id, const char* label, float cx, float cy, float diameter=69);
    void addEQ (const char* idG, const char* idQ, const char* label, float x, float yTop=432);
    void addSwitch (const char* id, const char* text, float x, float y, float w=96);
    void addMode (const char* id, float x, float y, float width=160, float height=56);
    juce::ComboBox* addChoice (const char* id, const juce::StringArray& options,
                              float x, float y, float w, float h);
    juce::Slider* addRatioSlider (const char* id,float x,float y,float w,float h);
    juce::Slider* addHorizontal (const char* id, const char* label,
                                 float x,float y,float w,float h,float resetValue);
    void addTag (juce::String text, float x, float y, float w, float h=18, float size=13);
    void timerCallback() override;
    void drawPanel (juce::Graphics&, juce::Rectangle<float>, const juce::String&, bool gold=false);
    void drawVU (juce::Graphics&, juce::Rectangle<float>, const juce::String&, float valueDb, bool reduction=false);
    void drawScrew (juce::Graphics&, float x, float y);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ThumpAudioProcessorEditor)
};
