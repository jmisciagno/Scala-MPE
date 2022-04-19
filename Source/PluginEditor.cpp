/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
NewProjectAudioProcessorEditor::NewProjectAudioProcessorEditor (NewProjectAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    
        addAndMakeVisible(fileNameLabel);      
        fileNameLabel.setText("Scala file:", juce::dontSendNotification);
        fileNameLabel.setColour (juce::Label::textColourId, juce::Colours::white);
        
        // Filename Label Code
        addAndMakeVisible(fileNameText);      
        fileNameText.setEditable(true);
        fileNameText.setText (audioProcessor.path, juce::dontSendNotification);
        fileNameText.setColour (juce::Label::backgroundColourId, juce::Colours::white);
        fileNameText.setColour(juce::Label::textWhenEditingColourId, juce::Colours::black);
        fileNameText.setColour (juce::Label::textColourId, juce::Colours::black);
        fileNameText.onTextChange = [this]
        {
            audioProcessor.path = fileNameText.getText().toStdString();
            audioProcessor.error = this->audioProcessor.loadFile(audioProcessor.path);
            if (audioProcessor.error)
            {
                audioProcessor.message = "Error loading file...reverting to 12-ET.";
                errorText.setColour (juce::Label::textColourId, juce::Colours::orange);
                errorText.setText (audioProcessor.message, juce::dontSendNotification);
            }
            else
            {
                string filename =  audioProcessor.path.substr(audioProcessor.path.find_last_of("/\\") + 1);
                audioProcessor.message = "Sucessfully loaded: " + filename;
                errorText.setColour (juce::Label::textColourId, juce::Colours::lightgreen);
                errorText.setText (audioProcessor.message, juce::dontSendNotification);
            }
        };
        
        // Error Label Code
        addAndMakeVisible(errorText);
        if (audioProcessor.error)
        {
            errorText.setColour (juce::Label::textColourId, juce::Colours::orange);
            errorText.setText (audioProcessor.message, juce::dontSendNotification);
        }
        else
        {
            errorText.setColour (juce::Label::textColourId, juce::Colours::lightgreen);
            errorText.setText (audioProcessor.message, juce::dontSendNotification);
        }
        
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);
}

NewProjectAudioProcessorEditor::~NewProjectAudioProcessorEditor()
{
}

//==============================================================================
void NewProjectAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
        g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void NewProjectAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
        int width = getWidth();
        int height = getHeight();
        fileNameLabel.setBounds (30, 50, width - 150, 20);
        fileNameText.setBounds (100, 50, width - 150, 20);
        errorText.setBounds (100, 80, width - 150, 20);
}
