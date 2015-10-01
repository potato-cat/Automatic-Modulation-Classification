#include "uhdmock.h"

UhdMock::UhdMock(StreamFunction * func, double rate, double fc, double gain, size_t frameSize) :
    Streamer(),
    _maxBuffSize(16384),
    _buffer(new SharedBuffer<std::complex<double> >()),
    _func(func),
    _funcMutex(),
    _streamThread(),
    _isStreaming(false),
    _rate(rate),
    _fc(fc),
    _gain(gain),
    _frameSize(frameSize)
{

}

void UhdMock::setMaxBuffer(size_t maxBuffSize)
{
    // Change the max buffer size
    _maxBuffSize = maxBuffSize;
}

void UhdMock::startStream()
{
    // Initialize run loop conditional and start main loop.
    _isStreaming = true;
    _streamThread = boost::thread(&UhdMock::runStream, this);
}

void UhdMock::changeFunc(StreamFunction * func)
{
    // Change the function used for data generation, aquire a mutex
    // before changing the function.
    boost::unique_lock < boost::mutex > funcLock(_funcMutex);
    _func.reset(func);
}

void UhdMock::stopStream()
{
    // Set the run loop conditional to false (stop the loop) and
    // join the thread.
    _isStreaming = false;
    _streamThread.join();
}

void UhdMock::runStream()
{
    double period = 1/_rate;
    double t = 0.0;

    while(_isStreaming)
    {
        // Sleep for the duration relative to the data rate, so that the rate is approximately right.
        boost::this_thread::sleep_for(boost::chrono::microseconds((long)(period * 1e6 * _frameSize)));

        // Get unique access.
        boost::shared_ptr < boost::shared_mutex > mutex = _buffer->getMutex();
        boost::unique_lock < boost::shared_mutex > lock (*mutex.get());
        boost::unique_lock < boost::mutex > funcLock(_funcMutex);

        // Generate a frame of data.
        for(unsigned int n = 0; n < _frameSize; ++n)
        {
            _buffer->getBuffer().push_back(_func->calc(t) * _gain);

            // Prohibit data buffer from getting too large.
            if(_buffer->getBuffer().size() > _maxBuffSize)
            {
                _buffer->getBuffer().pop_front();
            }

            t += period;
        }
    }
    std::cout << std::endl << "Closing UHD mock thread" << std::endl;
}

boost::shared_ptr < SharedBuffer<std::complex<double> > > UhdMock::getBuffer()
{
    return _buffer;
}
