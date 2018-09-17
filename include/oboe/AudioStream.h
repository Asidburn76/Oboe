/*
 * Copyright 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OBOE_STREAM_H_
#define OBOE_STREAM_H_

#include <cstdint>
#include <ctime>
#include "oboe/Definitions.h"
#include "oboe/ResultWithValue.h"
#include "oboe/AudioStreamBuilder.h"
#include "oboe/AudioStreamBase.h"

/** WARNING - UNDER CONSTRUCTION - THIS API WILL CHANGE. */

namespace oboe {

/**
 * The default number of nanoseconds to wait for when performing state change operations on the
 * stream, such as `start` and `stop`.
 *
 * @see oboe::AudioStream::start
 */
constexpr int64_t kDefaultTimeoutNanos = (2000 * kNanosPerMillisecond);

/**
 * Base class for Oboe C++ audio stream.
 */
class AudioStream : public AudioStreamBase {
public:

    AudioStream() {}

    /**
     * Construct an `AudioStream` using the given `AudioStreamBuilder`
     *
     * @param builder containing all the stream's attributes
     */
    explicit AudioStream(const AudioStreamBuilder &builder);

    virtual ~AudioStream() = default;

    /**
     * Open a stream based on the current settings.
     *
     * Note that we do not recommend re-opening a stream that has been closed.
     * TODO Should we prevent re-opening?
     *
     * @return
     */
    virtual Result open();

    /**
     * Close the stream and deallocate any resources from the open() call.
     */
    virtual Result close() = 0;

    /**
     * Start the stream. This will block until the stream has been started, an error occurs
     * or `timeoutNanoseconds` has been reached.
     */
    virtual Result start(int64_t timeoutNanoseconds = kDefaultTimeoutNanos);

    /**
     * Pause the stream. This will block until the stream has been paused, an error occurs
     * or `timeoutNanoseconds` has been reached.
     */
    virtual Result pause(int64_t timeoutNanoseconds = kDefaultTimeoutNanos);

    /**
     * Flush the stream. This will block until the stream has been flushed, an error occurs
     * or `timeoutNanoseconds` has been reached.
     */
    virtual Result flush(int64_t timeoutNanoseconds = kDefaultTimeoutNanos);

    /**
     * Stop the stream. This will block until the stream has been stopped, an error occurs
     * or `timeoutNanoseconds` has been reached.
     */
    virtual Result stop(int64_t timeoutNanoseconds = kDefaultTimeoutNanos);

    /* Asynchronous requests.
     * Use waitForStateChange() if you need to wait for completion.
     */

    /**
     * Start the stream asynchronously. Returns immediately (does not block). Equivalent to calling
     * `start(0)`.
     */
    virtual Result requestStart() = 0;

    /**
     * Pause the stream asynchronously. Returns immediately (does not block). Equivalent to calling
     * `pause(0)`.
     */
    virtual Result requestPause() = 0;

    /**
     * Flush the stream asynchronously. Returns immediately (does not block). Equivalent to calling
     * `flush(0)`.
     */
    virtual Result requestFlush() = 0;

    /**
     * Stop the stream asynchronously. Returns immediately (does not block). Equivalent to calling
     * `stop(0)`.
     */
    virtual Result requestStop() = 0;

    /**
     * Query the current state, eg. StreamState::Pausing
     *
     * @return state or a negative error.
     */
    virtual StreamState getState() = 0;

    /**
     * Wait until the stream's current state no longer matches the input state.
     * The input state is passed to avoid race conditions caused by the state
     * changing between calls.
     *
     * Note that generally applications do not need to call this. It is considered
     * an advanced technique.
     *
     * <pre><code>
     * int64_t timeoutNanos = 500 * kNanosPerMillisecond; // arbitrary 1/2 second
     * StreamState currentState = stream->getState();
     * StreamState nextState = StreamState::Unknown;
     * while (result == Result::OK && currentState != StreamState::Paused) {
     *     result = stream->waitForStateChange(
     *                                   currentState, &nextState, timeoutNanos);
     *     currentState = nextState;
     * }
     * </code></pre>
     *
     * @param inputState The state we want to avoid.
     * @param nextState Pointer to a variable that will be set to the new state.
     * @param timeoutNanoseconds The maximum time to wait in nanoseconds.
     * @return Result::OK or a Result::Error.
     */
    virtual Result waitForStateChange(StreamState inputState,
                                          StreamState *nextState,
                                          int64_t timeoutNanoseconds) = 0;

    /**
    * This can be used to adjust the latency of the buffer by changing
    * the threshold where blocking will occur.
    * By combining this with getXRunCount(), the latency can be tuned
    * at run-time for each device.
    *
    * This cannot be set higher than getBufferCapacity().
    *
    * @param requestedFrames requested number of frames that can be filled without blocking
    * @return the resulting buffer size in frames (obtained using value()) or an error (obtained
    * using error())
    */
    virtual ResultWithValue<int32_t> setBufferSizeInFrames(int32_t requestedFrames) {
        return Result::ErrorUnimplemented;
    }

    /**
     * An XRun is an Underrun or an Overrun.
     * During playing, an underrun will occur if the stream is not written in time
     * and the system runs out of valid data.
     * During recording, an overrun will occur if the stream is not read in time
     * and there is no place to put the incoming data so it is discarded.
     *
     * An underrun or overrun can cause an audible "pop" or "glitch".
     *
     * @return a result which is either Result::OK with the xRun count as the value, or a
     * Result::Error* code
     */
    virtual ResultWithValue<int32_t> getXRunCount() const {
        return ResultWithValue<int32_t>(Result::ErrorUnimplemented);
    }

    /**
     * @return true if XRun counts are supported on the stream
     */
    virtual bool isXRunCountSupported() const = 0;

    /**
     * Query the number of frames that are read or written by the endpoint at one time.
     *
     * @return burst size
     */
    virtual int32_t getFramesPerBurst() = 0;

    /**
     * Indicates whether the audio stream is playing.
     *
     * @deprecated check the stream state directly using `AudioStream::getState`.
     */
    bool isPlaying();

    /**
     * Get the number of bytes in each audio frame. This is calculated using the channel count
     * and the sample format. For example, a 2 channel floating point stream will have
     * 2 * 4 = 8 bytes per frame.
     *
     * @return number of bytes in each audio frame.
     */
    int32_t getBytesPerFrame() const { return mChannelCount * getBytesPerSample(); }

    /**
     * Get the number of bytes per sample. This is calculated using the sample format. For example,
     * a stream using 16-bit integer samples will have 2 bytes per sample.
     *
     * @return the number of bytes per sample.
     */
    int32_t getBytesPerSample() const;

    /**
     * The number of audio frames written into the stream.
     * This monotonic counter will never get reset.
     *
     * @return the number of frames written so far
     */
    virtual int64_t getFramesWritten() { return mFramesWritten; }

    /**
     * The number of audio frames read from the stream.
     * This monotonic counter will never get reset.
     *
     * @return the number of frames read so far
     */
    virtual int64_t getFramesRead() { return mFramesRead; }

    /**
     * Calculate the latency of a stream based on getTimestamp().
     *
     * Output latency is the time it takes for a given frame to travel from the
     * app to some type of digital-to-analog converter. If the DAC is external, for example
     * in a USB interface or a TV connected by HDMI, then there may be additional latency
     * that the Android device is unaware of.
     *
     * Input latency is the time it takes to a given frame to travel from an analog-to-digital
     * converter (ADC) to the app.
     *
     * Note that the latency of an OUTPUT stream will increase abruptly when you write data to it
     * and then decrease slowly over time as the data is consumed.
     *
     * The latency of an INPUT stream will decrease abruptly when you read data from it
     * and then increase slowly over time as more data arrives.
     *
     * The latency of an OUTPUT stream is generally higher than the INPUT latency
     * because an app generally tries to keep the OUTPUT buffer full and the INPUT buffer empty.
     *
     * @return a ResultWithValue which has a result of Result::OK and a value containing the latency
     * in milliseconds, or a result of Result::Error*.
     */
    virtual ResultWithValue<double> calculateLatencyMillis() {
        return ResultWithValue<double>(Result::ErrorUnimplemented);
    }

    /**
     * Get the estimated time that the frame at `framePosition` entered or left the audio processing
     * pipeline.
     *
     * This can be used to coordinate events and interactions with the external environment, and to
     * estimate the latency of an audio stream. An example of usage can be found in the hello-oboe
     * sample (search for "calculateCurrentOutputLatencyMillis").
     *
     * The time is based on the implementation's best effort, using whatever knowledge is available
     * to the system, but cannot account for any delay unknown to the implementation.
     *
     * @param clockId the type of clock to use e.g. CLOCK_MONOTONIC
     * @param framePosition the frame number to query
     * @param timeNanoseconds an output parameter which will contain the presentation timestamp
     * (if the operation is successful)
     */
    virtual Result getTimestamp(clockid_t clockId,
                                int64_t *framePosition,
                                int64_t *timeNanoseconds) {
        return Result::ErrorUnimplemented;
    }

    // ============== I/O ===========================
    /**
     * Write data from the supplied buffer into the stream. This method will block until the write
     * is complete or it runs out of time.
     *
     * If `timeoutNanoseconds` is zero then this call will not wait.
     *
     * @param buffer The address of the first sample.
     * @param numFrames Number of frames to write. Only complete frames will be written.
     * @param timeoutNanoseconds Maximum number of nanoseconds to wait for completion.
     * @return a ResultWithValue which has a result of Result::OK and a value containing the number
     * of frames actually written, or result of Result::Error*.
     */
    virtual ResultWithValue<int32_t> write(const void *buffer,
                             int32_t numFrames,
                             int64_t timeoutNanoseconds) {
        return ResultWithValue<int32_t>(Result::ErrorUnimplemented);
    }

    /**
     * Read data into the supplied buffer from the stream. This method will block until the read
     * is complete or it runs out of time.
     *
     * If `timeoutNanoseconds` is zero then this call will not wait.
     *
     * @param buffer The address of the first sample.
     * @param numFrames Number of frames to read. Only complete frames will be read.
     * @param timeoutNanoseconds Maximum number of nanoseconds to wait for completion.
     * @return a ResultWithValue which has a result of Result::OK and a value containing the number
     * of frames actually read, or result of Result::Error*.
     */
    virtual ResultWithValue<int32_t> read(void *buffer,
                            int32_t numFrames,
                            int64_t timeoutNanoseconds) {
        return ResultWithValue<int32_t>(Result::ErrorUnimplemented);
    }

    /**
     * Get the underlying audio API which the stream uses.
     *
     * @return the API that this stream uses.
     */
    virtual AudioApi getAudioApi() const = 0;

    /**
     * Returns true if the underlying audio API is AAudio.
     *
     * @return true if this stream is implemented using the AAudio API.
     */
    bool usesAAudio() const {
        return getAudioApi() == AudioApi::AAudio;
    }

    /**
     * Only for debugging. Do not use in production.
     * If you need to call this method something is wrong.
     * If you think you need it for production then please let us know
     * so we can modify Oboe so that you don't need this.
     *
     * @return nullptr or a pointer to a stream from the system API
     */
    virtual void *getUnderlyingStream() const {
        return nullptr;
    }

protected:

    /**
     * Increment the frames written to this stream
     *
     * @param frames number of frames to increment by
     * @return total frames which have been written
     */
    virtual int64_t incrementFramesWritten(int32_t frames) {
        return mFramesWritten += frames;
    }

    /**
     * Increment the frames which have been read from this stream
     *
     * @param frames number of frames to increment by
     * @return total frames which have been read
     */
    virtual int64_t incrementFramesRead(int32_t frames) {
        return mFramesRead += frames;
    }

    /**
     * Wait for a transition from one state to another.
     * @return OK if the endingState was observed, or ErrorUnexpectedState
     *   if any state that was not the startingState or endingState was observed
     *   or ErrorTimeout.
     */
    virtual Result waitForStateTransition(StreamState startingState,
                                          StreamState endingState,
                                          int64_t timeoutNanoseconds);

    /**
     * Override this to provide a default for when the application did not specify a callback.
     *
     * @param audioData
     * @param numFrames
     * @return result
     */
    virtual DataCallbackResult onDefaultCallback(void *audioData, int numFrames) {
        return DataCallbackResult::Stop;
    }

    /**
     * Override this to provide your own behaviour for the audio callback
     *
     * @param audioData container array which audio frames will be written into or read from
     * @param numFrames number of frames which were read/written
     * @return the result of the callback: stop or continue
     *
     */
    DataCallbackResult fireCallback(void *audioData, int numFrames);

    /**
     * Used to set the format of the underlying stream
     */
    virtual void setNativeFormat(AudioFormat format) {
        mNativeFormat = format;
    }

    AudioFormat mNativeFormat = AudioFormat::Invalid;

    /**
     * Number of frames which have been written into the stream
     *
     * TODO these should be atomic like in AAudio
     */
    int64_t              mFramesWritten = 0;

    /**
     * Number of frames which have been read from the stream
     *
     * TODO these should be atomic like in AAudio
     */
    int64_t              mFramesRead = 0;

private:
    int                  mPreviousScheduler = -1;
};

} // namespace oboe

#endif /* OBOE_STREAM_H_ */
