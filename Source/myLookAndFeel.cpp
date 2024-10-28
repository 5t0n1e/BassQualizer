#include "myLookAndFeel.h"

//==============================================================================
myLookAndFeelV1::myLookAndFeelV1() {
    juce::File knobImageFile1 = juce::File::getSpecialLocation
            (juce::File::SpecialLocationType::userDesktopDirectory).getChildFile("knob1.png");
    img1 = juce::ImageCache::getFromFile(knobImageFile1);
}

//==============================================================================
void myLookAndFeelV1::drawRotarySlider(juce::Graphics &g,
                                       int x, int y, int width, int height, float sliderPos,
                                       float rotaryStartAngle, float rotaryEndAngle, juce::Slider &slider) {
    if (img1.isValid()) {
        const double rotation = (slider.getValue()
                                 - slider.getMinimum())
                                / (slider.getMaximum()
                                   - slider.getMinimum());

        const int frames = img1.getHeight() / img1.getWidth();
        const int frameId = (int) ceil(rotation * ((double) frames - 1.0));
        const float radius = juce::jmin(width / 2.0f, height / 2.0f);
        const float centerX = x + width * 0.5f;
        const float centerY = y + height * 0.5f;
        const float rx = centerX - radius - 1.0f;
        const float ry = centerY - radius;

        g.drawImage(img1,
                    (int) rx,
                    (int) ry,
                    2 * (int) radius,
                    2 * (int) radius,
                    0,
                    frameId * img1.getWidth(),
                    img1.getWidth(),
                    img1.getWidth());
    } else {
        static const float textPpercent = 0.35f;
        juce::Rectangle<float> text_bounds(1.0f + width * (1.0f - textPpercent) / 2.0f,
                                           0.5f * height, width * textPpercent, 0.5f * height);

        g.setColour(juce::Colours::white);

        g.drawFittedText(juce::String("No Image"), text_bounds.getSmallestIntegerContainer(),
                         juce::Justification::horizontallyCentred | juce::Justification::centred, 1);
    }
}

void myLookAndFeelV1::drawToggleButton(juce::Graphics &g, juce::ToggleButton &toggleButton,
                                       bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) {
    using namespace juce;
    Path powerButton;

    auto bounds = toggleButton.getLocalBounds();
    auto size = jmin(bounds.getWidth(), bounds.getHeight()) - 6;
    auto r = bounds.withSizeKeepingCentre(size, size);

    float ang = 30.0f;

    powerButton.addCentredArc(r.getCentreX(), r.getCentreY(), r.getWidth() * 0.5f, r.getHeight() * 0.5f, 0.0f,
                              degreesToRadians(ang), degreesToRadians(360.0f - ang), true);
    powerButton.startNewSubPath(r.getCentreX(), r.getY());
    powerButton.lineTo(r.getCentre().toFloat());

    PathStrokeType pst(2.0f, PathStrokeType::JointStyle::curved);

    auto color = toggleButton.getToggleState() ? Colours::red : Colours::green;

    g.setColour(color);
    g.strokePath(powerButton, pst);

    g.drawEllipse(r.toFloat(), 2);
}

//==============================================================================
myLookAndFeelV3::myLookAndFeelV3() {
    juce::File knobImageFile2 = juce::File::getSpecialLocation
            (juce::File::SpecialLocationType::userDesktopDirectory).getChildFile("knob2.png");
    img2 = juce::ImageCache::getFromFile(knobImageFile2);
}

//==============================================================================
void myLookAndFeelV3::drawRotarySlider(juce::Graphics &g,
                                       int x, int y, int width, int height, float sliderPos,
                                       float rotaryStartAngle, float rotaryEndAngle, juce::Slider &slider) {
    if (img2.isValid()) {
        const double rotation = (slider.getValue()
                                 - slider.getMinimum())
                                / (slider.getMaximum()
                                   - slider.getMinimum());

        const int frames = img2.getHeight() / img2.getWidth();
        const int frameId = (int) ceil(rotation * ((double) frames - 1.0));
        const float radius = juce::jmin(width / 2.0f, height / 2.0f);
        const float centerX = x + width * 0.5f;
        const float centerY = y + height * 0.5f;
        const float rx = centerX - radius - 1.0f;
        const float ry = centerY - radius;

        g.drawImage(img2,
                    (int) rx,
                    (int) ry,
                    2 * (int) radius,
                    2 * (int) radius,
                    0,
                    frameId * img2.getWidth(),
                    img2.getWidth(),
                    img2.getWidth());
    } else {
        static const float textPpercent = 0.35f;
        juce::Rectangle<float> text_bounds(1.0f + width * (1.0f - textPpercent) / 2.0f,
                                           0.5f * height, width * textPpercent, 0.5f * height);

        g.setColour(juce::Colours::white);

        g.drawFittedText(juce::String("No Image"), text_bounds.getSmallestIntegerContainer(),
                         juce::Justification::horizontallyCentred | juce::Justification::centred, 1);
    }
}
