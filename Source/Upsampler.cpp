#include "base.h"
#include "FrequencySweepAudioSource.h"
#include "Upsampler.h"
#include "wavefile.h"

#undef T        // fix conflict with legacy JUCE macro
#pragma warning(disable:4201)
#pragma warning(disable:4127)
#include "r8brain/CDSPResampler.h"

#if 0
void Upsampler::test()
{
    //
    // Make some audio data
    //
    int sampleRate = 48000;
    FrequencySweepAudioSource generator;
    AudioSampleBuffer sourceBuffer(1, sampleRate * 2);
    AudioSourceChannelInfo asi(&sourceBuffer, 0, sourceBuffer.getNumSamples());
    
    generator.prepareToPlay(sourceBuffer.getNumSamples(), sampleRate);
    generator.getNextAudioBlock(asi);
    generator.releaseResources();

    //
    // Create the upsampler
    //
    int const upsampleFactor = 4;
    Upsampler upsampler(sampleRate, sampleRate * upsampleFactor, 2.0);
    HeapBlock<double> upsamplerInputBuffer, upsamplerOutputBuffer;
    int upsamplerOutputBufferSamples = sourceBuffer.getNumSamples() * upsampleFactor * 2;
    
    upsamplerInputBuffer.allocate(sourceBuffer.getNumSamples(), true);
    upsamplerOutputBuffer.allocate(upsamplerOutputBufferSamples, true);
    
    //
    // Convert data to doubles
    //
    const float* source = sourceBuffer.getReadPointer(0);
    for (int i = 0; i < sourceBuffer.getNumSamples(); ++i)
    {
        upsamplerInputBuffer[i] = source[i];
    }
    
    //
    // Upsample
    //
    int upsampledCount = upsampler.upsample( upsamplerInputBuffer,
                                            upsamplerOutputBuffer,
                                            sourceBuffer.getNumSamples(),
                                            upsamplerOutputBufferSamples);
    
    //
    // Convert upsampled data to float
    //
    AudioSampleBuffer finalBuffer(1, upsamplerOutputBufferSamples);
    
    float *destination = finalBuffer.getWritePointer(0);
    for (int i = 0; i < upsampledCount; ++i)
    {
        destination[i] = (float)upsamplerOutputBuffer[i];
    }
    
    WriteWaveFile("upsample.wav", sampleRate * upsampleFactor, &finalBuffer, upsamplerOutputBufferSamples);
}
#endif


Upsampler::Upsampler(double inputSampleRate_, double outputSampleRate_) :
inputSampleRate(inputSampleRate_),
outputSampleRate(outputSampleRate_)
{
    resampler = new r8b::CDSPResampler24( inputSampleRate_, outputSampleRate_, MAX_RESAMPLER_INPUT_SAMPLES);

	inputBlockBuffer.allocate(MAX_RESAMPLER_INPUT_SAMPLES,true);
}

Upsampler::~Upsampler()
{
	delete resampler;
}

void Upsampler::upsample(AudioSampleBuffer *inputBuffer)
{
    outputSampleCount = 0;
    
    //DBG("upsample inputSampleCount:" << inputSampleCount);
    
    if (nullptr == resampler)
        return;

	resampler->clear();
    
	int inputSampleCount = inputBuffer->getNumSamples();
	int inputBufferIndex = 0;
	int outputBufferIndex = 0;
    while (inputSampleCount > 0)
    {
		//
		// Convert float to double
		//
		const float *inputBufferReadPtr = inputBuffer->getReadPointer(0, inputBufferIndex);
		int inputConvertCount = jmin(inputSampleCount, (int)MAX_RESAMPLER_INPUT_SAMPLES);
		for (int i = 0; i < inputConvertCount; ++i)
		{
			inputBlockBuffer[i] = inputBufferReadPtr[i];
		}
		inputBufferIndex += inputConvertCount;
		inputSampleCount -= inputConvertCount;

		//
		// Run the SRC
		//
        double* outputBlock = nullptr;
        
		int outputBlockSampleCount = resampler->process(inputBlockBuffer, inputConvertCount, outputBlock);
        int outputSpaceRemaining = maxOutputSampleCount - outputSampleCount;
        int outputCopyCount = jmin( outputSpaceRemaining, outputBlockSampleCount);
        
        for (int i = 0; i < outputCopyCount; ++i)
        {
			outputBuffer[outputBufferIndex] = outputBlock[i];
			outputBufferIndex++;
        }
        
        outputSampleCount += outputCopyCount;
    }
    
    //DBG("   outputSampleCount:" << outputSampleCount);
}

void Upsampler::setOutputBufferSize(double seconds)
{
	maxOutputSampleCount = roundDoubleToInt(seconds * outputSampleRate);
	outputBuffer.allocate(maxOutputSampleCount, true);
}